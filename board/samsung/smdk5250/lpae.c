#include "lpae.h"
#include "hyp.h"

#ifndef CONFIG_SPL_BUILD

#define L1_PTE_NUM	512
#define L2_PTE_NUM	1024 /* 2GB */
#define L3_PTE_NUM	512

#define VTCR_INITVAL                                    0x80000000
#define VTCR_SH0_MASK                                   0x00003000
#define VTCR_SH0_SHIFT                                  12
#define VTCR_ORGN0_MASK                                 0x00000C00
#define VTCR_ORGN0_SHIFT                                10
#define VTCR_IRGN0_MASK                                 0x00000300
#define VTCR_IRGN0_SHIFT                                8
#define VTCR_SL0_MASK                                   0x000000C0
#define VTCR_SL0_SHIFT                                  6
#define VTCR_S_MASK                                     0x00000010
#define VTCR_S_SHIFT                                    4
#define VTCR_T0SZ_MASK                                  0x00000003
#define VTCR_T0SZ_SHIFT                                 0

#define VTTBR_INITVAL                                   0x0000000000000000ULL
#define VTTBR_VMID_MASK                                 0x00FF000000000000ULL
#define VTTBR_VMID_SHIFT                                48
#define VTTBR_BADDR_MASK                                0x000000FFFFFFF000ULL
#define VTTBR_BADDR_SHIFT                               12

#define MAX_PROT_AREA 10

struct mem_sect {
	uint32_t addr;
	int32_t size;
	p2m_perm_t perm;
};

int prot_area_num = 0;

/* kernel code/ro area map */
static struct mem_sect prot_area[MAX_PROT_AREA] = {0};

static lpae_t __pgtable_l1[L1_PTE_NUM] \
		__attribute((__aligned__(4096)));

/* For 0x40000000--0xBFFFFFFF, l2, l3 pte */
static lpae_t __pgtable_l2[L2_PTE_NUM] \
		__attribute((__aligned__(4096)));

static lpae_t __pgtable_l3[L2_PTE_NUM][L3_PTE_NUM] \
		__attribute((__aligned__(4096)));

/**
 * \defgroup LPAE_address_mask
 *
 * The address mask of each level descriptors.
 * - TTBL_L1_OUTADDR_MASK[39:30] Level1 Block address mask.
 * - TTBL_L2_OUTADDR_MASK[39:21] Level2 Block Address mask.
 * - TTBL_L3_OUTADDR_MASK[39:12] Page address mask.
 *
 * - TTBL_L1_TABADDR_MASK[39:12] Level2 table descriptor address mask.
 * - TTBL_L2_TABADDR_MASK]30:12] Level3 table descriptor address mask.
 * @{
 */
#define TTBL_L1_OUTADDR_MASK    0x000000FFC0000000ULL
#define TTBL_L2_OUTADDR_MASK    0x000000FFFFE00000ULL
#define TTBL_L3_OUTADDR_MASK    0x000000FFFFFFF000ULL

#define TTBL_L1_TABADDR_MASK    0x000000FFFFFFF000ULL
#define TTBL_L2_TABADDR_MASK    0x000000FFFFFFF000ULL

/*
 * Level 1 Block, 1GB, entry in LPAE Descriptor format
 * for the given physical address
 */
#define printh printf

lpae_t p2m_l1_block(uint64_t pa, uint8_t attr_idx)
{
	/* lpae.c */
	lpae_t e = (lpae_t) {
		.p2m.af = 1,
		.p2m.read = 1,
		.p2m.table = 0,
		.p2m.valid = 1,
	};

	e.p2m.base = (pa & TTBL_L1_OUTADDR_MASK) >> 12;
	e.p2m.mattr = MATTR_MEM;
	e.p2m.sh = LPAE_SH_INNER;
	e.p2m.xn = 0;
	e.p2m.write = 1;

	return e;
}

lpae_t p2m_l1_table(uint64_t baddr)
{
	/* lpae.c */
	lpae_t e = (lpae_t) {
		.p2m.table = 1,
		.p2m.valid = 1,
	};

	e.bits &= ~TTBL_L1_TABADDR_MASK;
	e.bits |= baddr & TTBL_L1_TABADDR_MASK;
	return e;
}

lpae_t p2m_l2_table(uint64_t baddr)
{
	/* lpae.c */
	lpae_t e = (lpae_t) {
		.p2m.table = 1,
		.p2m.valid = 1,
	};

	e.bits &= ~TTBL_L2_TABADDR_MASK;
	e.bits |= baddr & TTBL_L2_TABADDR_MASK;
	return e;
}

