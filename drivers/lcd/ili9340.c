/****************************************************************************
 * drivers/lcd/ili9340.c
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

/* References: ILI9340_DS_V1.10.pdf (Rev: 1.10), "a-Si TFT LCD Single Chip
 *             Driver 240RGBx320 Resolution and 262K color",
 *             ILI TECHNOLOGY CORP., www.ilitek.com.
 */

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
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/ili9340.h>

#include <arch/irq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* This is the generic lcd driver interface for the ili9340 Single Chip LCD
 * driver. The driver supports multiple displays, each connected with an own
 * ili9340 Single Chip LCD driver. The communication with the LCD single chip
 * driver must be provide by a subdriver accessible through the ili9340_dev_s
 * structure which is platform and MCU interface specific.
 *
 * Supported MCU interfaces (planned to support)
 *
 * Interface I
 *
 * 8080 MCU 8-bit bus interface
 * 8080 MCU 16-bit bus interface
 * 8080 MCU 9-bit bus interface
 * 8080 MCU 18-bit bus interface
 * 3-wire 9-bit data serial interface
 * 4-wire 8-bit data serial interface
 *
 * Interface II
 *
 * 8080 MCU 8-bit bus interface
 * 8080 MCU 16-bit bus interface
 * 8080 MCU 9-bit bus interface
 * 8080 MCU 18-bit bus interface
 * 3-wire 9-bit data serial Interface
 * 4-wire 8-bit data serial Interface
 *
 * Note! RGB interface will not supported by the lcd driver.
 * It should be use with the platform specific RGB graphic controller and the
 * nuttx framebuffer interface.
 *
 */

/* Fundamental command and parameter definition */

/* Interface control (IFCTL)
 *
 *
 * Parameter 1:  0x0001
 *
 * MY_EOR:  0
 * MX_EOR:  0
 * MV_EOR:  0
 * BGR_EOR: 0
 * WEMODE:  1   Reset column and page if data transfer exceeds
 */
#define ILI9340_IFCTL_MYEOR     0
#define ILI9340_IFCTL_MXEOR     0
#define ILI9340_IFCTL_MVEOR     0
#define ILI9340_IFCTL_BGREOR    0
#define ILI9340_IFCTL_WEMODE    ILI9340_INTERFACE_CONTROL_WEMODE

#define ILI9340_IFCTL_PARAM1    ILI9340_IFCTL_MYEOR | ILI9340_IFCTL_MXEOR | \
                                ILI9340_IFCTL_MVEOR | ILI9340_IFCTL_BGREOR | \
                                ILI9340_IFCTL_WEMODE

/* Parameter 2: 0x0000
 *
 * EPF:     0   65k color format for RGB interface, not used set to default
 * MDT:     0   Display data transfer mode, not used
 *
 * Note!
 * If RGB666 and 16-bit 8080-I interface supported by the driver, MDT must
 * be defined for each selected driver instance. Leave it empty for now.
 */

#define ILI9340_IFCTL_EPF       ILI9340_INTERFACE_CONTROL_EPF(0)
#define ILI9340_IFCTL_MDT       ILI9340_INTERFACE_CONTROL_MDT(0)

#define ILI9340_IFCTL_PARAM2    ILI9340_IFCTL_EPF | ILI9340_IFCTL_MDT

/* Parameter 3: 0x0000/0x0020
 *
 * ENDIAN:  0/1 Depending on endian mode of the mcu?
 * DM:      0   Internal clock operation
 * RM:      0   System interface/VSYNC interface
 * RIM:     0   RGB interface mode, unimportant set to default
 *
 */

#ifdef CONFIG_ENDIAN_BIG
#  define ILI9340_IFCTL_ENDIAN  0
#else
#  define ILI9340_IFCTL_ENDIAN  ILI9340_INTERFACE_CONTROL_ENDIAN
#endif
#define ILI9340_IFCTL_DM        ILI9340_INTERFACE_CONTROL_DM(0)
#define ILI9340_IFCTL_RM        0
#define ILI9340_IFCTL_RIM       0

#define ILI9340_IFCTL_PARAM3    ILI9340_IFCTL_RIM | ILI9340_IFCTL_RM | \
                                ILI9340_IFCTL_DM | ILI9340_IFCTL_ENDIAN

/* Memory access control (MADCTL) */

/* Landscape:   00100000 / 00101000 / h28
 *
 * MY:          0
 * MX:          0
 * MV:          1
 * ML:          0
 * BGR:         0/1 Depending on endian mode of the mcu?
 * MH:          0
 */

