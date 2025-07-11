/****************************************************************************
 * fs/mmap/fs_rammap.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/fs/fs.h>
#include <nuttx/kmalloc.h>
#include <nuttx/sched.h>

#include "fs_rammap.h"
#include "sched/sched.h"
#include "fs_heap.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: msync_rammap
 ****************************************************************************/

static int msync_rammap(FAR struct mm_map_entry_s *entry, FAR void *start,
                        size_t length, int flags)
{
  FAR struct file *filep = (FAR void *)((uintptr_t)entry->priv.p & ~3);
  FAR uint8_t *wrbuffer = start;
  ssize_t nwrite = 0;
  off_t offset;
  off_t fpos;
  off_t opos;

  offset = (uintptr_t)start - (uintptr_t)entry->vaddr;
  if (length > entry->length - offset)
    {
      length = entry->length - offset;
    }

  opos = file_seek(filep, 0, SEEK_CUR);
  if (opos < 0)
    {
      ferr("ERROR: Get current position failed\n");
      return opos;
    }

  fpos = file_seek(filep, entry->offset + offset, SEEK_SET);
  if (fpos < 0)
    {
      ferr("ERROR: Seek to position %"PRIdOFF" failed\n", fpos);
      return fpos;
    }

  while (length > 0)
    {
      nwrite = file_write(filep, wrbuffer, length);
      if (nwrite < 0)
        {
          /* Handle the special case where the write was interrupted by a
           * signal.
           */

          if (nwrite != -EINTR)
            {
              /* All other write errors are bad. */

              ferr("ERROR: Write failed: offset=%"PRIdOFF" nwrite=%zd\n",
                   entry->offset, nwrite);
              break;
            }
        }

      /* Increment number of bytes written */

      wrbuffer += nwrite;
      length   -= nwrite;
    }

  /* Restore file pos */

  fpos = file_seek(filep, opos, SEEK_SET);
  if (fpos < 0)
    {
      /* Ensure that we finally seek back to the current file pos */

      ferr("ERROR: Seek back to position %"PRIdOFF" failed\n", fpos);
      return fpos;
    }

  return nwrite >= 0 ? 0 : nwrite;
}

/****************************************************************************
 * Name: unmap_rammap
 ****************************************************************************/

