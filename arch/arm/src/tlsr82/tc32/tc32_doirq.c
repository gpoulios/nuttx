/****************************************************************************
 * arch/arm/src/tlsr82/tc32/tc32_doirq.c
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

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/compiler.h>
#include <nuttx/board.h>

#include <stdint.h>
#include <assert.h>
#include <debug.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "sched/sched.h"
#include "hardware/tlsr82_irq.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

uint32_t *arm_doirq(int irq, uint32_t *regs)
{
  struct tcb_s *tcb = this_task();

  board_autoled_on(LED_INIRQ);
#ifdef CONFIG_SUPPRESS_INTERRUPTS
  PANIC();
#else

  /* Nested interrupts are not supported */

  DEBUGASSERT(!up_interrupt_context());

  tcb->xcp.regs = regs;

  /* Acknowledge the interrupt */

  arm_ack_irq(irq);

  /* Deliver the IRQ */

  irq_dispatch(irq, regs);
  tcb = this_task();

  if (regs != tcb->xcp.regs)
    {
      regs = tcb->xcp.regs;
    }

#endif

  board_autoled_off(LED_INIRQ);
  return regs;
}

/****************************************************************************
 * Name: tc32_getirq
 *
 * Description:
 *   This function is used to get the interrupt number based on the
 *   interrupt source flag register.
 *
 * Parameters:
 *   void
 *
 * Return Value:
 *   [0, NR_IRQS-1] - On success, found interrupt number, success.
 *   NR_IRQS        - Error, invalid interrupt number.
 *
 ****************************************************************************/

static int locate_code(".ram_code") tc32_getirq(void)
{
  int irq;
  uint32_t value;

  value = IRQ_SRC_REG & IRQ_MASK_REG;

  if ((value & BIT(NR_RF_IRQ)) != 0)
    {
      return NR_RF_IRQ;
    }
  else if ((value & BIT(NR_SYSTEM_TIMER_IRQ)) != 0)
    {
      return NR_SYSTEM_TIMER_IRQ;
    }

  if (value == 0)
    {
      irq = NR_IRQS;
    }
  else
    {
      irq = __builtin_ctz(value);
    }

  return irq;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_ack_irq
 *
 * Description:
 *   Acknowledge the IRQ
 *
 ****************************************************************************/

void locate_code(".ram_code") arm_ack_irq(int irq)
{
}

/****************************************************************************
 * Name: irq_handler
 *
 * Description:
 *   This function is the common interrupt handler for all interrupts.
 *
 * Parameters:
 *   uint32_t *regs - the saved context array pointer of interrpted task,
 *                    size = XCPTCONTEXT_REGS.
 *
 * Return Value:
 *   uint32_t *regs - if occur context switch, regs = the saved context
 *                    array pointer of next task, if not, regs = input regs,
 *                    size = XCPTCONTEXT_REGS.
 *
 ****************************************************************************/

uint32_t * locate_code(".ram_code") irq_handler(uint32_t *regs)
{
  int irq = tc32_getirq();

  return arm_doirq(irq, regs);
}

