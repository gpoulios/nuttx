/****************************************************************************
 * drivers/lcd/pcd8544.c
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/pcd8544.h>

#include "pcd8544.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* PCD8544 Configuration Settings:
 *
 * CONFIG_PCD8544_SPIMODE - Controls the SPI mode
 * CONFIG_PCD8544_FREQUENCY - Define to use a different bus frequency
 * CONFIG_PCD8544_NINTERFACES - Specifies the number of physical
 *   PCD8544 devices that will be supported.  NOTE:  At present, this
 *   must be undefined or defined to be 1.
 * CONFIG_LCD_PCD8544DEBUG - Enable detailed PCD8544 debug pcd8544 output
 *   (CONFIG_DEBUG_FEATURES and CONFIG_VERBOSE must also be enabled).
 *
 * Required LCD driver settings:
 * CONFIG_LCD_PCD8544 - Enable PCD8544 support
 * CONFIG_LCD_MAXCONTRAST should be 255, but any value >0 and <=255 will be
 * accepted.
 * CONFIG_LCD_MAXPOWER should be 1:  0=off, 1=normal
 *
 * Required SPI driver settings:
 * CONFIG_SPI_CMDDATA - Include support for cmd/data selection.
 */

/* Verify that all configuration requirements have been met */

#ifndef CONFIG_PCD8544_SPIMODE
#  define CONFIG_PCD8544_SPIMODE SPIDEV_MODE0
#endif

/* SPI frequency */

#ifndef CONFIG_PCD8544_FREQUENCY
#  define CONFIG_PCD8544_FREQUENCY 3000000
#endif

/* CONFIG_PCD8544_NINTERFACES determines the number of physical interfaces
 * that will be supported.
 */

#ifndef CONFIG_PCD8544_NINTERFACES
#  define CONFIG_PCD8544_NINTERFACES 1
#endif

#if CONFIG_PCD8544_NINTERFACES != 1
#  warning "Only a single PCD8544 interface is supported"
#  undef CONFIG_PCD8544_NINTERFACES
#  define CONFIG_PCD8544_NINTERFACES 1
#endif

/* Verbose debug pcd8544 must also be enabled to use the extra OLED debug
 * pcd8544
 */

#ifndef CONFIG_DEBUG_FEATURES
#  undef CONFIG_DEBUG_INFO
#  undef CONFIG_DEBUG_GRAPHICS
#endif

#ifndef CONFIG_DEBUG_INFO
#  undef CONFIG_LCD_PCD8544DEBUG
#endif

/* Check contrast selection */

#ifndef CONFIG_LCD_MAXCONTRAST
#  define CONFIG_LCD_MAXCONTRAST 127
#endif

#if CONFIG_LCD_MAXCONTRAST <= 0 || CONFIG_LCD_MAXCONTRAST > 127
#  error "CONFIG_LCD_MAXCONTRAST exceeds supported maximum"
#endif

#if CONFIG_LCD_MAXCONTRAST < 60
#  warning "Optimal setting of CONFIG_LCD_MAXCONTRAST is 60"
#endif

/* Check power setting */

#if !defined(CONFIG_LCD_MAXPOWER)
#  define CONFIG_LCD_MAXPOWER 1
#endif

#if CONFIG_LCD_MAXPOWER != 1
#  warning "CONFIG_LCD_MAXPOWER should be 1"
#  undef CONFIG_LCD_MAXPOWER
#  define CONFIG_LCD_MAXPOWER 1
#endif

/* The Display requires CMD/DATA SPI support */

#ifndef CONFIG_SPI_CMDDATA
#  error "CONFIG_SPI_CMDDATA must be defined in your NuttX configuration"
#endif

/* Color Properties *********************************************************/

/* The PCD8544 display controller can handle a resolution of 84x48. */

/* Display Resolution */

#ifdef CONFIG_PCD8544_XRES
#define PCD8544_XRES         CONFIG_PCD8544_XRES
#else
#define PCD8544_XRES         84
#endif

#ifdef CONFIG_PCD8544_YRES
#define PCD8544_YRES         CONFIG_PCD8544_YRES
#else
#define PCD8544_YRES         48
#endif

