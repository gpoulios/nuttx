/****************************************************************************
 * arch/arm/src/efm32/hardware/efm32_letimer.h
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2014 Silicon Laboratories, Inc.
 * SPDX-FileCopyrightText: 2014 Pierre-noel Bouteville . All rights reserved.
 * SPDX-FileCopyrightText: 2014 Gregory Nutt. All rights reserved.
 * SPDX-FileContributor: Pierre-noel Bouteville <pnb990@gmail.com>
 * SPDX-FileContributor: Gregory Nutt <gnutt@nuttx.org>
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of
 * any kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties
 * against infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_EFM32_HARDWARE_EFM32_LETIMER_H
#define __ARCH_ARM_SRC_EFM32_HARDWARE_EFM32_LETIMER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/efm32_memorymap.h"

#if !defined(CONFIG_EFM32_EFM32GG)
#  warning This is the EFM32GG header file; Review/modification needed for this architecture
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LETIMER Register Offsets *************************************************/

#define EFM32_LETIMER_CTRL_OFFSET            0x0000  /* Control Register */
#define EFM32_LETIMER_CMD_OFFSET             0x0004  /* Command Register */
#define EFM32_LETIMER_STATUS_OFFSET          0x0008  /* Status Register */
#define EFM32_LETIMER_CNT_OFFSET             0x000c  /* Counter Value Register */
#define EFM32_LETIMER_COMP0_OFFSET           0x0010  /* Compare Value Register 0 */
#define EFM32_LETIMER_COMP1_OFFSET           0x0014  /* Compare Value Register 1 */
#define EFM32_LETIMER_REP0_OFFSET            0x0018  /* Repeat Counter Register 0 */
#define EFM32_LETIMER_REP1_OFFSET            0x001c  /* Repeat Counter Register 1 */
#define EFM32_LETIMER_IF_OFFSET              0x0020  /* Interrupt Flag Register */
#define EFM32_LETIMER_IFS_OFFSET             0x0024  /* Interrupt Flag Set Register */
#define EFM32_LETIMER_IFC_OFFSET             0x0028  /* Interrupt Flag Clear Register */
#define EFM32_LETIMER_IEN_OFFSET             0x002c  /* Interrupt Enable Register */
#define EFM32_LETIMER_FREEZE_OFFSET          0x0030  /* Freeze Register */
#define EFM32_LETIMER_SYNCBUSY_OFFSET        0x0034  /* Synchronization Busy Register */
#define EFM32_LETIMER_ROUTE_OFFSET           0x0040  /* I/O Routing Register */

/* LETIMER Register Addresses ***********************************************/

#define EFM32_LETIMER0_CTRL                  (EFM32_LETIMER0_BASE+EFM32_LETIMER0_CTRL_OFFSET)
#define EFM32_LETIMER0_CMD                   (EFM32_LETIMER0_BASE+EFM32_LETIMER0_CMD_OFFSET)
#define EFM32_LETIMER0_STATUS                (EFM32_LETIMER0_BASE+EFM32_LETIMER0_STATUS_OFFSET)
#define EFM32_LETIMER0_CNT                   (EFM32_LETIMER0_BASE+EFM32_LETIMER0_CNT_OFFSET)
#define EFM32_LETIMER0_COMP0                 (EFM32_LETIMER0_BASE+EFM32_LETIMER0_COMP0_OFFSET)
#define EFM32_LETIMER0_COMP1                 (EFM32_LETIMER0_BASE+EFM32_LETIMER0_COMP1_OFFSET)
#define EFM32_LETIMER0_REP0                  (EFM32_LETIMER0_BASE+EFM32_LETIMER0_REP0_OFFSET)
#define EFM32_LETIMER0_REP1                  (EFM32_LETIMER0_BASE+EFM32_LETIMER0_REP1_OFFSET)
#define EFM32_LETIMER0_IF                    (EFM32_LETIMER0_BASE+EFM32_LETIMER0_IF_OFFSET)
#define EFM32_LETIMER0_IFS                   (EFM32_LETIMER0_BASE+EFM32_LETIMER0_IFS_OFFSET)
#define EFM32_LETIMER0_IFC                   (EFM32_LETIMER0_BASE+EFM32_LETIMER0_IFC_OFFSET)
#define EFM32_LETIMER0_IEN                   (EFM32_LETIMER0_BASE+EFM32_LETIMER0_IEN_OFFSET)
#define EFM32_LETIMER0_FREEZE                (EFM32_LETIMER0_BASE+EFM32_LETIMER0_FREEZE_OFFSET)
#define EFM32_LETIMER0_SYNCBUSY              (EFM32_LETIMER0_BASE+EFM32_LETIMER0_SYNCBUSY_OFFSET)
#define EFM32_LETIMER0_ROUTE                 (EFM32_LETIMER0_BASE+EFM32_LETIMER0_ROUTE_OFFSET)

/* LETIMER Register Bit Field Definitions ***********************************/

/* Bit fields for LETIMER CTRL */

#define _LETIMER_CTRL_RESETVALUE             0x00000000UL                           /* Default value for LETIMER_CTRL */
#define _LETIMER_CTRL_MASK                   0x00001FFFUL                           /* Mask for LETIMER_CTRL */

