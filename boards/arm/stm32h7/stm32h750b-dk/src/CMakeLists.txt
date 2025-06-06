# ##############################################################################
# boards/arm/stm32h7/stm32h750b-dk/src/CMakeLists.txt
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

set(SRCS stm32_boot.c stm32_bringup.c)

if(CONFIG_ARCH_LEDS)
  list(APPEND SRCS stm32_autoleds.c)
else()
  list(APPEND SRCS stm32_userleds.c)
endif()

if(CONFIG_STM32H7_OTGFS)
  list(APPEND SRCS stm32_usb.c)
endif()

if(CONFIG_BOARDCTL)
  list(APPEND SRCS stm32_appinitialize.c)
endif()

if(CONFIG_TESTING_OSTEST)
  list(APPEND SRCS stm32_ostest.c)
endif()

if(CONFIG_BOARDCTL_RESET)
  list(APPEND SRCS stm32_reset.c)
endif()

if(CONFIG_INPUT_FT5X06)
  list(APPEND SRCS stm32_ft5x06.c)
endif()

if(CONFIG_STM32H7_LTDC)
  list(APPEND SRCS stm32_lcd.c)
endif()

target_sources(board PRIVATE ${SRCS})

if(CONFIG_ARCH_CHIP_STM32H7_CORTEXM7)
  set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
else()
  set_property(GLOBAL PROPERTY LD_SCRIPT
                               "${NUTTX_BOARD_DIR}/scripts/flash_m4.ld")
endif()
