#include <stdint.h>
#include <stdbool.h>

#include "kernel/mm.h"
#include "kernel/printk.h"
#include "mm.h"
#include "pgtable.h"
#include "head.h"
#include "asm/sections.h"

#ifdef DEBUG
/* -------------------------------------------------------------------------- */

enum {
    PAGE_SHIFT      = 12,
    PAGE_SIZE       = 1u << PAGE_SHIFT,

    ROOT_BITS       = 7,
    PTR_BITS        = 7,
    PAGE_BITS       = 6,

    ROOT_ENTRIES    = 1u << ROOT_BITS,
    PTR_ENTRIES     = 1u << PTR_BITS,
    PAGE_ENTRIES    = 1u << PAGE_BITS,
};

enum {
    ROOT_COVERAGE = PAGE_SIZE << (PTR_BITS + PAGE_BITS),
    PTR_COVERAGE  = PAGE_SIZE << PAGE_BITS,
    PAGE_COVERAGE = PAGE_SIZE
};

enum {
    ATTR_GLOBAL     = 1u << 0,
    ATTR_SUPERVISOR = 1u << 1,

    // Encode the cache mode in 2 bits (same semantics as descriptor bits 6..5)
    ATTR_CACHE_SHIFT = 2,
    ATTR_CACHE_MASK  = 0x3u << ATTR_CACHE_SHIFT,

    ATTR_MODIFIED   = 1u << 4,
};

static inline uint32_t desc_attrs_key(uint32_t desc)
{
    uint32_t k = 0;

    if (desc & (1u << 10)) k |= ATTR_GLOBAL;
    if (desc & (1u <<  7)) k |= ATTR_SUPERVISOR;
    if (desc & (1u <<  4)) k |= ATTR_MODIFIED;
    const uint32_t cachemode = (desc >> 5) & 0x3u;
    k |= (cachemode << ATTR_CACHE_SHIFT);

    return k;
}

static inline int root_desc_valid(uint32_t d) { return (d & (1u << 1)) != 0; }
static inline int ptr_desc_valid (uint32_t d) { return (d & (1u << 1)) != 0; }
static inline int page_desc_valid(uint32_t d) { return (d & (1u << 0)) != 0; }
static inline uintptr_t root_table_base(uint32_t d) { return (uintptr_t)phys_to_virt(d & 0xFFFFFE00u); }
static inline uintptr_t ptr_table_base (uint32_t d) { return (uintptr_t)phys_to_virt(d & 0xFFFFFF00u); }
static inline uint32_t page_phys_and_flags (uint32_t d) { return d & 0xFFFFF4E0u; }
static inline uint32_t page_phys_base(uint32_t d) { return d & 0xFFFFF000u; }

// Range coalescing
typedef enum {
    RANGE_UNINIT    =  0,
    RANGE_VALID     =  1,
    RANGE_INVALID   = -1
} range_state_t;

typedef struct {
    range_state_t state;

    uint32_t v_start, v_end;    // [start,end)
    uint32_t p_start, p_end;    // valid only for RANGE_VALID
    uint32_t attrs;             // valid only for RANGE_VALID / RANGE_TRANSPARENT
} range_coalescer_t;

static void coalesce_init(range_coalescer_t *c)
{
    c->state = RANGE_UNINIT;
    c->v_start = c->v_end = 0;
    c->p_start = c->p_end = 0;
    c->attrs = 0;
}

static void print_attrs_columns(uint32_t attrs)
{
    const bool g = (attrs & ATTR_GLOBAL) != 0;
    const bool s = (attrs & ATTR_SUPERVISOR) != 0;
    const bool m = (attrs & ATTR_MODIFIED) != 0;
    const uint32_t cm = (attrs & ATTR_CACHE_MASK) >> ATTR_CACHE_SHIFT;

    const char G = g ? 'G' : '-';
    const char S = s ? 'S' : '-';
    const char M = m ? 'M' : '-';

    // mutually exclusive:
    const char W = (cm == 0) ? 'W' : '-';
    const char C = (cm == 1) ? 'C' : '-';
    const char z = (cm == 2) ? 's' : '-';

    printk("%c %c %c %c %c %c", M, G, S, W, C, z);
}

static void coalesce_flush(range_coalescer_t *c)
{
    if (c->state == RANGE_UNINIT)
        return;
    
    if (c->state == RANGE_INVALID)
    {
        printk("║ "
               LOG_ANSI_SELECT_COLOR_RED_
               "- [0x%8x,0x%8x)            INVALID"
               LOG_ANSI_RESET_ATTRIBS_
               "                                 ║\n",
            c->v_start, c->v_end);
        c->state = RANGE_UNINIT;
        return;
    }

    if (c->state == RANGE_VALID)
    {
        printk("║ M [0x%8x,0x%8x) -> [0x%8x,0x%8x)             ",
            c->v_start, c->v_end,
            c->p_start, c->p_end);
        print_attrs_columns(c->attrs);
        printk(" ║\n");
        c->state = RANGE_UNINIT;
        return;
    }
}

static void coalesce_invalid(range_coalescer_t *c, uint32_t logical, uint32_t next_logical)
{
    // If we're currently tracking an invalid run and this chunk is contiguous,
    // extend it.
    if (c->state == RANGE_INVALID && logical == c->v_end)
    {
        c->v_end = next_logical;
        return;
    }

    // Otherwise, close whatever was open and start a new invalid run.
    coalesce_flush(c);

    c->state = RANGE_INVALID;
    c->v_start = logical;
    c->v_end = next_logical;
}

static void coalesce_valid(range_coalescer_t *c,
                           uint32_t logical,
                           uint32_t phys_start,
                           uint32_t next_logical,
                           uint32_t attrs_key)
{
    const uint32_t delta = next_logical - logical;

    // Extend current valid range if contiguous and linear.
    if (c->state <= RANGE_VALID &&
        logical == c->v_end &&
        phys_start == c->p_end &&
        attrs_key == c->attrs)
    {
        c->v_end = next_logical;
        c->p_end += delta;
        return;
    }

    // Otherwise, close whatever was open and start a new valid range.
    coalesce_flush(c);

    c->state = RANGE_VALID;
    c->v_start = logical;
    c->v_end = next_logical;
    c->p_start = phys_start;
    c->p_end = phys_start + delta;
    c->attrs = attrs_key;
}

typedef struct {
    range_coalescer_t co;
} mmu_print_ctx_t;

void mmu_print_040(uint32_t *root)
{
    mmu_print_ctx_t ctx;
    coalesce_init(&ctx.co);

    printk("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printk("║ 68040 MMU Report                          Memory Base   : 0x%8x         ║\n", (uint32_t)phys_kernel_start);
    printk("║ Root Pointer -> 0x%8x                       Size   : -M                 ║\n", (uint32_t)(uintptr_t)root);
    printk("║──────────────────────────────────────────────────────────────────────────────║\n");
    printk("║                                                       Serialized ──────────╮ ║\n");
    printk("║                                                         Copyback ────────╮ │ ║\n");
    printk("║                                                    Write-through ──────╮ │ │ ║\n");
    printk("║                                                       Supervisor ────╮ │ │ │ ║\n");
    printk("║                                                           Global ──╮ │ │ │ │ ║\n");
    printk("║                                                         Modified ╮ │ │ │ │ │ ║\n");
    printk("║      Virtual Addr.             Physical Addr.                    M G S W C s ║\n");
    printk("║──────────────────────────────────────────────────────────────────────────────║\n");

    uint32_t logical = 0;

    for (uint32_t r = 0; r < ROOT_ENTRIES; r++)
    {
        const uint32_t next_logical_root = logical + ROOT_COVERAGE;

        const uint32_t rd = root[r];
        if (!root_desc_valid(rd)) {
            coalesce_invalid(&ctx.co, logical, next_logical_root);
            logical = next_logical_root;
            continue;
        }

        volatile uint32_t *pt = (volatile uint32_t *)root_table_base(rd);

        for (uint32_t p = 0; p < PTR_ENTRIES; p++)
        {
            const uint32_t next_logical_ptr = logical + PTR_COVERAGE;

            const uint32_t pd = pt[p];
            if (!ptr_desc_valid(pd))
            {
                coalesce_invalid(&ctx.co, logical, next_logical_ptr);
                logical = next_logical_ptr;
                continue;
            }

            volatile uint32_t *pg = (volatile uint32_t *)ptr_table_base(pd);

            for (uint32_t i = 0; i < PAGE_ENTRIES; i++) {
                const uint32_t next_logical_page = logical + PAGE_COVERAGE;

                const uint32_t pgd = pg[i];
                if (!page_desc_valid(pgd))
                {
                    coalesce_invalid(&ctx.co, logical, next_logical_page);
                    logical = next_logical_page;
                    continue;
                }

                /*

                What to print as "physical":
                - Linux's code masks with 0xFFFFF4E0 and then prints flags
                - If you only care about frame base, use page_phys_base(pgd)
                */
                const uint32_t phys_for_printing = page_phys_base(pgd);
                coalesce_valid(&ctx.co,
                                logical,
                                phys_for_printing,
                                next_logical_page,
                                desc_attrs_key(pgd)
                );
                logical = next_logical_page;
            }
        }

        // At end of root entry, 'logical' should already be next_logical_root,
        // but we keep it explicit to mirror the asm's structure
        logical = next_logical_root;
    }
    coalesce_flush(&ctx.co);
    printk("╚══════════════════════════════════════════════════════════════════════════════╝\n");
}

#else
#define mmu_print_040(x)
#endif /* DEBUG */

static int32_t memoffset;

void mm_init_offset()
{
    memoffset = (int32_t)phys_kernel_start - PAGE_OFFSET;
}

void mm_init()
{
    mmu_print_040((uint32_t*)kernel_pg_dir);
}

phys_addr_t virt_to_phys(virt_addr_t va)
{
    return (phys_addr_t)(va + memoffset);
}

virt_addr_t phys_to_virt(phys_addr_t pa)
{
    return (virt_addr_t)(pa - memoffset);
}
