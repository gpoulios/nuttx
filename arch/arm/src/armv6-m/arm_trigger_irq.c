/****************************************************************************
 * arch/arm/src/armv6-m/arm_trigger_irq.c
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

#include <stdint.h>
#include <assert.h>

#include <nuttx/arch.h>
#include <arch/irq.h>

#include "arm_internal.h"
#include "nvic.h"

#ifdef CONFIG_ARCH_HAVE_IRQTRIGGER

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_trigger_irq
 *
 * Description:
 *   Trigger an IRQ by software.
 *
 ****************************************************************************/

void up_trigger_irq(int irq, cpu_set_t cpuset)
{
  uint32_t pend_bit = 0;

  DEBUGASSERT(irq >= NVIC_IRQ_NMI && irq < NR_IRQS);

  if (irq >= NVIC_IRQ_FIRST)
    {
      putreg32(irq - NVIC_IRQ_FIRST, ARMV6M_NVIC2_BASE);
    }
  else
    {
      switch (irq)
        {
          case NVIC_IRQ_PENDSV:
            pend_bit = SYSCON_ICSR_PENDSVSET;
            break;

          case NVIC_IRQ_NMI:
            pend_bit = SYSCON_ICSR_NMIPENDSET;
            break;

          case NVIC_IRQ_SYSTICK:
            pend_bit = SYSCON_ICSR_PENDSTSET;
            break;

          default:
            break;
        }

      if (pend_bit)
        {
          modifyreg32(ARMV6M_SYSCON_ICSR, 0, pend_bit);
        }
    }
}

#endif /* CONFIG_ARCH_HAVE_IRQTRIGGER */