#define ILI9340_MADCTL_LANDSCAPE_MY     0
#define ILI9340_MADCTL_LANDSCAPE_MX     0
#define ILI9340_MADCTL_LANDSCAPE_MV     ILI9340_MEMORY_ACCESS_CONTROL_MV
#define ILI9340_MADCTL_LANDSCAPE_ML     0
#ifdef CONFIG_BIG_ENDIAN
#  define ILI9340_MADCTL_LANDSCAPE_BGR  0
#else
#  define ILI9340_MADCTL_LANDSCAPE_BGR  ILI9340_MEMORY_ACCESS_CONTROL_BGR
#endif
#define ILI9340_MADCTL_LANDSCAPE_MH     0

#define ILI9340_MADCTL_LANDSCAPE_PARAM1 (ILI9340_MADCTL_LANDSCAPE_MY | \
                                        ILI9340_MADCTL_LANDSCAPE_MX | \
                                        ILI9340_MADCTL_LANDSCAPE_MV | \
                                        ILI9340_MADCTL_LANDSCAPE_ML | \
                                        ILI9340_MADCTL_LANDSCAPE_BGR | \
                                        ILI9340_MADCTL_LANDSCAPE_MH)

/* Portrait:    00000000 / 00001000 / h08
 *
 * MY:          0
 * MX:          0
 * MV:          0
 * ML:          0
 * BGR:         0/1 Depending on endian mode of the mcu?
 * MH:          0
 */

#define ILI9340_MADCTL_PORTRAIT_MY      0
#define ILI9340_MADCTL_PORTRAIT_MX      ILI9340_MEMORY_ACCESS_CONTROL_MX
#define ILI9340_MADCTL_PORTRAIT_MV      0
#define ILI9340_MADCTL_PORTRAIT_ML      ILI9340_MEMORY_ACCESS_CONTROL_ML
#ifdef CONFIG_BIG_ENDIAN
#  define ILI9340_MADCTL_PORTRAIT_BGR   0
#else
#  define ILI9340_MADCTL_PORTRAIT_BGR   ILI9340_MEMORY_ACCESS_CONTROL_BGR
#endif
#define ILI9340_MADCTL_PORTRAIT_MH      0

#define ILI9340_MADCTL_PORTRAIT_PARAM1  (ILI9340_MADCTL_PORTRAIT_MY | \
                                        ILI9340_MADCTL_PORTRAIT_MX | \
                                        ILI9340_MADCTL_PORTRAIT_MV | \
                                        ILI9340_MADCTL_PORTRAIT_ML | \
                                        ILI9340_MADCTL_PORTRAIT_BGR | \
                                        ILI9340_MADCTL_PORTRAIT_MH)
/* RLandscape:  01100000 / 01101000 / h68
 *
 * MY:          0
 * MX:          1
 * MV:          1
 * ML:          0
 * BGR:         0/1 Depending on endian mode of the mcu?
 * MH:          0
 */

#define ILI9340_MADCTL_RLANDSCAPE_MY    ILI9340_MEMORY_ACCESS_CONTROL_MY
#define ILI9340_MADCTL_RLANDSCAPE_MX    ILI9340_MEMORY_ACCESS_CONTROL_MX
#define ILI9340_MADCTL_RLANDSCAPE_MV    ILI9340_MEMORY_ACCESS_CONTROL_MV
#define ILI9340_MADCTL_RLANDSCAPE_ML    0
#ifdef CONFIG_BIG_ENDIAN
#  define ILI9340_MADCTL_RLANDSCAPE_BGR 0
#else
#  define ILI9340_MADCTL_RLANDSCAPE_BGR ILI9340_MEMORY_ACCESS_CONTROL_BGR
#endif
#define ILI9340_MADCTL_RLANDSCAPE_MH    0

#define ILI9340_MADCTL_RLANDSCAPE_PARAM1 \
                                        (ILI9340_MADCTL_RLANDSCAPE_MY | \
                                        ILI9340_MADCTL_RLANDSCAPE_MX | \
                                        ILI9340_MADCTL_RLANDSCAPE_MV | \
                                        ILI9340_MADCTL_RLANDSCAPE_ML | \
                                        ILI9340_MADCTL_RLANDSCAPE_BGR | \
                                        ILI9340_MADCTL_RLANDSCAPE_MH)

/* RPortrait:   11000000 / 11001000 / hc8
 *
 * MY:          1
 * MX:          1
 * MV:          0
 * ML:          0
 * BGR:         0/1 Depending on endian mode of the mcu?
 * MH:          0
 *
 */