/* Color depth and format */

#define PCD8544_BPP          1
#define PCD8544_COLORFMT     FB_FMT_Y1

/* Bytes per logical row and actual device row */

#define PCD8544_XSTRIDE      (PCD8544_XRES >> 3) /* Pixels arrange "horizontally for user" */
#define PCD8544_YSTRIDE      (PCD8544_YRES >> 3) /* But actual device arrangement is "vertical" */

/* The size of the shadow frame buffer */

#define PCD8544_FBSIZE       (PCD8544_XRES * PCD8544_YSTRIDE)

/* Bit helpers */

#define LS_BIT          (1 << 0)
#define MS_BIT          (1 << 7)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure describes the state of this driver */

struct pcd8544_dev_s
{
  /* Publicly visible device structure */

  struct lcd_dev_s dev;

  /* Private LCD-specific information follows */

  FAR struct spi_dev_s *spi;
  uint8_t contrast;
  uint8_t powered;

  /* The PCD8544 does not support reading from the display memory in SPI
   * mode. Since there is 1 BPP and access is byte-by-byte, it is necessary
   * to keep a shadow copy of the framebuffer memory.
   */

  uint8_t fb[PCD8544_FBSIZE];
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

/* SPI helpers */

static void pcd8544_select(FAR struct spi_dev_s *spi);
static void pcd8544_deselect(FAR struct spi_dev_s *spi);

/* LCD Data Transfer Methods */

static int pcd8544_putrun(FAR struct lcd_dev_s *dev,
                          fb_coord_t row, fb_coord_t col,
                          FAR const uint8_t *buffer, size_t npixels);
static int pcd8544_getrun(FAR struct lcd_dev_s *dev,
                          fb_coord_t row, fb_coord_t col,
                          FAR uint8_t *buffer, size_t npixels);

/* LCD Configuration */

static int pcd8544_getvideoinfo(FAR struct lcd_dev_s *dev,
                                FAR struct fb_videoinfo_s *vinfo);
static int pcd8544_getplaneinfo(FAR struct lcd_dev_s *dev,
                                unsigned int planeno,
                                FAR struct lcd_planeinfo_s *pinfo);

/* LCD RGB Mapping */

#ifdef CONFIG_FB_CMAP
#  error "RGB color mapping not supported by this driver"
#endif

/* Cursor Controls */

#ifdef CONFIG_FB_HWCURSOR
#  error "Cursor control not supported by this driver"
#endif

/* LCD Specific Controls */

static int pcd8544_getpower(struct lcd_dev_s *dev);
static int pcd8544_setpower(struct lcd_dev_s *dev, int power);
static int pcd8544_getcontrast(struct lcd_dev_s *dev);
static int pcd8544_setcontrast(struct lcd_dev_s *dev, unsigned int contrast);

/* Initialization */

static inline void up_clear(FAR struct pcd8544_dev_s  *priv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This is working memory allocated by the LCD driver for each LCD device
 * and for each color plane.  This memory will hold one raster line of data.
 * The size of the allocated run buffer must therefore be at least
 * (bpp * xres / 8).  Actual alignment of the buffer must conform to the
 * bitwidth of the underlying pixel type.
 *
 * If there are multiple planes, they may share the same working buffer
 * because different planes will not be operate on concurrently.  However,
 * if there are multiple LCD devices, they must each have unique run buffers.
 */

static uint8_t g_runbuffer[PCD8544_XSTRIDE + 1];

/* This structure describes the overall LCD video controller */

static const struct fb_videoinfo_s g_videoinfo =
{
  PCD8544_COLORFMT,    /* Color format: RGB16-565: RRRR RGGG GGGB BBBB */
  PCD8544_XRES,        /* Horizontal resolution in pixel columns */
  PCD8544_YRES,        /* Vertical resolution in pixel rows */
  1,                   /* Number of color planes supported */
};

/* This is the standard, NuttX Plane information object */

static const struct lcd_planeinfo_s g_planeinfo =
{
  .putrun = pcd8544_putrun,              /* Put a run into LCD memory */
  .getrun = pcd8544_getrun,              /* Get a run from LCD memory */
  .buffer = (FAR uint8_t *)g_runbuffer,  /* Run scratch buffer */
  .bpp    = PCD8544_BPP,                 /* Bits-per-pixel */
};

/* This is the standard, NuttX LCD driver object */

static struct pcd8544_dev_s g_pcd8544dev =
{
  /* struct lcd_dev_s */

  {
    /* LCD Configuration */

    pcd8544_getvideoinfo,
    pcd8544_getplaneinfo,

    /* LCD RGB Mapping -- Not supported */
#ifdef CONFIG_FB_CMAP
    NULL,
    NULL,
#endif

    /* Cursor Controls -- Not supported */
#ifdef CONFIG_FB_HWCURSOR
    NULL,
    NULL,
#endif

    /* LCD Specific Controls */

    pcd8544_getpower,
    pcd8544_setpower,
    pcd8544_getcontrast,
    pcd8544_setcontrast,
  },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  pcd8544_powerstring
 *
 * Description:
 *   Convert the power setting to a string.
 *
 ****************************************************************************/

static inline FAR const char *pcd8544_powerstring(uint8_t power)
{
  if (power == PCD8544_POWER_OFF)
    {
      return "OFF";
    }
  else if (power == PCD8544_POWER_ON)
    {
      return "ON";
    }
  else
    {
      return "ERROR";
    }
}

/****************************************************************************
 * Name: pcd8544_select
 *
 * Description:
 *   Select the SPI, locking and  re-configuring if necessary
 *
 * Input Parameters:
 *   spi  - Reference to the SPI driver structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void pcd8544_select(FAR struct spi_dev_s *spi)
{
  /* Select PCD8544 chip (locking the SPI bus in case there are multiple
   * devices competing for the SPI bus
   */

  SPI_LOCK(spi, true);
  SPI_SELECT(spi, SPIDEV_DISPLAY(0), true);

  /* Now make sure that the SPI bus is configured for the PCD8544 (it
   * might have gotten configured for a different device while unlocked)
   */

  SPI_SETMODE(spi, CONFIG_PCD8544_SPIMODE);
  SPI_SETBITS(spi, 8);
  SPI_HWFEATURES(spi, 0);
  SPI_SETFREQUENCY(spi, CONFIG_PCD8544_FREQUENCY);
}

/****************************************************************************
 * Name: pcd8544_deselect
 *
 * Description:
 *   De-select the SPI
 *
 * Input Parameters:
 *   spi  - Reference to the SPI driver structure
 *
 * Returned Value:
 *   None
 *
 * Assumptions:
 *
 ****************************************************************************/

static void pcd8544_deselect(FAR struct spi_dev_s *spi)
{
  /* De-select PCD8544 chip and relinquish the SPI bus. */

  SPI_SELECT(spi, SPIDEV_DISPLAY(0), false);
  SPI_LOCK(spi, false);
}

/****************************************************************************
 * Name:  pcd8544_putrun
 *
 * Description:
 *   This method can be used to write a partial raster line to the LCD:
 *
 *   dev     - The lcd device
 *   row     - Starting row to write to (range: 0 <= row < yres)
 *   col     - Starting column to write to (range: 0 <= col <= xres-npixels)
 *   buffer  - The buffer containing the run to be written to the LCD
 *   npixels - The number of pixels to write to the LCD
 *             (range: 0 < npixels <= xres-col)
 *
 ****************************************************************************/

static int pcd8544_putrun(FAR struct lcd_dev_s *dev,
                          fb_coord_t row, fb_coord_t col,
                          FAR const uint8_t *buffer, size_t npixels)
{
  /* Because of this line of code, we will only be able to support a single
   * PCD8544 device
   */

  FAR struct pcd8544_dev_s *priv = (FAR struct pcd8544_dev_s *)dev;
  FAR uint8_t *fbptr;
  FAR uint8_t *ptr;
  uint8_t fbmask;
  uint8_t page;
  uint8_t usrmask;
  uint8_t i;
  int pixlen;

  ginfo("row: %d col: %d npixels: %d\n", row, col, npixels);
  DEBUGASSERT(buffer);

  /* Clip the run to the display */

  pixlen = npixels;
  if ((unsigned int)col + (unsigned int)pixlen > (unsigned int)PCD8544_XRES)
    {
      pixlen = (int)PCD8544_XRES - (int)col;
    }

  /* Verify that some portion of the run remains on the display */

  if (pixlen <= 0 || row > PCD8544_YRES)
    {
      return OK;
    }

  /* Get the page number.  The range of 48 lines is divided up into six
   * pages of 8 lines each.
   */

  page = row >> 3;

  /* Update the shadow frame buffer memory. First determine the pixel
   * position in the frame buffer memory.  Pixels are organized like
   * this:
   *
   *  --------+---+---+---+---+-...-+----+
   *  Segment | 0 | 1 | 2 | 3 | ... | 83 |
   *  --------+---+---+---+---+-...-+----+
   *  Bit 0   |   | X |   |   |     |    |
   *  Bit 1   |   | X |   |   |     |    |
   *  Bit 2   |   | X |   |   |     |    |
   *  Bit 3   |   | X |   |   |     |    |
   *  Bit 4   |   | X |   |   |     |    |
   *  Bit 5   |   | X |   |   |     |    |
   *  Bit 6   |   | X |   |   |     |    |
   *  Bit 7   |   | X |   |   |     |    |
   *  --------+---+---+---+---+-...-+----+
   *
   * So, in order to draw a white, horizontal line, at row 45. we
   * would have to modify all of the bytes in page 45/8 = 5.  We
   * would have to set bit 45%8 = 5 in every byte in the page.
   */

  fbmask  = 1 << (row & 7);
  fbptr   = &priv->fb[page * PCD8544_XRES + col];
  ptr     = fbptr;
#ifdef CONFIG_LCD_PACKEDMSFIRST
  usrmask = MS_BIT;
#else
  usrmask = LS_BIT;
#endif

  for (i = 0; i < pixlen; i++)
    {
      /* Set or clear the corresponding bit */

      if ((*buffer & usrmask) != 0)
        {
          *ptr++ |= fbmask;
        }
      else
        {
          *ptr++ &= ~fbmask;
        }

      /* Inc/Decrement to the next source pixel */

#ifdef CONFIG_LCD_PACKEDMSFIRST
      if (usrmask == LS_BIT)
        {
          buffer++;
          usrmask = MS_BIT;
        }
      else
        {
          usrmask >>= 1;
        }
#else
      if (usrmask == MS_BIT)
        {
          buffer++;
          usrmask = LS_BIT;
        }
      else
        {
          usrmask <<= 1;
        }
#endif
    }

  /* Select and lock the device */

  pcd8544_select(priv->spi);

  /* Select command transfer */

  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), true);

  /* Set the starting position for the run */

  SPI_SEND(priv->spi, PCD8544_SET_Y_ADDR + page);         /* Set the page start */
  SPI_SEND(priv->spi, PCD8544_SET_X_ADDR + (col & 0x7f)); /* Set the low column */

  /* Select data transfer */

  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), false);

  /* Then transfer all of the data */

  SPI_SNDBLOCK(priv->spi, fbptr, pixlen);

  /* Unlock and de-select the device */

  pcd8544_deselect(priv->spi);
  return OK;
}

/****************************************************************************
 * Name:  pcd8544_getrun
 *
 * Description:
 *   This method can be used to read a partial raster line from the LCD:
 *
 *  dev     - The lcd device
 *  row     - Starting row to read from (range: 0 <= row < yres)
 *  col     - Starting column to read read (range: 0 <= col <= xres-npixels)
 *  buffer  - The buffer in which to return the run read from the LCD
 *  npixels - The number of pixels to read from the LCD
 *            (range: 0 < npixels <= xres-col)
 *
 ****************************************************************************/

static int pcd8544_getrun(FAR struct lcd_dev_s *dev,
                          fb_coord_t row, fb_coord_t col,
                          FAR uint8_t *buffer, size_t npixels)
{
  /* Because of this line of code, we will only be able to support a single
   * PCD8544 device
   */

  FAR struct pcd8544_dev_s *priv = (FAR struct pcd8544_dev_s *)dev;
  FAR uint8_t *fbptr;
  uint8_t page;
  uint8_t fbmask;
  uint8_t usrmask;
  uint8_t i;
  int     pixlen;

  ginfo("row: %d col: %d npixels: %d\n", row, col, npixels);
  DEBUGASSERT(buffer);

  /* Clip the run to the display */

  pixlen = npixels;
  if ((unsigned int)col + (unsigned int)pixlen > (unsigned int)PCD8544_XRES)
    {
      pixlen = (int)PCD8544_XRES - (int)col;
    }

  /* Verify that some portion of the run is actually the display */

  if (pixlen <= 0 || row > PCD8544_YRES)
    {
      return -EINVAL;
    }

  /* Then transfer the display data from the shadow frame buffer memory */

  /* Get the page number.  The range of 48 lines is divided up into six
   * pages of 8 lines each.
   */

  page = row >> 3;

  /* Update the shadow frame buffer memory. First determine the pixel
   * position in the frame buffer memory.  Pixels are organized like
   * this:
   *
   *  --------+---+---+---+---+-...-+----+
   *  Segment | 0 | 1 | 2 | 3 | ... | 83 |
   *  --------+---+---+---+---+-...-+----+
   *  Bit 0   |   | X |   |   |     |    |
   *  Bit 1   |   | X |   |   |     |    |
   *  Bit 2   |   | X |   |   |     |    |
   *  Bit 3   |   | X |   |   |     |    |
   *  Bit 4   |   | X |   |   |     |    |
   *  Bit 5   |   | X |   |   |     |    |
   *  Bit 6   |   | X |   |   |     |    |
   *  Bit 7   |   | X |   |   |     |    |
   *  --------+---+---+---+---+-...-+----+
   *
   * So, in order to draw a white, horizontal line, at row 45. we
   * would have to modify all of the bytes in page 45/8 = 5.  We
   * would have to set bit 45%8 = 5 in every byte in the page.
   */

  fbmask  = 1 << (row & 7);
  fbptr   = &priv->fb[page * PCD8544_XRES + col];
#ifdef CONFIG_LCD_PACKEDMSFIRST
  usrmask = MS_BIT;
#else
  usrmask = LS_BIT;
#endif

  *buffer = 0;
  for (i = 0; i < pixlen; i++)
    {
      /* Set or clear the corresponding bit */

      uint8_t byte = *fbptr++;
      if ((byte & fbmask) != 0)
        {
          *buffer |= usrmask;
        }

      /* Inc/Decrement to the next destination pixel. Hmmmm. It looks like
       * this logic could write past the end of the user buffer.  Revisit
       * this!
       */

#ifdef CONFIG_LCD_PACKEDMSFIRST
      if (usrmask == LS_BIT)
        {
          buffer++;
         *buffer = 0;
          usrmask = MS_BIT;
        }
      else
        {
          usrmask >>= 1;
        }
#else
      if (usrmask == MS_BIT)
        {
          buffer++;
         *buffer = 0;
          usrmask = LS_BIT;
        }
      else
        {
          usrmask <<= 1;
        }
#endif
    }

  return OK;
}

