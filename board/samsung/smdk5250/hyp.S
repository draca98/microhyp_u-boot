#include <config.h>
#ifndef CONFIG_SPL_BUILD

.syntax	unified
.arch_extension virt
.text

hyp_stack_end:
	.fill 0x100,1,0  /* 256 bytes */ 
hyp_stack:
	.long hyp_stack_end + 0x100

.align 5

/* We use the same vector table for Hyp and Monitor mode, since
 * we will only use each once and they don't overlap.
 */
hyp_vectors:
	.word 0	/* reset */
	.word 0	/* undef */
	.word 0 /* smc */
	.word 0 /* pabt */
	.word 0 /* dabt */
	b	trap_hyp
	.word 0 /* irq */
	.word 0 /* fiq */

trap_hyp:
	push {r0-r12}
	mrs r0, spsr_hyp
	mrs r1, elr_hyp
	push {r0, r1, lr}

	add r0, sp, #0xc /* pointer user_regs to r0-r12 */
	bl do_trap_hyp

	pop {r0, r1, lr}
	msr spsr_hyp, r0
	add r1, 4
	msr elr_hyp, r1
	pop {r0-r12}
	eret
	
/******************************************************************************
 * This code is called from u-boot into the above handler
 */

	.globl hyp_init
hyp_init:
	ldr sp, =hyp_stack
	msr	elr_hyp, lr

	push {r0-r12}
	mrs r0, spsr_hyp
	mrs r1, elr_hyp
	push {r0, r1, lr}

	bl hyp_mmu_init

	/* HCR */
	mrc	p15, 4, r0, c1, c1, 0		@ sead HCR
	orr	r0, r0, 0x4000000
	mcr	p15, 4, r0, c1, c1, 0		@ set HCR

	ldr	ip, =hyp_vectors
	mcr	p15, 4, ip, c12, c0, 0		@ set HVBAR

	pop {r0, r1, lr}
	msr spsr_hyp, r0
	msr elr_hyp, r1
	pop {r0-r12}
	eret
#endif