#define ILI9340_MADCTL_RPORTRAIT_MY     ILI9340_MEMORY_ACCESS_CONTROL_MY
#define ILI9340_MADCTL_RPORTRAIT_MX     0
#define ILI9340_MADCTL_RPORTRAIT_MV     0
#define ILI9340_MADCTL_RPORTRAIT_ML     ILI9340_MEMORY_ACCESS_CONTROL_ML
#ifdef CONFIG_BIG_ENDIAN
#  define ILI9340_MADCTL_RPORTRAIT_BGR  0
#else
#  define ILI9340_MADCTL_RPORTRAIT_BGR  ILI9340_MEMORY_ACCESS_CONTROL_BGR
#endif
#define ILI9340_MADCTL_RPORTRAIT_MH     0

#define ILI9340_MADCTL_RPORTRAIT_PARAM1 (ILI9340_MADCTL_RPORTRAIT_MY | \
                                        ILI9340_MADCTL_RPORTRAIT_MX | \
                                        ILI9340_MADCTL_RPORTRAIT_MV | \
                                        ILI9340_MADCTL_RPORTRAIT_ML | \
                                        ILI9340_MADCTL_RPORTRAIT_BGR | \
                                        ILI9340_MADCTL_RPORTRAIT_MH)

/* Pixel Format Set (COLMOD)
 *
 * Note! RGB interface settings (DPI) is unimportant for the MCU interface
 * mode but set the register to the defined state equal to the MCU interface
 * pixel format.
 *
 * 16 Bit MCU:  01010101 / h55
 *
 * DPI:         5 (RGB16-565 RGB interface, not used)
 * DBI:         5 (RGB16-565 MCU interface
 */

#define ILI9340_PIXSET_16BITDPI         ILI9340_PIXEL_FORMAT_SET_DPI(5)
#define ILI9340_PIXSET_16BITDBI         ILI9340_PIXEL_FORMAT_SET_DBI(5)

#define ILI9340_PIXSET_16BITMCU_PARAM1  (ILI9340_PIXSET_16BITDPI | \
                                        ILI9340_PIXSET_16BITDBI)

/* 18-bit MCU:  01100110 / h66 (not supported by nuttx until now)
 *
 * DPI:         6  (RGB18-666 RGB interface)
 * DBI:         6  (RGB18-666 MCU interface)
 */

#define ILI9340_PIXSET_18BITDPI         ILI9340_PIXEL_FORMAT_SET_DPI(6)
#define ILI9340_PIXSET_18BITDBI         ILI9340_PIXEL_FORMAT_SET_DBI(6)

#define ILI9340_PIXSET_18BITMCU_PARAM1  (ILI9340_PIXSET_18BITDPI | \
                                        ILI9340_PIXSET_18BITDBI)

/* General fix display resolution */

#define ILI9340_XRES           240
#define ILI9340_YRES           320

/* Validate configuration */

#if CONFIG_LCD_ILI9340_NINTERFACES < 1
#  undef CONFIG_LCD_ILI9340_IFACE0
#elif CONFIG_LCD_ILI9340_NINTERFACES < 2
#  undef CONFIG_LCD_ILI9340_IFACE1
#endif

/* First LCD display */

#ifdef CONFIG_LCD_ILI9340_IFACE0
#  if defined(CONFIG_LCD_ILI9340_IFACE0_LANDSCAPE)
#    define ILI9340_IFACE0_ORIENT     ILI9340_MADCTL_LANDSCAPE_PARAM1
#    define ILI9340_IFACE0_STRIDE     ILI9340_YRES
#  elif defined(CONFIG_LCD_ILI9340_IFACE0_PORTRAIT)
#    define ILI9340_IFACE0_ORIENT     ILI9340_MADCTL_PORTRAIT_PARAM1
#    define ILI9340_IFACE0_STRIDE     ILI9340_XRES
#  elif defined(CONFIG_LCD_ILI9340_IFACE0_RLANDSCAPE)
#    define ILI9340_IFACE0_ORIENT     ILI9340_MADCTL_RLANDSCAPE_PARAM1
#    define ILI9340_IFACE0_STRIDE     ILI9340_YRES
#  elif defined(CONFIG_LCD_ILI9340_IFACE0_RPORTRAIT)
#    define ILI9340_IFACE0_ORIENT     ILI9340_MADCTL_RPORTRAIT_PARAM1
#    define ILI9340_IFACE0_STRIDE     ILI9340_XRES
#  endif
#  ifdef CONFIG_LCD_ILI9340_IFACE0_RGB565
#    define ILI9340_IFACE0_PXFMT      FB_FMT_RGB16_565
#    define ILI9340_IFACE0_BPP        16
#    define ILI9340_IFACE0_BUFFER     ILI9340_IFACE0_STRIDE
#  else
#    error "undefined pixel format for lcd interface 0"
#  endif
#endif

/* Second LCD display */