#define _LETIMER_CTRL_REPMODE_SHIFT          0                                      /* Shift value for LETIMER_REPMODE */
#define _LETIMER_CTRL_REPMODE_MASK           0x3UL                                  /* Bit mask for LETIMER_REPMODE */
#define _LETIMER_CTRL_REPMODE_DEFAULT        0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_FREE           0x00000000UL                           /* Mode FREE for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_ONESHOT        0x00000001UL                           /* Mode ONESHOT for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_BUFFERED       0x00000002UL                           /* Mode BUFFERED for LETIMER_CTRL */
#define _LETIMER_CTRL_REPMODE_DOUBLE         0x00000003UL                           /* Mode DOUBLE for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_DEFAULT         (_LETIMER_CTRL_REPMODE_DEFAULT << 0)   /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_FREE            (_LETIMER_CTRL_REPMODE_FREE << 0)      /* Shifted mode FREE for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_ONESHOT         (_LETIMER_CTRL_REPMODE_ONESHOT << 0)   /* Shifted mode ONESHOT for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_BUFFERED        (_LETIMER_CTRL_REPMODE_BUFFERED << 0)  /* Shifted mode BUFFERED for LETIMER_CTRL */
#define LETIMER_CTRL_REPMODE_DOUBLE          (_LETIMER_CTRL_REPMODE_DOUBLE << 0)    /* Shifted mode DOUBLE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_SHIFT            2                                      /* Shift value for LETIMER_UFOA0 */
#define _LETIMER_CTRL_UFOA0_MASK             0xCUL                                  /* Bit mask for LETIMER_UFOA0 */
#define _LETIMER_CTRL_UFOA0_DEFAULT          0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_NONE             0x00000000UL                           /* Mode NONE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_TOGGLE           0x00000001UL                           /* Mode TOGGLE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_PULSE            0x00000002UL                           /* Mode PULSE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA0_PWM              0x00000003UL                           /* Mode PWM for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_DEFAULT           (_LETIMER_CTRL_UFOA0_DEFAULT << 2)     /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_NONE              (_LETIMER_CTRL_UFOA0_NONE << 2)        /* Shifted mode NONE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_TOGGLE            (_LETIMER_CTRL_UFOA0_TOGGLE << 2)      /* Shifted mode TOGGLE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_PULSE             (_LETIMER_CTRL_UFOA0_PULSE << 2)       /* Shifted mode PULSE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA0_PWM               (_LETIMER_CTRL_UFOA0_PWM << 2)         /* Shifted mode PWM for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_SHIFT            4                                      /* Shift value for LETIMER_UFOA1 */
#define _LETIMER_CTRL_UFOA1_MASK             0x30UL                                 /* Bit mask for LETIMER_UFOA1 */
#define _LETIMER_CTRL_UFOA1_DEFAULT          0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_NONE             0x00000000UL                           /* Mode NONE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_TOGGLE           0x00000001UL                           /* Mode TOGGLE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_PULSE            0x00000002UL                           /* Mode PULSE for LETIMER_CTRL */
#define _LETIMER_CTRL_UFOA1_PWM              0x00000003UL                           /* Mode PWM for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_DEFAULT           (_LETIMER_CTRL_UFOA1_DEFAULT << 4)     /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_NONE              (_LETIMER_CTRL_UFOA1_NONE << 4)        /* Shifted mode NONE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_TOGGLE            (_LETIMER_CTRL_UFOA1_TOGGLE << 4)      /* Shifted mode TOGGLE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_PULSE             (_LETIMER_CTRL_UFOA1_PULSE << 4)       /* Shifted mode PULSE for LETIMER_CTRL */
#define LETIMER_CTRL_UFOA1_PWM               (_LETIMER_CTRL_UFOA1_PWM << 4)         /* Shifted mode PWM for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL0                   (0x1UL << 6)                           /* Output 0 Polarity */
#define _LETIMER_CTRL_OPOL0_SHIFT            6                                      /* Shift value for LETIMER_OPOL0 */
#define _LETIMER_CTRL_OPOL0_MASK             0x40UL                                 /* Bit mask for LETIMER_OPOL0 */
#define _LETIMER_CTRL_OPOL0_DEFAULT          0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL0_DEFAULT           (_LETIMER_CTRL_OPOL0_DEFAULT << 6)     /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL1                   (0x1UL << 7)                           /* Output 1 Polarity */
#define _LETIMER_CTRL_OPOL1_SHIFT            7                                      /* Shift value for LETIMER_OPOL1 */
#define _LETIMER_CTRL_OPOL1_MASK             0x80UL                                 /* Bit mask for LETIMER_OPOL1 */
#define _LETIMER_CTRL_OPOL1_DEFAULT          0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_OPOL1_DEFAULT           (_LETIMER_CTRL_OPOL1_DEFAULT << 7)     /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_BUFTOP                  (0x1UL << 8)                           /* Buffered Top */
#define _LETIMER_CTRL_BUFTOP_SHIFT           8                                      /* Shift value for LETIMER_BUFTOP */
#define _LETIMER_CTRL_BUFTOP_MASK            0x100UL                                /* Bit mask for LETIMER_BUFTOP */
#define _LETIMER_CTRL_BUFTOP_DEFAULT         0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_BUFTOP_DEFAULT          (_LETIMER_CTRL_BUFTOP_DEFAULT << 8)    /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_COMP0TOP                (0x1UL << 9)                           /* Compare Value 0 Is Top Value */
#define _LETIMER_CTRL_COMP0TOP_SHIFT         9                                      /* Shift value for LETIMER_COMP0TOP */
#define _LETIMER_CTRL_COMP0TOP_MASK          0x200UL                                /* Bit mask for LETIMER_COMP0TOP */
#define _LETIMER_CTRL_COMP0TOP_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_COMP0TOP_DEFAULT        (_LETIMER_CTRL_COMP0TOP_DEFAULT << 9)  /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_RTCC0TEN                (0x1UL << 10)                          /* RTC Compare 0 Trigger Enable */
#define _LETIMER_CTRL_RTCC0TEN_SHIFT         10                                     /* Shift value for LETIMER_RTCC0TEN */
#define _LETIMER_CTRL_RTCC0TEN_MASK          0x400UL                                /* Bit mask for LETIMER_RTCC0TEN */
#define _LETIMER_CTRL_RTCC0TEN_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_RTCC0TEN_DEFAULT        (_LETIMER_CTRL_RTCC0TEN_DEFAULT << 10) /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_RTCC1TEN                (0x1UL << 11)                          /* RTC Compare 1 Trigger Enable */
#define _LETIMER_CTRL_RTCC1TEN_SHIFT         11                                     /* Shift value for LETIMER_RTCC1TEN */
#define _LETIMER_CTRL_RTCC1TEN_MASK          0x800UL                                /* Bit mask for LETIMER_RTCC1TEN */
#define _LETIMER_CTRL_RTCC1TEN_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_RTCC1TEN_DEFAULT        (_LETIMER_CTRL_RTCC1TEN_DEFAULT << 11) /* Shifted mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_DEBUGRUN                (0x1UL << 12)                          /* Debug Mode Run Enable */
#define _LETIMER_CTRL_DEBUGRUN_SHIFT         12                                     /* Shift value for LETIMER_DEBUGRUN */
#define _LETIMER_CTRL_DEBUGRUN_MASK          0x1000UL                               /* Bit mask for LETIMER_DEBUGRUN */
#define _LETIMER_CTRL_DEBUGRUN_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_CTRL */
#define LETIMER_CTRL_DEBUGRUN_DEFAULT        (_LETIMER_CTRL_DEBUGRUN_DEFAULT << 12) /* Shifted mode DEFAULT for LETIMER_CTRL */