/****************************************************************************
 * Name:  pcd8544_getvideoinfo
 *
 * Description:
 *   Get information about the LCD video controller configuration.
 *
 ****************************************************************************/

static int pcd8544_getvideoinfo(FAR struct lcd_dev_s *dev,
                              FAR struct fb_videoinfo_s *vinfo)
{
  DEBUGASSERT(dev && vinfo);
  ginfo("fmt: %d xres: %d yres: %d nplanes: %d\n",
         g_videoinfo.fmt, g_videoinfo.xres, g_videoinfo.yres,
        g_videoinfo.nplanes);
  memcpy(vinfo, &g_videoinfo, sizeof(struct fb_videoinfo_s));
  return OK;
}

/****************************************************************************
 * Name:  pcd8544_getplaneinfo
 *
 * Description:
 *   Get information about the configuration of each LCD color plane.
 *
 ****************************************************************************/

static int pcd8544_getplaneinfo(FAR struct lcd_dev_s *dev,
                                unsigned int planeno,
                                FAR struct lcd_planeinfo_s *pinfo)
{
  DEBUGASSERT(dev && pinfo && planeno == 0);
  ginfo("planeno: %d bpp: %d\n", planeno, g_planeinfo.bpp);
  memcpy(pinfo, &g_planeinfo, sizeof(struct lcd_planeinfo_s));
  pinfo->dev = dev;
  return OK;
}

/****************************************************************************
 * Name:  pcd8544_getpower
 *
 * Description:
 *   Get the LCD panel power status (0: full off - CONFIG_LCD_MAXPOWER: full
 *   on). On backlit LCDs, this setting may correspond to the backlight
 *   setting.
 *
 ****************************************************************************/

static int pcd8544_getpower(struct lcd_dev_s *dev)
{
  struct pcd8544_dev_s *priv = (struct pcd8544_dev_s *)dev;
  DEBUGASSERT(priv);
  ginfo("powered: %s\n", pcd8544_powerstring(priv->powered));
  return priv->powered;
}

/****************************************************************************
 * Name:  pcd8544_setpower
 *
 * Description:
 *   Enable/disable LCD panel power (0: full off - CONFIG_LCD_MAXPOWER: full
 *   on). On backlit LCDs, this setting may correspond to the backlight
 *   setting.
 *
 ****************************************************************************/

static int pcd8544_setpower(struct lcd_dev_s *dev, int power)
{
  struct pcd8544_dev_s *priv = (struct pcd8544_dev_s *)dev;

  DEBUGASSERT(priv && (unsigned)power <= CONFIG_LCD_MAXPOWER);
  ginfo("power: %s powered: %s\n",
        pcd8544_powerstring(power), pcd8544_powerstring(priv->powered));

  /* Select and lock the device */

  pcd8544_select(priv->spi);

  /* Select command transfer */

  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), true);

  if (power <= PCD8544_POWER_OFF)
    {
      /* Turn the display off (power-down) */

      SPI_SEND(priv->spi, (PCD8544_FUNC_SET | PCD8544_POWER_DOWN));

      priv->powered = PCD8544_POWER_OFF;
    }
  else
    {
      /* Leave the power-down */

      SPI_SEND(priv->spi, PCD8544_FUNC_SET);

      priv->powered = PCD8544_POWER_ON;
    }

  /* Select data transfer */

  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), false);

  /* Let go of the SPI lock and de-select the device */

  pcd8544_deselect(priv->spi);

  return OK;
}