#ifdef CONFIG_LCD_ILI9340_IFACE1
#  ifdef CONFIG_LCD_ILI9340_IFACE1_LANDSCAPE
#    define ILI9340_IFACE1_ORIENT     ILI9340_MADCTL_LANDSCAPE_PARAM1
#    define ILI9340_IFACE1_STRIDE     ILI9340_YRES
#  elif CONFIG_LCD_ILI9340_IFACE1_PORTRAIT
#    define ILI9340_IFACE1_ORIENT     ILI9340_MADCTL_PORTRAIT_PARAM1
#    define ILI9340_IFACE1_STRIDE     ILI9340_XRES
#  elif CONFIG_LCD_ILI9340_IFACE1_RLANDSCAPE
#    define ILI9340_IFACE1_ORIENT     ILI9340_MADCTL_RLANDSCAPE_PARAM1
#    define ILI9340_IFACE1_STRIDE     ILI9340_YRES
#  elif CONFIG_LCD_ILI9340_IFACE1_RPORTRAIT
#    define ILI9340_IFACE1_ORIENT     ILI9340_MADCTL_RPORTRAIT_PARAM1
#    define ILI9340_IFACE1_STRIDE     ILI9340_XRES
#  endif
#  ifdef CONFIG_LCD_ILI9340_IFACE1_RGB565
#    define ILI9340_IFACE1_PXFMT      FB_FMT_RGB16_565
#    define ILI9340_IFACE1_BPP        16
#    define ILI9340_IFACE1_BUFFER     ILI9340_IFACE1_STRIDE
#  else
#    error "undefined pixel format for lcd interface 1"
#  endif
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Each single connected ili9340 LCD driver needs an own driver instance
 * to provide a unique getrun and putrun method. Also store fundamental
 * parameter in driver internal structure. This minimal overhead should be
 * acceptable.
 */

struct ili9340_dev_s
{
  /* Publicly visible device structure */

  struct lcd_dev_s dev;

  /* Private driver-specific information follows */

  FAR struct ili9340_lcd_s *lcd;

  /* Run buffer for the device */

  uint16_t *runbuffer;

  /* Display orientation, e.g. Landscape, Portrait */

  uint8_t orient;

  /* LCD driver pixel format */

  uint8_t pxfmt;

  /* LCD driver color depth */

  uint8_t bpp;

  /* Current power state of the device */

