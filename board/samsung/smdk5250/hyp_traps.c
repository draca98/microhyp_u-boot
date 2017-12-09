#include "hyp.h"
#include "lpae.h"

#define __AC(X,Y)   (X##Y)
#define _AC(X,Y)    __AC(X,Y)

#define FSC_FLT_PERM   (0x0c)

#define FSC_LL_MASK    (_AC(0x03,U)<<0)

#define LONG_BYTEORDER 2
#define BYTES_PER_LONG (1 << LONG_BYTEORDER)
#define BITS_PER_LONG (BYTES_PER_LONG << 3)

#define GENMASK(h, l) \
    (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define HPFAR_MASK      GENMASK(31, 4)

#define PAGE_SIZE 4096UL
#define PAGE_MASK (~(PAGE_SIZE-1))

static unsigned long nr_hyp_pgtraps = 0;
static unsigned long nr_hyp_ctraps = 0;

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

static inline void __flush_tlb_one(uint32_t va)
{
	asm volatile(STORE_CP32(0, TLBIMVAAIS) : : "r" (va) : "memory");
}

void hvc_set_exec(uint32_t pa, int32_t size)
{
	set_protect_area(pa, size, PERM_RX);
	p2m_addr_set_perm(pa, size, PERM_RX);

	/* TODO: Hard coding address */
	p2m_addr_set_perm(0x4053a000, 0x7fa34000, PERM_RW);
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

void do_data_abort_guest(struct cpu_user_regs *regs, const union hsr hsr)
{
	const struct hsr_dabt dabt = hsr.dabt;
	uint64_t gva;
	uint64_t ipa;
	uint8_t fsc = hsr.dabt.dfsc & ~FSC_LL_MASK;

	switch (fsc)
	{
	case FSC_FLT_PERM:
		gva = READ_CP32(HDFAR);
		ipa = (READ_SYSREG(HPFAR) & HPFAR_MASK) << (12 - 4);
		ipa |= gva & ~PAGE_MASK;

		if (!dabt.write) {
			/* unhandled */
			break;
		}

		if (is_protect_area(ipa)) {
			/* write is never allowed */
			break;
		} else {
			p2m_addr_set_perm(ipa & PAGE_MASK, PAGE_SIZE, PERM_RW);
			asm volatile("dsb" ::);
			__flush_tlb_one(gva & PAGE_MASK);
		}
		break;
	default:
		/* unhandled */
		advance_pc(regs);
		break;
	}
}

#define isb()           asm volatile("isb" : : : "memory")

static inline uint64_t gva_to_ipa_par(vaddr_t va)
{
	uint64_t par, tmp;
	tmp = READ_CP64(PAR);

	WRITE_CP32(va, ATS1CPR);
	isb(); /* Ensure result is available. */
	par = READ_CP64(PAR);
	WRITE_CP64(tmp, PAR);
	return par;
}

static inline int gva_to_ipa(vaddr_t va, paddr_t *paddr)
{
	uint64_t par = gva_to_ipa_par(va);
	*paddr = (par & PAGE_MASK) | ((unsigned long) va & ~PAGE_MASK);
	return 0;
}

void do_instr_abort_guest(struct cpu_user_regs *regs, const union hsr hsr)
{
	uint8_t fsc = hsr.iabt.ifsc & ~FSC_LL_MASK;
	uint64_t gva;
	uint64_t ipa;
	uint8_t mode;

	switch (fsc)
	{
	case FSC_FLT_PERM:
		gva = READ_CP32(HIFAR);
		gva_to_ipa(gva, &ipa);
		if (is_protect_area(ipa))
			/* never allowed */
			break;
		mode = regs->spsr & PSR_MODE_MASK;

		/* Only PL0 is allowed execution
		 * exception vector 0xffffxxxx are allowed
		 */
		if (mode != PSR_MODE_USR && gva < 0xFFFF0000)
			/* never allowed */
			break;

		p2m_addr_set_perm(ipa & PAGE_MASK, PAGE_SIZE, PERM_RX);
		asm volatile("dsb" ::);
		__flush_tlb_one(gva & PAGE_MASK);
		break;
	default:
		advance_pc(regs);
		break;
	}
}

void do_trap_hyp(struct cpu_user_regs *regs)
{
	const union hsr hsr = { .bits = READ_SYSREG32(HSR) };

	switch (hsr.ec) {
	case HSR_EC_CP15_32:
		nr_hyp_ctraps++;
		do_cp15_32(regs, hsr);
		break;
	case HSR_EC_HVC32:
		do_hvc32(regs, hsr);
		break;
	case HSR_EC_DATA_ABORT_LOWER_EL:
		nr_hyp_pgtraps++;
		do_data_abort_guest(regs, hsr);
		break;
	case HSR_EC_INSTR_ABORT_LOWER_EL:
		nr_hyp_pgtraps++;
		do_instr_abort_guest(regs, hsr);
		break;
	default:
		advance_pc(regs);
		break;
	}
}