static int unmap_rammap(FAR struct task_group_s *group,
                        FAR struct mm_map_entry_s *entry,
                        FAR void *start,
                        size_t length)
{
  FAR struct file *filep = (FAR void *)((uintptr_t)entry->priv.p & ~3);
  enum mm_map_type_e type =
                    (enum mm_map_type_e)((uintptr_t)entry->priv.p & 3);
  FAR void *newaddr = NULL;
  off_t offset;
  int ret = OK;

  /* Get the offset from the beginning of the region and the actual number
   * of bytes to "unmap".  All mappings must extend to the end of the region.
   * There is no support for freeing a block of memory but leaving a block of
   * memory at the end.  This is a consequence of using kumm_realloc() to
   * simulate the unmapping.
   */

  offset = (uintptr_t)start - (uintptr_t)entry->vaddr;
  if (offset + length < entry->length)
    {
      ferr("ERROR: Cannot umap without unmapping to the end\n");
      return -ENOSYS;
    }

  /* Okay.. the region is being unmapped to the end.  Make sure the length
   * indicates that.
   */

  length = entry->length - offset;

  /* Are we unmapping the entire region (offset == 0)? */

  if (length >= entry->length)
    {
      /* Free the region */

      if (type == MAP_KERNEL)
        {
          fs_heap_free(entry->vaddr);
        }
      else if (type == MAP_USER)
        {
          kumm_free(entry->vaddr);
        }

      file_put(filep);

      /* Then remove the mapping from the list */

      ret = mm_map_remove(get_group_mm(group), entry);
    }

  /* No.. We have been asked to "unmap' only a portion of the memory
   * (offset > 0).
   */

  else
    {
      if (type == MAP_KERNEL)
        {
          newaddr = fs_heap_realloc(entry->vaddr, length);
        }
      else if (type == MAP_USER)
        {
          newaddr = kumm_realloc(entry->vaddr, length);
        }

      DEBUGASSERT(newaddr == entry->vaddr);
      entry->vaddr = newaddr;
      entry->length = length;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rammmap
 *
 * Description:
 *   Support simulation of memory mapped files by copying files into RAM.
 *
 * Input Parameters:
 *   filep   file descriptor of the backing file -- required.
 *   entry   mmap entry information.
 *           field offset and length must be initialized correctly.
 *   type    fs_heap_zalloc or kumm_zalloc or xip_base
 *
 * Returned Value:
 *  On success, rammap returns 0 and entry->vaddr points to memory mapped.
 *     Otherwise errno is returned appropriately.
 *
 *     EBADF
 *      'fd' is not a valid file descriptor.
 *     EINVAL
 *       'length' or 'offset' are invalid
 *     ENOMEM
 *       Insufficient memory is available to map the file.
 *
 ****************************************************************************/

int rammap(FAR struct file *filep, FAR struct mm_map_entry_s *entry,
           enum mm_map_type_e type)
{
  FAR uint8_t *rdbuffer;
  ssize_t nread;
  off_t fpos;
  int ret;
  size_t length = entry->length;

  ret = file_ioctl(filep, BIOC_XIPBASE, (unsigned long)&entry->vaddr);
  if (ret == OK)
    {
      type = MAP_XIP;
      goto out;
    }

  /* There is a major design flaw that I have not yet thought of fix for:
   * The goal is to have a single region of memory that represents a single
   * file and can be shared by many threads.  That is, given a filename a
   * thread should be able to open the file, get a file descriptor, and
   * call mmap() to get a memory region.  Different file descriptors opened
   * with the same file path should get the same memory region when mapped.
   *
   * The design flaw is that I don't have sufficient knowledge to know that
   * these different file descriptors map to the same file.  So, for the time
   * being, a new memory region is created each time that rammap() is called.
   * Not very useful!
   */

  /* Allocate a region of memory of the specified size */

  rdbuffer = type == MAP_KERNEL ? fs_heap_malloc(length)
                                : kumm_malloc(length);
  if (!rdbuffer)
    {
      ferr("ERROR: Region allocation failed, length: %zu\n", length);
      return -ENOMEM;
    }

  entry->vaddr = rdbuffer; /* save the buffer firstly */

  /* Seek to the specified file offset */

  fpos = file_seek(filep, entry->offset, SEEK_SET);
  if (fpos < 0)
    {
      /* Seek failed... errno has already been set, but EINVAL is probably
       * the correct response.
       */

      ferr("ERROR: Seek to position %zu failed\n", (size_t)entry->offset);
      ret = fpos;
      goto errout_with_region;
    }

  /* Read the file data into the memory region */

  while (length > 0)
    {
      nread = file_read(filep, rdbuffer, length);
      if (nread < 0)
        {
          /* Handle the special case where the read was interrupted by a
           * signal.
           */

          if (nread != -EINTR)
            {
              /* All other read errors are bad. */

              ferr("ERROR: Read failed: offset=%zu ret=%zd\n",
                   (size_t)entry->offset, nread);

              ret = nread;
              goto errout_with_region;
            }
        }

      /* Check for end of file. */

      if (nread == 0)
        {
          break;
        }

      /* Increment number of bytes read */

      rdbuffer += nread;
      length   -= nread;
    }

  /* Zero any memory beyond the amount read from the file */

  memset(rdbuffer, 0, length);

  /* Add the buffer to the list of regions */

out:
  file_ref(filep);
  entry->priv.p = (FAR void *)((uintptr_t)filep | type);
  entry->munmap = unmap_rammap;
  entry->msync = msync_rammap;

  ret = mm_map_add(get_current_mm(), entry);
  if (ret < 0)
    {
      goto errout_with_region;
    }

  return OK;

errout_with_region:
  if (type == MAP_KERNEL)
    {
      fs_heap_free(entry->vaddr);
    }
  else if (type == MAP_USER)
    {
      kumm_free(entry->vaddr);
    }

  return ret;
}