  uint8_t power;
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

/* Internal low level helpers */

static inline uint16_t ili9340_getxres(FAR struct ili9340_dev_s *dev);
static inline uint16_t ili9340_getyres(FAR struct ili9340_dev_s *dev);

/* lcd data transfer methods */

static int ili9340_putrun(FAR struct lcd_dev_s *dev, fb_coord_t row,
                          fb_coord_t col, FAR const uint8_t *buffer,
                          size_t npixels);
#ifndef CONFIG_LCD_NOGETRUN
static int ili9340_getrun(FAR struct lcd_dev_s *dev, fb_coord_t row,
                          fb_coord_t col, FAR uint8_t *buffer,
                          size_t npixels);
#endif

/* lcd configuration */

static int ili9340_getvideoinfo(FAR struct lcd_dev_s *dev,
                                FAR struct fb_videoinfo_s *vinfo);
static int ili9340_getplaneinfo(FAR struct lcd_dev_s *dev,
                                unsigned int planeno,
                                FAR struct lcd_planeinfo_s *pinfo);

/* lcd specific controls */

static int ili9340_getpower(struct lcd_dev_s *dev);
static int ili9340_setpower(struct lcd_dev_s *dev, int power);
static int ili9340_getcontrast(struct lcd_dev_s *dev);
static int ili9340_setcontrast(struct lcd_dev_s *dev, unsigned int contrast);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Initialize driver instance 1 < LCD_ILI9340_NINTERFACES */

#ifdef CONFIG_LCD_ILI9340_IFACE0
static uint16_t g_runbuffer0[ILI9340_IFACE0_BUFFER];
#endif
#ifdef CONFIG_LCD_ILI9340_IFACE1
static uint16_t g_runbuffer1[ILI9340_IFACE1_BUFFER];
#endif

static struct ili9340_dev_s g_lcddev[CONFIG_LCD_ILI9340_NINTERFACES] =
{
#ifdef CONFIG_LCD_ILI9340_IFACE0
  {
    .lcd              = 0,
    .runbuffer        = g_runbuffer0,
    .orient           = ILI9340_IFACE0_ORIENT,
    .pxfmt            = ILI9340_IFACE0_PXFMT,
    .bpp              = ILI9340_IFACE0_BPP,
    .power            = 0,
  },
#endif
#ifdef CONFIG_LCD_ILI9340_IFACE1
  {
    .lcd              = 0,
    .runbuffer        = g_runbuffer1,
    .orient           = ILI9340_IFACE1_ORIENT,
    .pxfmt            = ILI9340_IFACE1_PXFMT,
    .bpp              = ILI9340_IFACE1_BPP,
    .power            = 0,
  },
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ili9340_getxres
 *
 * Description:
 *   Get horizontal resolution of the connected LCD driver depending on the
 *   configured display orientation.
 *
 * Parameters:
 *   dev   - Reference to private driver structure
 *
 * Return Value:
 *
 *   Horizontal resolution
 *
 ****************************************************************************/

static inline uint16_t ili9340_getxres(FAR struct ili9340_dev_s *dev)
{
  if (dev->orient == ILI9340_MADCTL_LANDSCAPE_PARAM1 ||
        dev->orient == ILI9340_MADCTL_RLANDSCAPE_PARAM1)
    {
          return ILI9340_YRES;
    }

  return ILI9340_XRES;
}

/****************************************************************************
 * Name:  ili9340_getyres
 *
 * Description:
 *   Get vertical resolution of the connected LCD driver depending on the
 *   configured display orientation.
 *
 * Parameter:
 *   dev   - Reference to private driver structure
 *
 * Return Value:
 *
 *   Vertical resolution
 *
 ****************************************************************************/

static inline uint16_t ili9340_getyres(FAR struct ili9340_dev_s *dev)
{
  if (dev->orient == ILI9340_MADCTL_LANDSCAPE_PARAM1 ||
        dev->orient == ILI9340_MADCTL_RLANDSCAPE_PARAM1)
    {
          return ILI9340_XRES;
    }

  return ILI9340_YRES;
}

/****************************************************************************
 * Name:  ili9340_selectarea
 *
 * Description:
 *   Select the active area for displaying pixel
 *
 * Parameter:
 *   lcd       - Reference to private driver structure
 *   x0        - Start x position
 *   y0        - Start y position
 *   x1        - End x position
 *   y1        - End y position
 *
 ****************************************************************************/

static void ili9340_selectarea(FAR struct ili9340_lcd_s *lcd,
                               uint16_t x0, uint16_t y0,
                               uint16_t x1, uint16_t y1)
{
  /* Select column */

  lcd->sendcmd(lcd, ILI9340_COLUMN_ADDRESS_SET);
  lcd->sendparam(lcd, (x0 >> 8));
  lcd->sendparam(lcd, (x0 & 0xff));
  lcd->sendparam(lcd, (x1 >> 8));
  lcd->sendparam(lcd, (x1 & 0xff));

  /* Select page */

  lcd->sendcmd(lcd, ILI9340_PAGE_ADDRESS_SET);
  lcd->sendparam(lcd, (y0 >> 8));
  lcd->sendparam(lcd, (y0 & 0xff));
  lcd->sendparam(lcd, (y1 >> 8));
  lcd->sendparam(lcd, (y1 & 0xff));
}

/****************************************************************************
 * Name:  ili9340_putrun
 *
 * Description:
 *   Write a partial raster line to the LCD.
 *
 * Parameters:
 *   lcd_dev - The lcd device
 *   row     - Starting row to write to (range: 0 <= row < yres)
 *   col     - Starting column to write to (range: 0 <= col <= xres-npixels)
 *   buffer  - The buffer containing the run to be written to the LCD
 *   npixels - The number of pixels to write to the
 *             (range: 0 < npixels <= xres-col)
 *
 * Returned Value:
 *
 *   On success - OK
 *   On error   - -EINVAL
 *
 ****************************************************************************/

static int ili9340_putrun(FAR struct lcd_dev_s *lcd_dev, fb_coord_t row,
                          fb_coord_t col, FAR const uint8_t *buffer,
                          size_t npixels)
{
  FAR struct ili9340_dev_s *dev = (FAR struct ili9340_dev_s *)lcd_dev;
  FAR struct ili9340_lcd_s *lcd = dev->lcd;
  FAR const uint16_t *src = (FAR const uint16_t *)buffer;

  DEBUGASSERT(buffer && ((uintptr_t)buffer & 1) == 0);

  /* Check if position outside of area */

  if (col + npixels > ili9340_getxres(dev) || row > ili9340_getyres(dev))
    {
      return -EINVAL;
    }

  /* Select lcd driver */

  lcd->select(lcd);

  /* Select column and area similar to the partial raster line */

  ili9340_selectarea(lcd, col, row, col + npixels - 1, row);

  /* Send memory write cmd */

  lcd->sendcmd(lcd, ILI9340_MEMORY_WRITE);

  /* Send pixel to gram */

  lcd->sendgram(lcd, src, npixels);

  /* Deselect the lcd driver */

  lcd->deselect(lcd);

  return OK;
}

/****************************************************************************
 * Name:  ili9340_getrun
 *
 * Description:
 *   Read a partial raster line from the LCD.
 *
 * Parameter:
 *   lcd_dev - The lcd device
 *   row     - Starting row to read from (range: 0 <= row < yres)
 *   col     - Starting column to read read (range: 0 <= col <= xres-npixels)
 *   buffer  - The buffer in which to return the run read from the LCD
 *   npixels - The number of pixels to read from the LCD
 *            (range: 0 < npixels <= xres-col)
 *
 * Returned Value:
 *
 *   On success - OK
 *   On error   - -EINVAL
 *
 ****************************************************************************/

# ifndef CONFIG_LCD_NOGETRUN
static int ili9340_getrun(FAR struct lcd_dev_s *lcd_dev, fb_coord_t row,
                          fb_coord_t col, FAR uint8_t *buffer,
                          size_t npixels)
{
  FAR struct ili9340_dev_s *dev = (FAR struct ili9340_dev_s *)lcd_dev;
  FAR struct ili9340_lcd_s *lcd = dev->lcd;
  FAR uint16_t *dest = (FAR uint16_t *)buffer;

  DEBUGASSERT(buffer && ((uintptr_t)buffer & 1) == 0);

  /* Check if position outside of area */

  if (col + npixels > ili9340_getxres(dev) || row > ili9340_getyres(dev))
    {
      return -EINVAL;
    }

  /* Select lcd driver */

  lcd->select(lcd);

  /* Select column and area similar to the partial raster line */

  ili9340_selectarea(lcd, col, row, col + npixels - 1, row);

  /* Send memory read cmd */

  lcd->sendcmd(lcd, ILI9340_MEMORY_READ);

  /* Receive pixel to gram */

  lcd->recvgram(lcd, dest, npixels);

  /* Deselect the lcd driver */

  lcd->deselect(lcd);

  return OK;
}
#endif

/****************************************************************************
 * Name:  ili9340_hwinitialize
 *
 * Description:
 *   Initialize and configure the ILI9340 LCD driver hardware.
 *
 * Parameter:
 *   dev - A reference to the driver specific structure
 *
 * Returned Value:
 *
 *   On success - OK
 *   On error - EINVAL
 *
 ****************************************************************************/

static int ili9340_hwinitialize(FAR struct ili9340_dev_s *dev)
{
#ifdef CONFIG_DEBUG_LCD_INFO
  uint8_t param;
#endif
  FAR struct ili9340_lcd_s *lcd = dev->lcd;

  /* Select spi device */

  lcdinfo("Initialize lcd driver\n");
  lcd->select(lcd);

#ifdef CONFIG_DEBUG_LCD_INFO
  /* Read display identification */

  lcd->sendcmd(lcd, ILI9340_READ_ID1);
  lcd->recvparam(lcd, &param);
  lcdinfo("ili9340 LCD driver: LCD modules manufacturer ID: %d\n", param);

  lcd->sendcmd(lcd, ILI9340_READ_ID2);
  lcd->recvparam(lcd, &param);
  lcdinfo("ili9340 LCD driver: LCD modules driver version ID: %d\n", param);

  lcd->sendcmd(lcd, ILI9340_READ_ID3);
  lcd->recvparam(lcd, &param);
  lcdinfo("ili9340 LCD driver: LCD modules driver ID: %d\n", param);
#endif

  /* Reset the lcd display to the default state */

  lcdinfo("ili9340 LCD driver: Software Reset\n");
  lcd->sendcmd(lcd, ILI9340_SOFTWARE_RESET);
  up_mdelay(5);

  lcdinfo("ili9340 LCD driver: set Memory Access Control: %04x\n",
          dev->orient);
  lcd->sendcmd(lcd, ILI9340_MEMORY_ACCESS_CONTROL);
  lcd->sendparam(lcd, dev->orient);

  /* Select column and area */

  ili9340_selectarea(lcd, 0, 0, ILI9340_XRES, ILI9340_YRES);

  /* Pixel Format set */

  lcd->sendcmd(lcd, ILI9340_PIXEL_FORMAT_SET);

  /* 16 bit RGB565 */

  lcdinfo("ili9340 LCD driver: Set Pixel Format: %04x\n",
      ILI9340_PIXSET_16BITMCU_PARAM1);
  lcd->sendparam(lcd, ILI9340_PIXSET_16BITMCU_PARAM1);

  /* 18 bit RGB666, add settings here */

  lcdinfo("ili9340 LCD driver: Set Interface control\n");
  lcd->sendcmd(lcd, ILI9340_INTERFACE_CONTROL);
  lcd->sendparam(lcd, ILI9340_IFCTL_PARAM1);
  lcd->sendparam(lcd, ILI9340_IFCTL_PARAM2);
  lcd->sendparam(lcd, ILI9340_IFCTL_PARAM3);

  /* Sleep out */

  lcdinfo("ili9340 LCD driver: Sleep Out\n");
  lcd->sendcmd(lcd, ILI9340_SLEEP_OUT);
  up_mdelay(120);

  /* Deselect the device */

  lcd->deselect(lcd);

  /* Switch display off */

  ili9340_setpower(&dev->dev, 0);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ili9340_getvideoinfo
 *
 * Description:
 *   Get information about the LCD video controller configuration.
 *
 * Parameter:
 *   dev - A reference to the driver specific structure
 *   vinfo - A reference to the videoinfo structure
 *
 * Returned Value:
 *
 *  On success - OK
 *  On error   - -EINVAL
 *
 ****************************************************************************/

static int ili9340_getvideoinfo(FAR struct lcd_dev_s *dev,
                               FAR struct fb_videoinfo_s *vinfo)
{
  if (dev && vinfo)
    {
      FAR struct ili9340_dev_s *priv = (FAR struct ili9340_dev_s *)dev;

      vinfo->fmt = priv->pxfmt;
      vinfo->xres = ili9340_getxres(priv);
      vinfo->yres = ili9340_getyres(priv);
      vinfo->nplanes = 1;

      lcdinfo("fmt: %d xres: %d yres: %d nplanes: %d\n",
      vinfo->fmt, vinfo->xres, vinfo->yres, vinfo->nplanes);

      return OK;
    }

  return -EINVAL;
}

/****************************************************************************
 * Name:  ili9340_getplaneinfo
 *
 * Description:
 *   Get information about the configuration of each LCD color plane.
 *
 * Parameter:
 *   dev     - A reference to the driver specific structure
 *   planeno - The plane number
 *   pinfo   - A reference to the planeinfo structure
 *
 * Returned Value:
 *
 *  On success - OK
 *  On error   - -EINVAL
 *
 ****************************************************************************/

static int ili9340_getplaneinfo(FAR struct lcd_dev_s *dev,
                                unsigned int planeno,
                                FAR struct lcd_planeinfo_s *pinfo)
{
  if (dev && pinfo && planeno == 0)
    {
      FAR struct ili9340_dev_s *priv = (FAR struct ili9340_dev_s *)dev;

      pinfo->putrun = ili9340_putrun;
#ifndef CONFIG_LCD_NOGETRUN
      pinfo->getrun = ili9340_getrun;
#endif
      pinfo->bpp    = priv->bpp;
      pinfo->buffer = (FAR uint8_t *)priv->runbuffer;  /* Run scratch buffer */
      pinfo->dev    = dev;

      lcdinfo("planeno: %d bpp: %d\n", planeno, pinfo->bpp);

      return OK;
    }

  return -EINVAL;
}

/****************************************************************************
 * Name:  ili9340_getpower
 *
 * Description:
 *  Get the LCD panel power status
 *  (0: full off - CONFIG_LCD_MAXPOWER: full on.
 *  On backlit LCDs, this setting may correspond to the backlight setting.
 *
 * Parameter:
 *  dev     - A reference to the driver specific structure
 *
 * Returned Value:
 *
 *  On success - OK
 *  On error   - -EINVAL
 *
 ****************************************************************************/

static int ili9340_getpower(FAR struct lcd_dev_s *dev)
{
  FAR struct ili9340_dev_s *priv = (FAR struct ili9340_dev_s *)dev;

  if (priv)
    {
      lcdinfo("%d\n", priv->power);

      return priv->power;
    }

  return -EINVAL;
}

/****************************************************************************
 * Name:  ili9340_setpower
 *
 * Description:
 *  Enable/disable LCD panel power
 *  (0: full off - CONFIG_LCD_MAXPOWER: full on).
 *  On backlight LCDs, this setting may correspond to the backlight setting.
 *
 * Parameter:
 *  dev   - A reference to the driver specific structure
 *  power - Value of the power
 *
 * Returned Value:
 *
 *  On success - OK
 *  On error   - -EINVAL
 *
 ****************************************************************************/

static int ili9340_setpower(FAR struct lcd_dev_s *dev, int power)
{
  FAR struct ili9340_dev_s *priv = (FAR struct ili9340_dev_s *)dev;
  FAR struct ili9340_lcd_s *lcd  = priv->lcd;

  if (dev)
    {
      lcdinfo("%d\n", power);

      lcd->select(lcd);

      if (power > 0)
        {
          /* Set backlight level */

          lcd->backlight(lcd, power);

          /* And switch LCD on */

          lcd->sendcmd(lcd, ILI9340_DISPLAY_ON);
          up_mdelay(120);
        }
      else
        {
          /* Switch LCD off */

          lcd->sendcmd(lcd, ILI9340_DISPLAY_OFF);
        }

      lcd->deselect(lcd);

      priv->power = power;

      return OK;
    }

  return -EINVAL;
}

/****************************************************************************
 * Name:  ili9340_getcontrast
 *
 * Description:
 *   Get the current contrast setting (0-CONFIG_LCD_MAXCONTRAST).
 *
 * Parameter:
 *   dev   - A reference to the lcd driver structure
 *
 * Returned Value:
 *
 *  On success - current contrast value
 *  On error   - -ENOSYS, not supported by the ili9340.
 *
 ****************************************************************************/

static int ili9340_getcontrast(struct lcd_dev_s *dev)
{
  lcdinfo("Not implemented\n");
  return -ENOSYS;
}

/****************************************************************************
 * Name:  ili9340_setcontrast
 *
 * Description:
 *   Set LCD panel contrast (0-CONFIG_LCD_MAXCONTRAST).
 *
 * Parameter:
 *   dev   - A reference to the lcd driver structure
 *
 * Returned Value:
 *
 *  On success - OK
 *  On error   - -ENOSYS, not supported by the ili9340.
 *
 ****************************************************************************/

static int ili9340_setcontrast(struct lcd_dev_s *dev, unsigned int contrast)
{
  lcdinfo("contrast: %d\n", contrast);
  return -ENOSYS;
}

/****************************************************************************
 * Name:  ili9340_initialize
 *
 * Description:
 *  Initialize the LCD video driver internal structure. Also initialize the
 *  lcd hardware if not done. The control of the LCD driver is depend on the
 *  selected MCU interface and part of the platform specific subdriver (see
 *  config/stm32f429i-disco/src/stm32_ili93404ws.c)
 *
 * Input Parameters:
 *
 *  lcd - A reference to the platform specific driver instance to control the
 *    ili9340 display driver.
 *  devno - A value in the range of 0 through CONFIG_ILI9340_NINTERFACES-1.
 *    This allows support for multiple LCD devices.
 *
 * Returned Value:
 *
 *  On success, this function returns a reference to the LCD driver object
 *  for the specified LCD driver. NULL is returned on any failure.
 *
 ****************************************************************************/

FAR struct lcd_dev_s *ili9340_initialize(
                        FAR struct ili9340_lcd_s *lcd, int devno)
{
  if (lcd && devno >= 0 && devno < CONFIG_LCD_ILI9340_NINTERFACES)
    {
      FAR struct ili9340_dev_s *priv = &g_lcddev[devno];

      /* Check if initialized */

      if (!priv->lcd)
        {
          FAR struct lcd_dev_s *dev = &priv->dev;
          int   ret;

          /* Initialize internal structure */

          dev->getvideoinfo = ili9340_getvideoinfo;
          dev->getplaneinfo = ili9340_getplaneinfo;
          dev->getpower     = ili9340_getpower;
          dev->setpower     = ili9340_setpower;
          dev->getcontrast  = ili9340_getcontrast;
          dev->setcontrast  = ili9340_setcontrast;
          priv->lcd         = lcd;

          /* Initialize the LCD driver */

          ret = ili9340_hwinitialize(priv);

          if (ret == OK)
            {
              return &priv->dev;
            }
        }
    }

  return NULL;
}

/****************************************************************************
 * Name:  ili9340_clear
 *
 * Description:
 *  This is a non-standard LCD interface.  Because of the various rotations,
 *  clearing the display in the normal way by writing a sequences of runs
 *  that covers the entire display can be very slow. Here the display is
 *  cleared by simply setting all GRAM memory to the specified color.
 *
 * Parameter:
 *  dev   - A reference to the lcd driver structure
 *  color - The background color
 *
 * Returned Value:
 *
 *  On success - OK
 *  On error   - -EINVAL
 *
 ****************************************************************************/

int ili9340_clear(FAR struct lcd_dev_s *dev, uint16_t color)
{
  FAR struct ili9340_dev_s *priv = (FAR struct ili9340_dev_s *)dev;
  FAR struct ili9340_lcd_s *lcd = priv->lcd;
  uint16_t xres = ili9340_getxres(priv);
  uint16_t yres = ili9340_getyres(priv);
  uint32_t n;

  if (!lcd)
    {
      return -EINVAL;
    }

  /* Select lcd driver */

  lcd->select(lcd);

  /* Select column and area similar to the visible area */

  ili9340_selectarea(lcd, 0, 0, xres, yres);

  /* Send memory write cmd */

  lcd->sendcmd(lcd, ILI9340_MEMORY_WRITE);

  /* clear the visible area */

  for (n = 0; n < xres * yres; n++)
    {
      /* Send pixel to gram */

      lcd->sendgram(lcd, &color, 1);
    }

  /* Deselect the lcd driver */

  lcd->deselect(lcd);

  return OK;
}
