/****************************************************************************
 * arch/arm/src/arm/lpc2378/lpc23xx_head.S
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2010 Rommel Marcelo. All rights reserved.
 * SPDX-FileCopyrightText: 2014 Gregory Nutt. All rights reserved.
 * SPDX-FileCopyrightText: 2010,2012 Gregory Nutt. All rights reserved.
 * SPDX-FileContributor: Rommel Marcelo
 * SPDX-FileContributor: Gregory Nutt <gnutt@nuttx.org>
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <arch/board/board.h>

#include "arm.h"
#include "lpc2378.h"
#include "lpc23xx_uart.h"
#include "lpc23xx_scb.h"
#include "lpc23xx_pinsel.h"

/****************************************************************************
 * Macros
 ****************************************************************************/

/* Print a character on the UART to show boot status. This macro will
 * modify r0, r1, r2 and r14
 */

#ifdef CONFIG_DEBUG_FEATURES
	.macro	showprogress, code
	mov	r0, #\code
	bl	arm_lowputc
	.endm
#else
	.macro	showprogress, code
	.endm
#endif

/****************************************************************************
 * Text
 ****************************************************************************/

	.text

/****************************************************************************
 * Name: _vector_table
 *
 * Description:
 *   Interrupt vector table.  This must be located at the beginning
 *   of the memory space (at CONFIG_LPC2378_CODE_BASE).  The first entry in
 *   the vector table is the reset vector and this is the code that
 *   will execute when the processor is reset.
 *
 ****************************************************************************/

	.globl	_vector_table
	.type	_vector_table, %function
_vector_table:
	ldr	pc, .Lresethandler		/* 0x00: Reset */
	ldr	pc, .Lundefinedhandler		/* 0x04: Undefined instruction */
	ldr	pc, .Lswihandler		/* 0x08: Software interrupt */
	ldr	pc, .Lprefetchaborthandler	/* 0x0c: Prefetch abort */
	ldr	pc, .Ldataaborthandler		/* 0x10: Data abort */
	.long	0xB8A06F58			/* 0x14: Vector checksum */
	ldr	pc, .Lirqhandler		/* 0x18: IRQ */
	ldr	pc, .Lfiqhandler		/* 0x1c: FIQ */

	.globl	__start
	.globl	arm_vectorundefinsn
	.globl	arm_vectorsvc
	.globl	arm_vectorprefetch
	.globl	arm_vectordata
	.globl	arm_vectorirq
	.globl	arm_vectorfiq

.Lresethandler:
	.long	__start
.Lundefinedhandler:
	.long	arm_vectorundefinsn
.Lswihandler:
	.long	arm_vectorsvc
.Lprefetchaborthandler:
	.long	arm_vectorprefetch
.Ldataaborthandler:
	.long	arm_vectordata
.Lirqhandler:
	.long	arm_vectorirq
.Lfiqhandler:
	.long	arm_vectorfiq
	.size	_vector_table, . - _vector_table

/****************************************************************************
 * OS Entry Point
 ****************************************************************************/

/* We assume the bootloader has already initialized most of the h/w for
 * us and that only leaves us having to do some os specific things
 * below.
 */
	.text
	.globl __start
	.type	__start, #function
__start:

	/* Call lowlevel init C-function */

	.extern configure_pll
	ldr	r0, =configure_pll
	mov	lr, pc
	bx	r0

	/* First, setup initial processor mode */

	mov	r0, #(PSR_MODE_SYS | PSR_I_BIT | PSR_F_BIT )
	msr	cpsr, r0

/* Configure the uart so that we can get debug output as soon
 * as possible.  Modifies r0, r1, r2, and r14.
 */
	bl	up_lowsetup

	showprogress 'A'

	/* Setup system stack (and get the BSS range) */

	adr	r0, LC0
	ldmia	r0, {r4, r5, sp}

	/* Clear system BSS section (Initialize with 0) */

	mov	r0, #0
1:	cmp	r4, r5
	strcc	r0, [r4], #4
	bcc	1b

	showprogress 'B'

	/* Copy system .data sections to new home in RAM. */

	adr	r3, LC2
	ldmia	r3, {r0, r1, r2}

2:	ldmia	r0!, {r3 - r10}
	stmia	r1!, {r3 - r10}
	cmp	r1, r2
	blt	2b

	/* Perform early serial initialization */

	mov	fp, #0
#ifdef USE_EARLYSERIALINIT
	bl	arm_earlyserialinit
	showprogress 'S'
#endif

	showprogress 'C'
	showprogress '\n'

	/* Initialize onboard LEDs */

#ifdef CONFIG_ARCH_LEDS
	bl	board_autoled_initialize
#endif

	/* Then jump to OS entry */

	b	nx_start

	/* Variables:
	 * _sbss is the start of the BSS region (see ld.script)
	 * _ebss is the end of the BSS region (see ld.script)
	 * The idle task stack starts at the end of BSS and is
	 * of size CONFIG_IDLETHREAD_STACKSIZE.  The heap continues
	 * from there until the end of memory.  See g_idle_topstack
	 * below.
	 */

LC0:	.long	_sbss
	.long	_ebss
	.long	_ebss+CONFIG_IDLETHREAD_STACKSIZE

LC2:	.long	_eronly	/* Where .data defaults are stored in FLASH */
	.long	_sdata	/* Where .data needs to reside in SDRAM */
	.long	_edata
	.size	__start, .-__start

	/* This global variable is unsigned long g_idle_topstack and is
	 * exported from here only because of its coupling to LCO
	 * above.
	 */

	.data
	.align	4
	.globl	g_idle_topstack
	.type	g_idle_topstack, object
g_idle_topstack:
	.long	_ebss+CONFIG_IDLETHREAD_STACKSIZE
	.size	g_idle_topstack, .-g_idle_topstack

	.end