/****************************************************************************
 * Name:  pcd8544_getcontrast
 *
 * Description:
 *   Get the current contrast setting (0-CONFIG_LCD_MAXCONTRAST).
 *
 ****************************************************************************/

static int pcd8544_getcontrast(struct lcd_dev_s *dev)
{
  struct pcd8544_dev_s *priv = (struct pcd8544_dev_s *)dev;
  DEBUGASSERT(priv);
  return (int)priv->contrast;
}

/****************************************************************************
 * Name:  pcd8544_setcontrast
 *
 * Description:
 *   Set LCD panel contrast (0-CONFIG_LCD_MAXCONTRAST).
 *
 ****************************************************************************/

static int pcd8544_setcontrast(struct lcd_dev_s *dev, unsigned int contrast)
{
  struct pcd8544_dev_s *priv = (struct pcd8544_dev_s *)dev;

  ginfo("contrast: %d\n", contrast);
  DEBUGASSERT(priv);

  if (contrast > 127)
    {
      return -EINVAL;
    }

  /* Save the contrast */

  priv->contrast = contrast;

  /* Select and lock the device */

  pcd8544_select(priv->spi);

  /* Select command transfer */

  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), true);

  /* Select the extended instruction set ( H = 1 ) */

  SPI_SEND(priv->spi, (PCD8544_FUNC_SET | PCD8544_MODEH));

  /* Set the contrast */

  SPI_SEND(priv->spi, (PCD8544_WRITE_VOP | contrast));

  /* Return to normal mode */

  SPI_SEND(priv->spi, PCD8544_FUNC_SET);

  /* Select data transfer */

  SPI_CMDDATA(priv->spi, SPIDEV_DISPLAY(0), false);

  /* Let go of the SPI lock and de-select the device */

  pcd8544_deselect(priv->spi);

  return OK;
}