lpae_t p2m_l3_block(uint64_t pa)
{
	/* lpae.c */
	lpae_t e = (lpae_t) {
		.p2m.af = 1,
		.p2m.read = 1,
		.p2m.table = 1,
		.p2m.valid = 1,
	};

	e.p2m.base = (pa & TTBL_L3_OUTADDR_MASK) >> 12;
	e.p2m.mattr = MATTR_MEM;
	e.p2m.sh = LPAE_SH_INNER;
	e.p2m.xn = 0;
	e.p2m.write = 1;

	return e;
}

void p2m_set_perm(lpae_t *pte, p2m_perm_t perm)
{
	pte->p2m.read = 1;
	if (perm == PERM_RO) {
		pte->p2m.xn = 1;
		pte->p2m.write = 0;
	} else if (perm == PERM_RX) {
		pte->p2m.xn = 0;
		pte->p2m.write = 0;
	} else  if (perm == PERM_RW) {
		pte->p2m.xn = 1;
		pte->p2m.write = 1;
	}
}

void p2m_addr_set_perm(uint64_t addr, int32_t size, p2m_perm_t perm)
{
	int l2_index, l3_index;

	if (addr < 0x40000000 || addr >= 0xc0000000) {
		/* invalid address */
		return;
	}

	l2_index = (addr - 0x40000000) >> 21;
	l3_index = ((addr - 0x40000000) & 0x1fffff) >> 12;

	while (size > 0) {
		lpae_t *pte = &__pgtable_l3[l2_index][l3_index];
		p2m_set_perm(pte, perm);
		size -= 4096;
		l3_index++;

		if (l3_index == 512) {
			l2_index++;
			l3_index = 0;
		}
	}
}

int is_protect_area(uint32_t addr)
{
	int ret = 0;
	int i = 0;
	struct mem_sect *prot;

	for (i = 0; i < prot_area_num; i++) {
		prot = &prot_area[i];

		if ((addr >= prot->addr) &&
				(addr < (prot->addr + prot->size))) {
			return 1;
		}
	}

	return ret;
}

void set_protect_area(uint32_t addr, int32_t size, p2m_perm_t perm)
{
	struct mem_sect *prot;

	if (prot_area_num >= MAX_PROT_AREA) {
		/* unable add prot area */
		return;
	}

	prot = &prot_area[prot_area_num];
	prot->addr = addr;
	prot->size = size;
	prot->perm = perm;
	prot_area_num++;
}

void init_unprotect_area(void)
{

}

void guest_mmu_init(void)
{
	uint32_t vtcr, hcr;
	uint64_t vttbr;

	vtcr = READ_SYSREG32(VTCR);
	vtcr &= ~VTCR_SL0_MASK;
	vtcr |= (0x01 << VTCR_SL0_SHIFT) & VTCR_SL0_MASK;
	vtcr &= ~VTCR_ORGN0_MASK;
	vtcr |= (0x3 << VTCR_ORGN0_SHIFT) & VTCR_ORGN0_MASK;
	vtcr &= ~VTCR_IRGN0_MASK;
	vtcr |= (0x3 << VTCR_IRGN0_SHIFT) & VTCR_IRGN0_MASK;
	//vtcr |= 0x18; // T0SZ -8	
	WRITE_SYSREG32(vtcr, VTCR);

	vttbr = READ_SYSREG64(VTTBR);
	vttbr &= ~(VTTBR_VMID_MASK);
	vttbr |= ((uint64_t)0 << VTTBR_VMID_SHIFT) & VTTBR_VMID_MASK;
	vttbr &= ~(VTTBR_BADDR_MASK);
	vttbr |= (uint32_t)__pgtable_l1 & VTTBR_BADDR_MASK;
	WRITE_SYSREG64(vttbr, VTTBR);

	hcr = READ_SYSREG32(HCR);
	hcr |= (0x1);
	WRITE_SYSREG32(hcr, HCR);
};

void hyp_mmu_init(void)
{
	uint64_t pa = 0x0ULL;
	int i = 0;
	int j = 0;

	__pgtable_l1[0] = p2m_l1_block(pa, ATTR_IDX_DEV_SHARED);
	pa += 0x40000000ULL;

	__pgtable_l1[1] = p2m_l1_table((uint32_t)&__pgtable_l2[i]);
	for (i = 0; i < 512; i++) {
		__pgtable_l2[i] = p2m_l2_table((uint32_t)&__pgtable_l3[i][0]);

		for (j = 0; j < 512; j++) {
			__pgtable_l3[i][j] = p2m_l3_block(pa);
			pa += 0x1000;
		}
	}

	__pgtable_l1[2] = p2m_l1_table((uint32_t)&__pgtable_l2[512]);
	for (i = 512; i < 1024; i++) {
		__pgtable_l2[i] = p2m_l2_table((uint32_t)&__pgtable_l3[i][0]);

		for (j = 0; j < 512; j++) {
			__pgtable_l3[i][j] = p2m_l3_block(pa);
			pa += 0x1000;
		}
	}

	guest_mmu_init();
}

#endif