/* Bit fields for LETIMER CMD */

#define _LETIMER_CMD_RESETVALUE              0x00000000UL                      /* Default value for LETIMER_CMD */
#define _LETIMER_CMD_MASK                    0x0000001FUL                      /* Mask for LETIMER_CMD */

#define LETIMER_CMD_START                    (0x1UL << 0)                      /* Start LETIMER */
#define _LETIMER_CMD_START_SHIFT             0                                 /* Shift value for LETIMER_START */
#define _LETIMER_CMD_START_MASK              0x1UL                             /* Bit mask for LETIMER_START */
#define _LETIMER_CMD_START_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_START_DEFAULT            (_LETIMER_CMD_START_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_STOP                     (0x1UL << 1)                      /* Stop LETIMER */
#define _LETIMER_CMD_STOP_SHIFT              1                                 /* Shift value for LETIMER_STOP */
#define _LETIMER_CMD_STOP_MASK               0x2UL                             /* Bit mask for LETIMER_STOP */
#define _LETIMER_CMD_STOP_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_STOP_DEFAULT             (_LETIMER_CMD_STOP_DEFAULT << 1)  /* Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CLEAR                    (0x1UL << 2)                      /* Clear LETIMER */
#define _LETIMER_CMD_CLEAR_SHIFT             2                                 /* Shift value for LETIMER_CLEAR */
#define _LETIMER_CMD_CLEAR_MASK              0x4UL                             /* Bit mask for LETIMER_CLEAR */
#define _LETIMER_CMD_CLEAR_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CLEAR_DEFAULT            (_LETIMER_CMD_CLEAR_DEFAULT << 2) /* Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO0                     (0x1UL << 3)                      /* Clear Toggle Output 0 */
#define _LETIMER_CMD_CTO0_SHIFT              3                                 /* Shift value for LETIMER_CTO0 */
#define _LETIMER_CMD_CTO0_MASK               0x8UL                             /* Bit mask for LETIMER_CTO0 */
#define _LETIMER_CMD_CTO0_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO0_DEFAULT             (_LETIMER_CMD_CTO0_DEFAULT << 3)  /* Shifted mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO1                     (0x1UL << 4)                      /* Clear Toggle Output 1 */
#define _LETIMER_CMD_CTO1_SHIFT              4                                 /* Shift value for LETIMER_CTO1 */
#define _LETIMER_CMD_CTO1_MASK               0x10UL                            /* Bit mask for LETIMER_CTO1 */
#define _LETIMER_CMD_CTO1_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_CMD */
#define LETIMER_CMD_CTO1_DEFAULT             (_LETIMER_CMD_CTO1_DEFAULT << 4)  /* Shifted mode DEFAULT for LETIMER_CMD */

