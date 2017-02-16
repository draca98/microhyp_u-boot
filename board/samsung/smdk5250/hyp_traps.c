#include "hyp.h"

static void do_cp15_32(struct cpu_user_regs *regs, const union hsr hsr)
{
	const struct hsr_cp32 cp32 = hsr.cp32;
	int regidx = cp32.reg;
	uint32_t *r = (uint32_t *)regs + regidx;

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
}

void do_trap_hyp(struct cpu_user_regs *regs)
{
	const union hsr hsr = { .bits = READ_SYSREG32(HSR) };

	switch (hsr.ec) {
	case HSR_EC_CP15_32:
		do_cp15_32(regs, hsr);
		break;
	}
}

