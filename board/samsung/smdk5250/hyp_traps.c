#include "hyp.h"
#include "lpae.h"

void advance_pc(struct cpu_user_regs *regs)
{
	regs->elr += 4;
}

static void do_cp15_32(struct cpu_user_regs *regs, const union hsr hsr)
{
	const struct hsr_cp32 cp32 = hsr.cp32;
	int regidx = cp32.reg;
	uint32_t *r = (uint32_t *)&regs->r0 + regidx;

#ifndef CONFIG_SPL_BUILD
//	printf("hsr.bits=%x\n", hsr.bits & HSR_CP32_REGS_MASK);
#endif
	switch (hsr.bits & HSR_CP32_REGS_MASK)
	{
	case HSR_CPREG32(SCTLR):
		TVM_EMUL(regs, hsr, *r, SCTLR);
		break;
	case HSR_CPREG32(TTBR0_32):
		TVM_EMUL(regs, hsr, *r, TTBR0_32);
		break;
	case HSR_CPREG32(TTBR1_32):
		TVM_EMUL(regs, hsr, *r, TTBR1_32);
		break;
	case HSR_CPREG32(TTBCR):
		TVM_EMUL(regs, hsr, *r, TTBCR);
		break;
	case HSR_CPREG32(DACR):
		TVM_EMUL(regs, hsr, *r, DACR);
		break;
	case HSR_CPREG32(DFSR):
		TVM_EMUL(regs, hsr, *r, DFSR);
		break;
	case HSR_CPREG32(IFSR):
		TVM_EMUL(regs, hsr, *r, IFSR);
		break;
	case HSR_CPREG32(DFAR):
		TVM_EMUL(regs, hsr, *r, DFAR);
		break;
	case HSR_CPREG32(IFAR):
		TVM_EMUL(regs, hsr, *r, IFAR);
		break;
	case HSR_CPREG32(ADFSR):
		TVM_EMUL(regs, hsr, *r, ADFSR);
		break;
	case HSR_CPREG32(AIFSR):
		TVM_EMUL(regs, hsr, *r, AIFSR);
		break;
	case HSR_CPREG32(MAIR0):
		TVM_EMUL(regs, hsr, *r, MAIR0);
		break;
	case HSR_CPREG32(MAIR1):
		TVM_EMUL(regs, hsr, *r, MAIR1);
		break;
	case HSR_CPREG32(AMAIR0):
		TVM_EMUL(regs, hsr, *r, AMAIR0);
		break;
	case HSR_CPREG32(AMAIR1):
		TVM_EMUL(regs, hsr, *r, AMAIR1);
		break;
	case HSR_CPREG32(CONTEXTIDR):
		TVM_EMUL(regs, hsr, *r, CONTEXTIDR);
		break;
	}

	advance_pc(regs);
}

void hvc_set_exec(uint32_t pa, int32_t size)
{
	set_protect_area(pa, size, PERM_RX);
	p2m_addr_set_perm(pa, size, PERM_RX);
}

void hvc_set_ro(uint32_t pa, int32_t size)
{
	set_protect_area(pa, size, PERM_RO);
	p2m_addr_set_perm(pa, size, PERM_RO);
}

void hvc_enable_protect(void)
{
	init_unprotect_area();
}

static void do_hvc32(struct cpu_user_regs *regs, const union hsr hsr)
{
	switch (hsr.iss) {
	case 0:
		/* reserved */
		break;
	case 1:
		/* set kernel exec memory */
		hvc_set_exec(regs->r0, regs->r1);
		break;
	case 2:
		/* set kernel ro memory */
		hvc_set_ro(regs->r0, regs->r1);
		break;
	case 3:
		/* enable stage 2 protection */
		hvc_enable_protect();
		break;
	}
}

void do_trap_hyp(struct cpu_user_regs *regs)
{
	const union hsr hsr = { .bits = READ_SYSREG32(HSR) };

	switch (hsr.ec) {
	case HSR_EC_CP15_32:
		do_cp15_32(regs, hsr);
		break;
	case HSR_EC_HVC32:
		do_hvc32(regs, hsr);
		break;
	default:
		advance_pc(regs);
		break;
	}
}