/* Bit fields for LETIMER STATUS */

#define _LETIMER_STATUS_RESETVALUE           0x00000000UL                           /* Default value for LETIMER_STATUS */
#define _LETIMER_STATUS_MASK                 0x00000001UL                           /* Mask for LETIMER_STATUS */

#define LETIMER_STATUS_RUNNING               (0x1UL << 0)                           /* LETIMER Running */
#define _LETIMER_STATUS_RUNNING_SHIFT        0                                      /* Shift value for LETIMER_RUNNING */
#define _LETIMER_STATUS_RUNNING_MASK         0x1UL                                  /* Bit mask for LETIMER_RUNNING */
#define _LETIMER_STATUS_RUNNING_DEFAULT      0x00000000UL                           /* Mode DEFAULT for LETIMER_STATUS */
#define LETIMER_STATUS_RUNNING_DEFAULT       (_LETIMER_STATUS_RUNNING_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_STATUS */

/* Bit fields for LETIMER CNT */

#define _LETIMER_CNT_RESETVALUE              0x00000000UL                    /* Default value for LETIMER_CNT */
#define _LETIMER_CNT_MASK                    0x0000FFFFUL                    /* Mask for LETIMER_CNT */

#define _LETIMER_CNT_CNT_SHIFT               0                               /* Shift value for LETIMER_CNT */
#define _LETIMER_CNT_CNT_MASK                0xFFFFUL                        /* Bit mask for LETIMER_CNT */
#define _LETIMER_CNT_CNT_DEFAULT             0x00000000UL                    /* Mode DEFAULT for LETIMER_CNT */
#define LETIMER_CNT_CNT_DEFAULT              (_LETIMER_CNT_CNT_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_CNT */

/* Bit fields for LETIMER COMP0 */

#define _LETIMER_COMP0_RESETVALUE            0x00000000UL                        /* Default value for LETIMER_COMP0 */
#define _LETIMER_COMP0_MASK                  0x0000FFFFUL                        /* Mask for LETIMER_COMP0 */

#define _LETIMER_COMP0_COMP0_SHIFT           0                                   /* Shift value for LETIMER_COMP0 */
#define _LETIMER_COMP0_COMP0_MASK            0xFFFFUL                            /* Bit mask for LETIMER_COMP0 */
#define _LETIMER_COMP0_COMP0_DEFAULT         0x00000000UL                        /* Mode DEFAULT for LETIMER_COMP0 */
#define LETIMER_COMP0_COMP0_DEFAULT          (_LETIMER_COMP0_COMP0_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_COMP0 */

/* Bit fields for LETIMER COMP1 */

#define _LETIMER_COMP1_RESETVALUE            0x00000000UL                        /* Default value for LETIMER_COMP1 */
#define _LETIMER_COMP1_MASK                  0x0000FFFFUL                        /* Mask for LETIMER_COMP1 */

#define _LETIMER_COMP1_COMP1_SHIFT           0                                   /* Shift value for LETIMER_COMP1 */
#define _LETIMER_COMP1_COMP1_MASK            0xFFFFUL                            /* Bit mask for LETIMER_COMP1 */
#define _LETIMER_COMP1_COMP1_DEFAULT         0x00000000UL                        /* Mode DEFAULT for LETIMER_COMP1 */
#define LETIMER_COMP1_COMP1_DEFAULT          (_LETIMER_COMP1_COMP1_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_COMP1 */

/* Bit fields for LETIMER REP0 */

#define _LETIMER_REP0_RESETVALUE             0x00000000UL                      /* Default value for LETIMER_REP0 */
#define _LETIMER_REP0_MASK                   0x000000FFUL                      /* Mask for LETIMER_REP0 */

#define _LETIMER_REP0_REP0_SHIFT             0                                 /* Shift value for LETIMER_REP0 */
#define _LETIMER_REP0_REP0_MASK              0xFFUL                            /* Bit mask for LETIMER_REP0 */
#define _LETIMER_REP0_REP0_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_REP0 */
#define LETIMER_REP0_REP0_DEFAULT            (_LETIMER_REP0_REP0_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_REP0 */

/* Bit fields for LETIMER REP1 */

#define _LETIMER_REP1_RESETVALUE             0x00000000UL                      /* Default value for LETIMER_REP1 */
#define _LETIMER_REP1_MASK                   0x000000FFUL                      /* Mask for LETIMER_REP1 */

#define _LETIMER_REP1_REP1_SHIFT             0                                 /* Shift value for LETIMER_REP1 */
#define _LETIMER_REP1_REP1_MASK              0xFFUL                            /* Bit mask for LETIMER_REP1 */
#define _LETIMER_REP1_REP1_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_REP1 */
#define LETIMER_REP1_REP1_DEFAULT            (_LETIMER_REP1_REP1_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_REP1 */

/* Bit fields for LETIMER IF */

#define _LETIMER_IF_RESETVALUE               0x00000000UL                     /* Default value for LETIMER_IF */
#define _LETIMER_IF_MASK                     0x0000001FUL                     /* Mask for LETIMER_IF */

