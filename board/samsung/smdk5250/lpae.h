#include <common.h>

typedef struct __attribute__((__packed__)) {
	/* These are used in all kinds of entry. */
	unsigned long valid:1;      /* Valid mapping */
	unsigned long table:1;      /* == 1 in 4k map entries too */

	/* These ten bits are only used in Block entries and are ignored
	 * in Table entries. */
	unsigned long ai:3;         /* Attribute Index */
	unsigned long ns:1;         /* Not-Secure */
	unsigned long user:1;       /* User-visible */
	unsigned long ro:1;         /* Read-Only */
	unsigned long sh:2;         /* Shareability */
	unsigned long af:1;         /* Access Flag */
	unsigned long ng:1;         /* Not-Global */

	/* The base address must be appropriately aligned for Block entries */
	unsigned long long base:36; /* Base address of block or next table */
	unsigned long sbz:4;        /* Must be zero */

	/* These seven bits are only used in Block entries and are ignored
	 * in Table entries. */
	unsigned long contig:1;     /* In a block of 16 contiguous entries */
	unsigned long pxn:1;        /* Privileged-XN */
	unsigned long xn:1;         /* eXecute-Never */
	unsigned long avail:4;      /* Ignored by hardware */

	/* These 5 bits are only used in Table entries and are ignored in
	 * Block entries */
	unsigned long pxnt:1;       /* Privileged-XN */
	unsigned long xnt:1;        /* eXecute-Never */
	unsigned long apt:2;        /* Access Permissions */
	unsigned long nst:1;        /* Not-Secure */
} lpae_pt_t;

/* The p2m tables have almost the same layout, but some of the permission
 * and cache-control bits are laid out differently (or missing) */
typedef struct __attribute__((__packed__)) {
	/* These are used in all kinds of entry. */
	unsigned long valid:1;      /* Valid mapping */
	unsigned long table:1;      /* == 1 in 4k map entries too */

	/* These ten bits are only used in Block entries and are ignored
	 * in Table entries. */
	unsigned long mattr:4;      /* Memory Attributes */
	unsigned long read:1;       /* Read access */
	unsigned long write:1;      /* Write access */
	unsigned long sh:2;         /* Shareability */
	unsigned long af:1;         /* Access Flag */
	unsigned long sbz4:1;

	/* The base address must be appropriately aligned for Block entries */
	unsigned long long base:36; /* Base address of block or next table */
	unsigned long sbz3:4;

	/* These seven bits are only used in Block entries and are ignored
	 * in Table entries. */
	unsigned long contig:1;     /* In a block of 16 contiguous entries */
	unsigned long sbz2:1;
	unsigned long xn:1;         /* eXecute-Never */
	unsigned long type:4;       /* Ignore by hardware. Used to store p2m types */

	unsigned long sbz1:5;
} lpae_p2m_t;

/* Permission mask: xn, write, read */
#define P2M_PERM_MASK (0x00400000000000C0ULL)
#define P2M_CLEAR_PERM(pte) ((pte).bits & ~P2M_PERM_MASK)

/*
 * Walk is the common bits of p2m and pt entries which are needed to
 * simply walk the table (e.g. for debug).
 */
typedef struct __attribute__((__packed__)) {
	/* These are used in all kinds of entry. */
	unsigned long valid:1;      /* Valid mapping */
	unsigned long table:1;      /* == 1 in 4k map entries too */

	unsigned long pad2:10;

	/* The base address must be appropriately aligned for Block entries */
	unsigned long long base:36; /* Base address of block or next table */

	unsigned long pad1:16;
} lpae_walk_t;

typedef union {
	uint64_t bits;
	lpae_pt_t pt;
	lpae_p2m_t p2m;
	lpae_walk_t walk;
} lpae_t;

typedef u32 vaddr_t;
typedef u64 paddr_t;

#define ATTR_IDX_UNCACHED      0x0
#define ATTR_IDX_BUFFERABLE    0x1
#define ATTR_IDX_WRITETHROUGH  0x2
#define ATTR_IDX_WRITEBACK     0x3
#define ATTR_IDX_DEV_SHARED    0x4
#define ATTR_IDX_WRITEALLOC    0x7
#define ATTR_IDX_DEV_NONSHARED ATTR_IDX_DEV_SHARED
#define ATTR_IDX_DEV_WC        ATTR_IDX_BUFFERABLE
#define ATTR_IDX_DEV_CACHED    ATTR_IDX_WRITEBACK

/*
 * Stage 2 Memory Type.
 *
 * These are valid in the MemAttr[3:0] field of an LPAE stage 2 page
 * table entry.
 *
 */
#define MATTR_DEV     0x1
#define MATTR_MEM_NC  0x5
#define MATTR_MEM     0xf

/* Shareability values for the LPAE entries */
#define LPAE_SH_NON_SHAREABLE 0x0
#define LPAE_SH_UNPREDICTALE  0x1
#define LPAE_SH_OUTER         0x2
#define LPAE_SH_INNER         0x3

