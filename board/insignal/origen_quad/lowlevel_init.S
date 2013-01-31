/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Copyright (C) 2012 Insignal
 *
 * Lowlevel initialize for the Insignal OrigenQUAD board (Exynos4x12)
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <version.h>
#include <asm/arch/cpu.h>
#include "setup.h"

_TEXT_BASE:
	.word	CONFIG_SYS_TEXT_BASE

	.globl lowlevel_init
lowlevel_init:

	/* use iRAM stack in bl2 */
	ldr	sp, =0x02060000
	stmdb	r13!, {ip,lr}

	bl      relocate_nscode

	/* check reset status */
	ldr	r0, =(EXYNOS4_POWER_BASE + 0x800) /* INFORM1 */
	ldr	r1, [r0]

	/* AFTR wakeup reset */
	ldr	r2, =S5P_CHECK_DIDLE
	cmp	r1, r2
	beq	exit_wakeup

	/* LPA wakeup reset */
	ldr	r2, =S5P_CHECK_LPA
	cmp	r1, r2
	beq	exit_wakeup

	/* Sleep wakeup reset */
	ldr	r2, =S5P_CHECK_SLEEP
	cmp	r1, r2
	beq	wakeup_reset

	/*
	 * If U-boot is already running in RAM, no need to relocate U-Boot.
	 * Memory controller must be configured before relocating U-Boot
	 * in ram.
	 */
	ldr	r0, =0x0ffffff		/* r0 <- Mask Bits*/
	bic	r1, pc, r0		/* pc <- current addr of code */
					/* r1 <- unmasked bits of pc */
	ldr	r2, _TEXT_BASE		/* r2 <- original base addr in ram */
	bic	r2, r2, r0		/* r2 <- unmasked bits of r2*/
	cmp	r1, r2			/* compare r1, r2 */
	beq	1f			/* r0 == r1 then skip sdram init */

#ifdef CONFIG_PMIC
	/* PMIC initialize */
	bl	pmic_init
#endif

	/* DMC initialize */
	bl	mem_ctrl_init

	/* CMU initialize */
	bl	clock_ctrl_init

	/* DMC initialize */
	bl	mem_ctrl_init_2nd

1:
	ldmia	r13!, {ip,pc}

wakeup_reset:
	bl	clock_ctrl_init
	bl	mem_ctrl_init

exit_wakeup:
	/* Load return address and jump to kernel */
	ldr	r0, =(EXYNOS4_POWER_BASE + 0x800) /* INFORM0 */

	/* r1 = physical address of exynos4_cpu_resume function*/
	ldr	r1, [r0]

	/* Jump to kernel */
	mov	pc, r1
	nop
	nop


relocate_nscode:
	adr	r0, nscode_base			@ r0: source address (start)
	adr	r1, nscode_end			@ r1: source address (end)
	ldr	r2, =CONFIG_PHY_IRAM_NS_BASE	@ r2: target address

1:
	ldmia	r0!, {r3-r6}
	stmia	r2!, {r3-r6}
	cmp	r0, r1
	blt	1b

	dsb
	isb

	mov	pc, lr


	.align	4
nscode_base:
	adr	r0, _ns_reg5
	b	1f

	.word	0x0		@ REG0: RESUME_ADDR
	.word	0x0		@ REG1: RESUME_FLAG
	.word	0x0		@ REG2
	.word	0x0		@ REG3
	.word	0x0		@ REG4
_ns_reg5:
	.word	0x0		@ REG5: CPU1_BOOT_REG
	.word	0x0		@ REG6: REG_DIRECTGO_FLAG
	.word	0x0		@ REG7: REG_DIRECTGO_ADDR
	.word	0x0		@ REG8
	.word	0x0		@ REG9

	nop
	nop

1:
#if defined(CONFIG_EXYNOS4412)
	mrc	p15, 0, r1, c0, c0, 5	@ MPIDR
	and	r1, r1, #0x3
	add	r0, r0, r1, lsl #0x2
#endif
cpu1_wait:
	.word	0xE320F002		@ wfe instruction
	ldr	r1, [r0]
	cmp	r1, #0x0
	bxne	r1
	b	cpu1_wait
	nop
nscode_end:
