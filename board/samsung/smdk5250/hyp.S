#include <config.h>
#ifndef CONFIG_SPL_BUILD

.syntax	unified
.arch_extension virt
.text

hyp_stack_end:
	.fill 0x100,1,0  /* 256 bytes */ 
hyp_stack:
	.long hyp_stack_end + 0x100
hyp_stack2_end:
	.fill 0x100,1,0  /* 256 bytes */
hyp_stack2:
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

	mov r0, sp
	bl do_trap_hyp

	pop {r0, r1, lr}
	msr spsr_hyp, r0
	msr elr_hyp, r1
	pop {r0-r12}
	eret
	
/******************************************************************************
 * This code is called from u-boot into the above handler
 */

	.globl hyp_init
hyp_init:
	dsb
9998:
	sev
	.pushsection ".alt.smp.init", "a"
	.long 9998b
	nop
	.popsection

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

	ldr r2, =0x02020000
	ldr r3, =init_secondary
	str r3, [r2]

	pop {r0, r1, lr}
	msr spsr_hyp, r0
	msr elr_hyp, r1
	pop {r0-r12}
	eret

	.global init_secondary
init_secondary:
	ldr sp, =hyp_stack2
	msr	elr_hyp, lr

	push {r0-r12}
	mrs r0, spsr_hyp
	mrs r1, elr_hyp
	push {r0, r1, lr}

	bl guest_mmu_init

	/* HCR */
	mrc	p15, 4, r0, c1, c1, 0		@ sead HCR
	orr	r0, r0, 0x4000000
	mcr	p15, 4, r0, c1, c1, 0		@ set HCR

	ldr	ip, =hyp_vectors
	mcr	p15, 4, ip, c12, c0, 0		@ set HVBAR

	pop {r0, r1, lr}
	msr spsr_hyp, r0
	ldr r1, =enter_smp_pen
	msr elr_hyp, r1
	pop {r0-r12}
	eret

__smp_pen:
        wfe
        ldr     r1, [r0]
        mov     pc, r1

        .globl enter_smp_pen
enter_smp_pen:
        adr     r1, __smp_pen
        ldmia   r1, {r4, r5, r6}
        ldr     r2, =CONFIG_SPL_SMP_PEN
        stmia   r2, {r4, r5, r6}

        /*
         * Store SMP pen into SYSFLAGS so the processor stays in the loop if
         * it gets a spurious interrupt
         */
        ldr     r0, =CONFIG_SYSFLAGS_ADDR
        str     r2, [r0]

        /*
         * Make instruction copy coherent
         */
        mcr     p15, 0, r2, c7, c11, 1 /* Clean the data cache by MVA*/
        mov     r10, #0
        mcr     p15, 0, r10, c7, c5, 0 /* Invalidate the I-cache */
        isb     /* Make sure the invalidate ops are complete */
        dsb

        mov     pc, r2

#endif
