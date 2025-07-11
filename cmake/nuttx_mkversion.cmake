# ##############################################################################
# cmake/nuttx_mkversion.cmake
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

set(VERSION_H ${CMAKE_BINARY_DIR}/include/nuttx/version.h)
if(EXISTS ${VERSION_H})
  return()
endif()

execute_process(
  COMMAND git -C ${NUTTX_DIR} describe --match "nuttx-*"
  WORKING_DIRECTORY ${NUTTX_DIR}
  ERROR_VARIABLE NUTTX_ERROR
  OUTPUT_VARIABLE NUTTX_VERSION
  RESULT_VARIABLE VERSION_STATUS
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(${VERSION_STATUS} AND NOT ${VERSION_STATUS} EQUAL 0)
  set(NUTTX_VERSION "0.0.0")
else()
  string(REPLACE "-" ";" NUTTX_VERSION ${NUTTX_VERSION})
  list(GET NUTTX_VERSION 1 NUTTX_VERSION)
  string(STRIP "${NUTTX_VERSION}" NUTTX_VERSION)
endif()

string(REPLACE "." ";" NUTTX_CHECK_VERSION ${NUTTX_VERSION})
list(LENGTH NUTTX_CHECK_VERSION NUTTX_VERSION_LENGTH)
if(${NUTTX_VERSION_LENGTH} LESS 3)
  execute_process(
    COMMAND git -C ${NUTTX_DIR} -c "versionsort.suffix=-" tag --sort=v:refname
    WORKING_DIRECTORY ${NUTTX_DIR}
    OUTPUT_VARIABLE NUTTX_VERSION_LIST
    RESULT_VARIABLE VERSION_STATUS)

  if(${VERSION_STATUS} EQUAL 0)
    string(REPLACE "\n" ";" NUTTX_VERSION_LIST ${NUTTX_VERSION_LIST})
    foreach(version ${NUTTX_VERSION_LIST})
      string(REGEX MATCH "nuttx-[0-9]+\.[0-9]+\.[0-9]+" NUTTX_CHECK_VERSION
                   ${version})
      if(NUTTX_CHECK_VERSION)
        string(REPLACE "-" ";" NUTTX_VERSION ${NUTTX_CHECK_VERSION})
        list(GET NUTTX_VERSION 1 NUTTX_VERSION)
      endif()
    endforeach()
  endif()
endif()

execute_process(
  COMMAND git -C ${NUTTX_DIR} log --oneline -1
  WORKING_DIRECTORY ${NUTTX_DIR}
  OUTPUT_VARIABLE NUTTX_VERSION_BUILD
  RESULT_VARIABLE VERSION_STATUS)

if(${VERSION_STATUS} AND NOT ${VERSION_STATUS} EQUAL 0)
  set(NUTTX_VERSION_BUILD)
else()
  string(REPLACE " " ";" NUTTX_VERSION_BUILD ${NUTTX_VERSION_BUILD})
  list(GET NUTTX_VERSION_BUILD 0 NUTTX_VERSION_BUILD)

  execute_process(
    COMMAND git -C ${NUTTX_DIR} diff-index --name-only HEAD
    WORKING_DIRECTORY ${NUTTX_DIR}
    OUTPUT_VARIABLE NUTTX_VERSION_BUILD_DIRTY)

  if(NUTTX_VERSION_BUILD_DIRTY)
    set(NUTTX_VERSION_BUILD ${NUTTX_VERSION_BUILD}-dirty)
  endif()
endif()

file(WRITE ${VERSION_H} "/* version.h -- Autogenerated! Do not edit. */\n\n")

file(APPEND ${VERSION_H} "#ifndef __INCLUDE_NUTTX_VERSION_H\n")
file(APPEND ${VERSION_H} "#define __INCLUDE_NUTTX_VERSION_H\n\n")
file(APPEND ${VERSION_H} "#define CONFIG_VERSION_STRING \"${NUTTX_VERSION}\"\n")

string(REPLACE "." ";" NUTTX_VERSION ${NUTTX_VERSION})

list(GET NUTTX_VERSION 0 NUTTX_VERSION_MAJOR)
list(GET NUTTX_VERSION 1 NUTTX_VERSION_MINOR)
list(GET NUTTX_VERSION 2 NUTTX_VERSION_PATCH)

file(APPEND ${VERSION_H}
     "#define CONFIG_VERSION_MAJOR ${NUTTX_VERSION_MAJOR}\n")
file(APPEND ${VERSION_H}
     "#define CONFIG_VERSION_MINOR ${NUTTX_VERSION_MINOR}\n")
file(APPEND ${VERSION_H}
     "#define CONFIG_VERSION_PATCH ${NUTTX_VERSION_PATCH}\n")
file(APPEND ${VERSION_H}
     "#define CONFIG_VERSION_BUILD \"${NUTTX_VERSION_BUILD}\"\n\n")

file(
  APPEND ${VERSION_H}
  "#define CONFIG_VERSION ((CONFIG_VERSION_MAJOR << 16) | \\
                        (CONFIG_VERSION_MINOR << 8)  | \\
                        (CONFIG_VERSION_PATCH))\n")

file(APPEND ${VERSION_H} "\n#endif /* __INCLUDE_NUTTX_VERSION_H */\n")
