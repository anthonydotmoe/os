#pragma once

/*
 * 68040 pagetable geometry for 4 KiB pages (3-level):
 *   Root:    128 entries (512 bytes) -> covers  32 MiB per entry
 *   Pointer: 128 entries (512 bytes) -> covers 256 KiB per entry
 *   Page:     64 entries (256 bytes) -> covers   4 KiB per entry
 *
 * Index bits:
 *   root index: bits 31..25 (7 bits)
 *   ptr  index: bits 24..18 (7 bits)
 *   page index: bits 17..12 (6 bits)
 */

#define PAGE_SHIFT          12u
//#define PAGE_SIZE           (1u << PAGE_SHIFT)
#define PAGE_MASK           (~(PAGE_SIZE - 1u))

#define ROOT_ENTRIES        128u
#define PTR_ENTRIES         128u
#define PAGE_ENTRIES        64u

#define ROOT_INDEX_SHIFT    25u
#define PTR_INDEX_SHIFT     18u
#define PAGE_INDEX_SHIFT    12u

/*
 * Descriptor type (low two bits).
 *
 * Page descriptor type:
 *   00b = invalid
 *   01b = resident
 *   11b = resident
 *   10b = indirect (not used in this system)
 *
 * Table descriptor type:
 *   00b = invalid
 *   01b = invalid
 *   10b = resident
 *   11b = resident
 */
#define DESC_TYPE_MASK      0x00000003u
#define DESC_TYPE_INVALID   0x00000000u
#define DESC_TYPE_TABLE     0x00000003u
#define DESC_TYPE_PAGE      0x00000003u

/*
 * Address field masks
 *
 * Table descriptors point to the next-level table. Hardware requires alignment;
 * using these masks also clears the low "state/type" bits when extracting.
 *
 * - Root table: must be 512-byte aligned
 * - Pointer tables: must be 512-byte aligned
 * - Page tables: must be 256-byte aligned
 */
#define ROOTPTR_ALIGN_MASK  0xFFFFFE00u
#define TABLE_ADDR_MASK     0xFFFFFE00u
#define PAGE_ADDR_MASK      0xFFFFF000u

/*
 * Common attribute bits (68040)
 */
#define PTE_RONLY           0x00000004u /* write-protect (read-only) */
#define PTE_ACCESSED        0x00000008u
#define PTE_DIRTY           0x00000010u

#define PTE_SUPERVISOR      0x00000080u /* supervisor only */
#define PTE_GLOBAL          0x00000400u /* global */

/* 
 * Cache mode field
 */
#define PTE_CACHEMODE_SHIFT 5u
#define PTE_CAHCEMODE_MASK  (0x3u << PTE_CACHEMODE_SHIFT)

#define PTE_CACHE_WT        (0x0u << PTE_CACHEMODE_SHIFT) /* write-through */
#define PTE_CACHE_CB        (0x1u << PTE_CACHEMODE_SHIFT) /* copy-back */
#define PTE_CACHE_NC_SER    (0x2u << PTE_CACHEMODE_SHIFT) /* non-cache, serialized */
#define PTE_CACHE_NC        (0x3u << PTE_CACHEMODE_SHIFT) /* non-cache, non-serialized */

#define KERNEL_PTE_FLAGS    (PTE_SUPERVISOR | PTE_GLOBAL | PTE_ACCESSED | PTE_DIRTY | PTE_CACHE_CB)
#define KERNEL_RO_FLAGS     (PTE_SUPERVISOR | PTE_GLOBAL | PTE_ACCESSED | PTE_RONLY | PTE_CACHE_CB)

#define USER_PTE_FLAGS      (PTE_ACCESSED | PTE_DIRTY | PTE_CACHE_CB)
#define USER_RO_FLAGS       (PTE_ACCESSED | PTE_RONLY | PTE_CACHE_CB)

#ifndef __ASSEMBLER__

#include <stdint.h>

#define ROOT_INDEX(va)      (((uint32_t)(va) >> ROOT_INDEX_SHIFT) & (ROOT_ENTRIES - 1u))
#define PTR_INDEX(va)       (((uint32_t)(va) >> PTR_INDEX_SHIFT)  & (PTR_ENTRIES  - 1u))
#define PAGE_INDEX(va)      (((uint32_t)(va) >> PAGE_INDEX_SHIFT) & (PAGE_ENTRIES - 1u))

static inline uint32_t desc_type(uint32_t d) { return d & DESC_TYPE_MASK; }
static inline int desc_is_table(uint32_t d)  { return desc_type(d) == DESC_TYPE_TABLE; }
static inline int desc_is_page(uint32_t d)   { return desc_type(d) == DESC_TYPE_PAGE;  }

/*
 * Helpers to construct descriptors.
 */
static inline uint32_t mk_table_desc(uint32_t table_pa, uint32_t table_flags)
{
    /* table_pa must have bits 8..0 = 0 (512 alignment) */
    return (table_pa & TABLE_ADDR_MASK) | DESC_TYPE_TABLE | (table_flags & ~DESC_TYPE_MASK);
}

static inline uint32_t mk_page_desc(uint32_t page_pa, uint32_t pte_flags)
{
    /* page_pa must be 4KiB aligned */
    return (page_pa & PAGE_ADDR_MASK) | DESC_TYPE_PAGE | (pte_flags & ~DESC_TYPE_MASK);
}

#endif /* __ASSEMBLER__ */