#define LETIMER_IF_COMP0                     (0x1UL << 0)                     /* Compare Match 0 Interrupt Flag */
#define _LETIMER_IF_COMP0_SHIFT              0                                /* Shift value for LETIMER_COMP0 */
#define _LETIMER_IF_COMP0_MASK               0x1UL                            /* Bit mask for LETIMER_COMP0 */
#define _LETIMER_IF_COMP0_DEFAULT            0x00000000UL                     /* Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_COMP0_DEFAULT             (_LETIMER_IF_COMP0_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_COMP1                     (0x1UL << 1)                     /* Compare Match 1 Interrupt Flag */
#define _LETIMER_IF_COMP1_SHIFT              1                                /* Shift value for LETIMER_COMP1 */
#define _LETIMER_IF_COMP1_MASK               0x2UL                            /* Bit mask for LETIMER_COMP1 */
#define _LETIMER_IF_COMP1_DEFAULT            0x00000000UL                     /* Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_COMP1_DEFAULT             (_LETIMER_IF_COMP1_DEFAULT << 1) /* Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_UF                        (0x1UL << 2)                     /* Underflow Interrupt Flag */
#define _LETIMER_IF_UF_SHIFT                 2                                /* Shift value for LETIMER_UF */
#define _LETIMER_IF_UF_MASK                  0x4UL                            /* Bit mask for LETIMER_UF */
#define _LETIMER_IF_UF_DEFAULT               0x00000000UL                     /* Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_UF_DEFAULT                (_LETIMER_IF_UF_DEFAULT << 2)    /* Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP0                      (0x1UL << 3)                     /* Repeat Counter 0 Interrupt Flag */
#define _LETIMER_IF_REP0_SHIFT               3                                /* Shift value for LETIMER_REP0 */
#define _LETIMER_IF_REP0_MASK                0x8UL                            /* Bit mask for LETIMER_REP0 */
#define _LETIMER_IF_REP0_DEFAULT             0x00000000UL                     /* Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP0_DEFAULT              (_LETIMER_IF_REP0_DEFAULT << 3)  /* Shifted mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP1                      (0x1UL << 4)                     /* Repeat Counter 1 Interrupt Flag */
#define _LETIMER_IF_REP1_SHIFT               4                                /* Shift value for LETIMER_REP1 */
#define _LETIMER_IF_REP1_MASK                0x10UL                           /* Bit mask for LETIMER_REP1 */
#define _LETIMER_IF_REP1_DEFAULT             0x00000000UL                     /* Mode DEFAULT for LETIMER_IF */
#define LETIMER_IF_REP1_DEFAULT              (_LETIMER_IF_REP1_DEFAULT << 4)  /* Shifted mode DEFAULT for LETIMER_IF */

/* Bit fields for LETIMER IFS */

#define _LETIMER_IFS_RESETVALUE              0x00000000UL                      /* Default value for LETIMER_IFS */
#define _LETIMER_IFS_MASK                    0x0000001FUL                      /* Mask for LETIMER_IFS */

#define LETIMER_IFS_COMP0                    (0x1UL << 0)                      /* Set Compare Match 0 Interrupt Flag */
#define _LETIMER_IFS_COMP0_SHIFT             0                                 /* Shift value for LETIMER_COMP0 */
#define _LETIMER_IFS_COMP0_MASK              0x1UL                             /* Bit mask for LETIMER_COMP0 */
#define _LETIMER_IFS_COMP0_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_COMP0_DEFAULT            (_LETIMER_IFS_COMP0_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_COMP1                    (0x1UL << 1)                      /* Set Compare Match 1 Interrupt Flag */
#define _LETIMER_IFS_COMP1_SHIFT             1                                 /* Shift value for LETIMER_COMP1 */
#define _LETIMER_IFS_COMP1_MASK              0x2UL                             /* Bit mask for LETIMER_COMP1 */
#define _LETIMER_IFS_COMP1_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_COMP1_DEFAULT            (_LETIMER_IFS_COMP1_DEFAULT << 1) /* Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_UF                       (0x1UL << 2)                      /* Set Underflow Interrupt Flag */
#define _LETIMER_IFS_UF_SHIFT                2                                 /* Shift value for LETIMER_UF */
#define _LETIMER_IFS_UF_MASK                 0x4UL                             /* Bit mask for LETIMER_UF */
#define _LETIMER_IFS_UF_DEFAULT              0x00000000UL                      /* Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_UF_DEFAULT               (_LETIMER_IFS_UF_DEFAULT << 2)    /* Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP0                     (0x1UL << 3)                      /* Set Repeat Counter 0 Interrupt Flag */
#define _LETIMER_IFS_REP0_SHIFT              3                                 /* Shift value for LETIMER_REP0 */
#define _LETIMER_IFS_REP0_MASK               0x8UL                             /* Bit mask for LETIMER_REP0 */
#define _LETIMER_IFS_REP0_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP0_DEFAULT             (_LETIMER_IFS_REP0_DEFAULT << 3)  /* Shifted mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP1                     (0x1UL << 4)                      /* Set Repeat Counter 1 Interrupt Flag */
#define _LETIMER_IFS_REP1_SHIFT              4                                 /* Shift value for LETIMER_REP1 */
#define _LETIMER_IFS_REP1_MASK               0x10UL                            /* Bit mask for LETIMER_REP1 */
#define _LETIMER_IFS_REP1_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_IFS */
#define LETIMER_IFS_REP1_DEFAULT             (_LETIMER_IFS_REP1_DEFAULT << 4)  /* Shifted mode DEFAULT for LETIMER_IFS */

/* Bit fields for LETIMER IFC */

#define _LETIMER_IFC_RESETVALUE              0x00000000UL                      /* Default value for LETIMER_IFC */
#define _LETIMER_IFC_MASK                    0x0000001FUL                      /* Mask for LETIMER_IFC */

#define LETIMER_IFC_COMP0                    (0x1UL << 0)                      /* Clear Compare Match 0 Interrupt Flag */
#define _LETIMER_IFC_COMP0_SHIFT             0                                 /* Shift value for LETIMER_COMP0 */
#define _LETIMER_IFC_COMP0_MASK              0x1UL                             /* Bit mask for LETIMER_COMP0 */
#define _LETIMER_IFC_COMP0_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_COMP0_DEFAULT            (_LETIMER_IFC_COMP0_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_COMP1                    (0x1UL << 1)                      /* Clear Compare Match 1 Interrupt Flag */
#define _LETIMER_IFC_COMP1_SHIFT             1                                 /* Shift value for LETIMER_COMP1 */
#define _LETIMER_IFC_COMP1_MASK              0x2UL                             /* Bit mask for LETIMER_COMP1 */
#define _LETIMER_IFC_COMP1_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_COMP1_DEFAULT            (_LETIMER_IFC_COMP1_DEFAULT << 1) /* Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_UF                       (0x1UL << 2)                      /* Clear Underflow Interrupt Flag */
#define _LETIMER_IFC_UF_SHIFT                2                                 /* Shift value for LETIMER_UF */
#define _LETIMER_IFC_UF_MASK                 0x4UL                             /* Bit mask for LETIMER_UF */
#define _LETIMER_IFC_UF_DEFAULT              0x00000000UL                      /* Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_UF_DEFAULT               (_LETIMER_IFC_UF_DEFAULT << 2)    /* Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP0                     (0x1UL << 3)                      /* Clear Repeat Counter 0 Interrupt Flag */
#define _LETIMER_IFC_REP0_SHIFT              3                                 /* Shift value for LETIMER_REP0 */
#define _LETIMER_IFC_REP0_MASK               0x8UL                             /* Bit mask for LETIMER_REP0 */
#define _LETIMER_IFC_REP0_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP0_DEFAULT             (_LETIMER_IFC_REP0_DEFAULT << 3)  /* Shifted mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP1                     (0x1UL << 4)                      /* Clear Repeat Counter 1 Interrupt Flag */
#define _LETIMER_IFC_REP1_SHIFT              4                                 /* Shift value for LETIMER_REP1 */
#define _LETIMER_IFC_REP1_MASK               0x10UL                            /* Bit mask for LETIMER_REP1 */
#define _LETIMER_IFC_REP1_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_IFC */
#define LETIMER_IFC_REP1_DEFAULT             (_LETIMER_IFC_REP1_DEFAULT << 4)  /* Shifted mode DEFAULT for LETIMER_IFC */

/* Bit fields for LETIMER IEN */

#define _LETIMER_IEN_RESETVALUE              0x00000000UL                      /* Default value for LETIMER_IEN */
#define _LETIMER_IEN_MASK                    0x0000001FUL                      /* Mask for LETIMER_IEN */

#define LETIMER_IEN_COMP0                    (0x1UL << 0)                      /* Compare Match 0 Interrupt Enable */
#define _LETIMER_IEN_COMP0_SHIFT             0                                 /* Shift value for LETIMER_COMP0 */
#define _LETIMER_IEN_COMP0_MASK              0x1UL                             /* Bit mask for LETIMER_COMP0 */
#define _LETIMER_IEN_COMP0_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_COMP0_DEFAULT            (_LETIMER_IEN_COMP0_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_COMP1                    (0x1UL << 1)                      /* Compare Match 1 Interrupt Enable */
#define _LETIMER_IEN_COMP1_SHIFT             1                                 /* Shift value for LETIMER_COMP1 */
#define _LETIMER_IEN_COMP1_MASK              0x2UL                             /* Bit mask for LETIMER_COMP1 */
#define _LETIMER_IEN_COMP1_DEFAULT           0x00000000UL                      /* Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_COMP1_DEFAULT            (_LETIMER_IEN_COMP1_DEFAULT << 1) /* Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_UF                       (0x1UL << 2)                      /* Underflow Interrupt Enable */
#define _LETIMER_IEN_UF_SHIFT                2                                 /* Shift value for LETIMER_UF */
#define _LETIMER_IEN_UF_MASK                 0x4UL                             /* Bit mask for LETIMER_UF */
#define _LETIMER_IEN_UF_DEFAULT              0x00000000UL                      /* Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_UF_DEFAULT               (_LETIMER_IEN_UF_DEFAULT << 2)    /* Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP0                     (0x1UL << 3)                      /* Repeat Counter 0 Interrupt Enable */
#define _LETIMER_IEN_REP0_SHIFT              3                                 /* Shift value for LETIMER_REP0 */
#define _LETIMER_IEN_REP0_MASK               0x8UL                             /* Bit mask for LETIMER_REP0 */
#define _LETIMER_IEN_REP0_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP0_DEFAULT             (_LETIMER_IEN_REP0_DEFAULT << 3)  /* Shifted mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP1                     (0x1UL << 4)                      /* Repeat Counter 1 Interrupt Enable */
#define _LETIMER_IEN_REP1_SHIFT              4                                 /* Shift value for LETIMER_REP1 */
#define _LETIMER_IEN_REP1_MASK               0x10UL                            /* Bit mask for LETIMER_REP1 */
#define _LETIMER_IEN_REP1_DEFAULT            0x00000000UL                      /* Mode DEFAULT for LETIMER_IEN */
#define LETIMER_IEN_REP1_DEFAULT             (_LETIMER_IEN_REP1_DEFAULT << 4)  /* Shifted mode DEFAULT for LETIMER_IEN */

