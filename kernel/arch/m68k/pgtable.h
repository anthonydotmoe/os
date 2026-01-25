#pragma once

// On Linux, this is CONFIG_RAMBASE
#define PAGE_OFFSET         0

/*
 * Definitions for MMU descriptors
 */
#define _PAGE_PRESENT       0x001
#define _PAGE_SHORT         0x002
#define _PAGE_RONLY         0x004
#define _PAGE_READWRITE     0x000
#define _PAGE_ACCESSED      0x008
#define _PAGE_DIRTY         0x010
#define _PAGE_SUPER         0x080    /* 68040 supervisor only */
#define _PAGE_GLOBAL        0x400    /* 68040 global bit, used for kva descs */
#define _PAGE_NOCACHE       0x060    /* 68040 cache mode, non-serialized */
#define _PAGE_NOCACHE_S     0x040    /* 68040 no-cache mode, serialized */
#define _PAGE_CACHE         0x020    /* 68040 cache mode, cachable, copyback */
#define _PAGE_CACHEW        0x000    /* 68040 cache mode, cachable, write-through */

#define _DESCTYPE_MASK      0x003

#define _CACHEMASK040       (~0x060)

/*
 * Currently set to the minimum alignment of table pointers (256 bytes).
 * The hardware only uses the low 4 bits for state:
 *
 *    3 - Used
 *    2 - Write Protected
 *  0,1 - Descriptor Type
 *
 * and has the rest of the bits reserved.
 */
#define _TABLE_MASK     (0xffffff00)

#define _PAGE_TABLE     (_PAGE_SHORT)
#define _PAGE_CHG_MASK  (PAGE_MASK | _PAGE_ACCESSED | _PAGE_DIRTY | _PAGE_NOCACHE)

#define _PAGE_PROTNONE      0x004

/* We borrow bit 11 to store the exclusive marker in swap PTEs. */
#define _PAGE_SWP_EXCLUSIVE 0x800