/****************************************************************************
 * Name:  up_clear
 *
 * Description:
 *   Clear the display.
 *
 ****************************************************************************/

static inline void up_clear(FAR struct pcd8544_dev_s  *priv)
{
  FAR struct spi_dev_s *spi  = priv->spi;
  int page;
  int i;

  /* Clear the framebuffer */

  memset(priv->fb, PCD8544_Y1_BLACK, PCD8544_FBSIZE);

  /* Select and lock the device */

  pcd8544_select(priv->spi);

  /* Go throw pcd8544 all 6 pages */

  for (page = 0, i = 0; i < 6; i++)
    {
      /* Select command transfer */

      SPI_CMDDATA(spi, SPIDEV_DISPLAY(0), true);

      /* Set the starting position for the run */

      SPI_SEND(priv->spi, PCD8544_SET_Y_ADDR + i);    /* Set the page start */
      SPI_SEND(priv->spi, PCD8544_SET_X_ADDR + page); /* Set the column */

      /* Select data transfer */

      SPI_CMDDATA(spi, SPIDEV_DISPLAY(0), false);

      /* Then transfer all 84 columns of data */

      SPI_SNDBLOCK(priv->spi, &priv->fb[page * PCD8544_XRES], PCD8544_XRES);
    }

  /* Unlock and de-select the device */

  pcd8544_deselect(spi);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  pcd8544_initialize
 *
 * Description:
 *   Initialize the PCD8544 video hardware.  The initial state of the
 *   OLED is fully initialized, display memory cleared, and the OLED ready to
 *   use, but with the power setting at 0 (full off == sleep mode).
 *
 * Input Parameters:
 *
 *   spi - A reference to the SPI driver instance.
 *   devno - A value in the range of 0 thropcd8544h
 *          CONFIG_PCD8544_NINTERFACES-1. This allows support for multiple
 *          OLED devices.
 *
 * Returned Value:
 *
 *   On success, this function returns a reference to the LCD object for the
 *   specified OLED.  NULL is returned on any failure.
 *
 ****************************************************************************/

FAR struct lcd_dev_s *pcd8544_initialize(FAR struct spi_dev_s *spi,
                                         unsigned int devno)
{
  /* Configure and enable LCD */

  FAR struct pcd8544_dev_s  *priv = &g_pcd8544dev;

  ginfo("Initializing\n");
  DEBUGASSERT(spi && devno == 0);

  /* Save the reference to the SPI device */

  priv->spi = spi;

  /* Select and lock the device */

  pcd8544_select(spi);

  /* Select command transfer */

  SPI_CMDDATA(spi, SPIDEV_DISPLAY(0), true);

  /* Leave the power-down and select extended instruction set mode H = 1 */

  SPI_SEND(spi, (PCD8544_FUNC_SET | PCD8544_MODEH));

  /* Set LCD Bias to n = 3 */

  SPI_SEND(spi, (PCD8544_BIAS_SYSTEM | PCD8544_BIAS_BS2));

  /* Select the normal instruction set mode H = 0 */

  SPI_SEND(spi, PCD8544_FUNC_SET);

  /* Clear the screen */

  SPI_SEND(spi, (PCD8544_DISP_CTRL | PCD8544_DISP_BLANK));

  /* Set the Display Control to Normal Mode D = 1 and E = 0 */

  SPI_SEND(spi, (PCD8544_DISP_CTRL | PCD8544_DISP_NORMAL));

  /* Select data transfer */

  SPI_CMDDATA(spi, SPIDEV_DISPLAY(0), false);

  /* Let go of the SPI lock and de-select the device */

  pcd8544_deselect(spi);

  /* Clear the framebuffer */

  up_mdelay(100);
  up_clear(priv);
  return &priv->dev;
}