/* Bit fields for LETIMER FREEZE */

#define _LETIMER_FREEZE_RESETVALUE           0x00000000UL                             /* Default value for LETIMER_FREEZE */
#define _LETIMER_FREEZE_MASK                 0x00000001UL                             /* Mask for LETIMER_FREEZE */

#define LETIMER_FREEZE_REGFREEZE             (0x1UL << 0)                             /* Register Update Freeze */
#define _LETIMER_FREEZE_REGFREEZE_SHIFT      0                                        /* Shift value for LETIMER_REGFREEZE */
#define _LETIMER_FREEZE_REGFREEZE_MASK       0x1UL                                    /* Bit mask for LETIMER_REGFREEZE */
#define _LETIMER_FREEZE_REGFREEZE_DEFAULT    0x00000000UL                             /* Mode DEFAULT for LETIMER_FREEZE */
#define _LETIMER_FREEZE_REGFREEZE_UPDATE     0x00000000UL                             /* Mode UPDATE for LETIMER_FREEZE */
#define _LETIMER_FREEZE_REGFREEZE_FREEZE     0x00000001UL                             /* Mode FREEZE for LETIMER_FREEZE */
#define LETIMER_FREEZE_REGFREEZE_DEFAULT     (_LETIMER_FREEZE_REGFREEZE_DEFAULT << 0) /* Shifted mode DEFAULT for LETIMER_FREEZE */
#define LETIMER_FREEZE_REGFREEZE_UPDATE      (_LETIMER_FREEZE_REGFREEZE_UPDATE << 0)  /* Shifted mode UPDATE for LETIMER_FREEZE */
#define LETIMER_FREEZE_REGFREEZE_FREEZE      (_LETIMER_FREEZE_REGFREEZE_FREEZE << 0)  /* Shifted mode FREEZE for LETIMER_FREEZE */

/* Bit fields for LETIMER SYNCBUSY */

#define _LETIMER_SYNCBUSY_RESETVALUE         0x00000000UL                           /* Default value for LETIMER_SYNCBUSY */
#define _LETIMER_SYNCBUSY_MASK               0x0000003FUL                           /* Mask for LETIMER_SYNCBUSY */

#define LETIMER_SYNCBUSY_CTRL                (0x1UL << 0)                           /* CTRL Register Busy */
#define _LETIMER_SYNCBUSY_CTRL_SHIFT         0                                      /* Shift value for LETIMER_CTRL */
#define _LETIMER_SYNCBUSY_CTRL_MASK          0x1UL                                  /* Bit mask for LETIMER_CTRL */
#define _LETIMER_SYNCBUSY_CTRL_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_CTRL_DEFAULT        (_LETIMER_SYNCBUSY_CTRL_DEFAULT << 0)  /* Shifted mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_CMD                 (0x1UL << 1)                           /* CMD Register Busy */
#define _LETIMER_SYNCBUSY_CMD_SHIFT          1                                      /* Shift value for LETIMER_CMD */
#define _LETIMER_SYNCBUSY_CMD_MASK           0x2UL                                  /* Bit mask for LETIMER_CMD */
#define _LETIMER_SYNCBUSY_CMD_DEFAULT        0x00000000UL                           /* Mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_CMD_DEFAULT         (_LETIMER_SYNCBUSY_CMD_DEFAULT << 1)   /* Shifted mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_COMP0               (0x1UL << 2)                           /* COMP0 Register Busy */
#define _LETIMER_SYNCBUSY_COMP0_SHIFT        2                                      /* Shift value for LETIMER_COMP0 */
#define _LETIMER_SYNCBUSY_COMP0_MASK         0x4UL                                  /* Bit mask for LETIMER_COMP0 */
#define _LETIMER_SYNCBUSY_COMP0_DEFAULT      0x00000000UL                           /* Mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_COMP0_DEFAULT       (_LETIMER_SYNCBUSY_COMP0_DEFAULT << 2) /* Shifted mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_COMP1               (0x1UL << 3)                           /* COMP1 Register Busy */
#define _LETIMER_SYNCBUSY_COMP1_SHIFT        3                                      /* Shift value for LETIMER_COMP1 */
#define _LETIMER_SYNCBUSY_COMP1_MASK         0x8UL                                  /* Bit mask for LETIMER_COMP1 */
#define _LETIMER_SYNCBUSY_COMP1_DEFAULT      0x00000000UL                           /* Mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_COMP1_DEFAULT       (_LETIMER_SYNCBUSY_COMP1_DEFAULT << 3) /* Shifted mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_REP0                (0x1UL << 4)                           /* REP0 Register Busy */
#define _LETIMER_SYNCBUSY_REP0_SHIFT         4                                      /* Shift value for LETIMER_REP0 */
#define _LETIMER_SYNCBUSY_REP0_MASK          0x10UL                                 /* Bit mask for LETIMER_REP0 */
#define _LETIMER_SYNCBUSY_REP0_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_REP0_DEFAULT        (_LETIMER_SYNCBUSY_REP0_DEFAULT << 4)  /* Shifted mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_REP1                (0x1UL << 5)                           /* REP1 Register Busy */
#define _LETIMER_SYNCBUSY_REP1_SHIFT         5                                      /* Shift value for LETIMER_REP1 */
#define _LETIMER_SYNCBUSY_REP1_MASK          0x20UL                                 /* Bit mask for LETIMER_REP1 */
#define _LETIMER_SYNCBUSY_REP1_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_SYNCBUSY */
#define LETIMER_SYNCBUSY_REP1_DEFAULT        (_LETIMER_SYNCBUSY_REP1_DEFAULT << 5)  /* Shifted mode DEFAULT for LETIMER_SYNCBUSY */

/* Bit fields for LETIMER ROUTE */

#define _LETIMER_ROUTE_RESETVALUE            0x00000000UL                           /* Default value for LETIMER_ROUTE */
#define _LETIMER_ROUTE_MASK                  0x00000703UL                           /* Mask for LETIMER_ROUTE */

#define LETIMER_ROUTE_OUT0PEN                (0x1UL << 0)                           /* Output 0 Pin Enable */
#define _LETIMER_ROUTE_OUT0PEN_SHIFT         0                                      /* Shift value for LETIMER_OUT0PEN */
#define _LETIMER_ROUTE_OUT0PEN_MASK          0x1UL                                  /* Bit mask for LETIMER_OUT0PEN */
#define _LETIMER_ROUTE_OUT0PEN_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_ROUTE */
#define LETIMER_ROUTE_OUT0PEN_DEFAULT        (_LETIMER_ROUTE_OUT0PEN_DEFAULT << 0)  /* Shifted mode DEFAULT for LETIMER_ROUTE */
#define LETIMER_ROUTE_OUT1PEN                (0x1UL << 1)                           /* Output 1 Pin Enable */
#define _LETIMER_ROUTE_OUT1PEN_SHIFT         1                                      /* Shift value for LETIMER_OUT1PEN */
#define _LETIMER_ROUTE_OUT1PEN_MASK          0x2UL                                  /* Bit mask for LETIMER_OUT1PEN */
#define _LETIMER_ROUTE_OUT1PEN_DEFAULT       0x00000000UL                           /* Mode DEFAULT for LETIMER_ROUTE */
#define LETIMER_ROUTE_OUT1PEN_DEFAULT        (_LETIMER_ROUTE_OUT1PEN_DEFAULT << 1)  /* Shifted mode DEFAULT for LETIMER_ROUTE */
#define _LETIMER_ROUTE_LOCATION_SHIFT        8                                      /* Shift value for LETIMER_LOCATION */
#define _LETIMER_ROUTE_LOCATION_MASK         0x700UL                                /* Bit mask for LETIMER_LOCATION */
#define _LETIMER_ROUTE_LOCATION_LOC0         0x00000000UL                           /* Mode LOC0 for LETIMER_ROUTE */
#define _LETIMER_ROUTE_LOCATION_DEFAULT      0x00000000UL                           /* Mode DEFAULT for LETIMER_ROUTE */
#define _LETIMER_ROUTE_LOCATION_LOC1         0x00000001UL                           /* Mode LOC1 for LETIMER_ROUTE */
#define _LETIMER_ROUTE_LOCATION_LOC2         0x00000002UL                           /* Mode LOC2 for LETIMER_ROUTE */
#define _LETIMER_ROUTE_LOCATION_LOC3         0x00000003UL                           /* Mode LOC3 for LETIMER_ROUTE */
#define LETIMER_ROUTE_LOCATION_LOC0          (_LETIMER_ROUTE_LOCATION_LOC0 << 8)    /* Shifted mode LOC0 for LETIMER_ROUTE */
#define LETIMER_ROUTE_LOCATION_DEFAULT       (_LETIMER_ROUTE_LOCATION_DEFAULT << 8) /* Shifted mode DEFAULT for LETIMER_ROUTE */
#define LETIMER_ROUTE_LOCATION_LOC1          (_LETIMER_ROUTE_LOCATION_LOC1 << 8)    /* Shifted mode LOC1 for LETIMER_ROUTE */
#define LETIMER_ROUTE_LOCATION_LOC2          (_LETIMER_ROUTE_LOCATION_LOC2 << 8)    /* Shifted mode LOC2 for LETIMER_ROUTE */
#define LETIMER_ROUTE_LOCATION_LOC3          (_LETIMER_ROUTE_LOCATION_LOC3 << 8)    /* Shifted mode LOC3 for LETIMER_ROUTE */

#endif /* __ARCH_ARM_SRC_EFM32_HARDWARE_EFM32_LETIMER_H */
