#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <form_os/config.h>
#include <form_os/type.h>

#include "kernel/printk.h"
#include "kernel/mm.h"
#include "head.h"
#include "mm.h"
#include "mm_debug.h"
#include "pgtable.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

/* --- phys_to_virt/virt_to_phys --- */

static intptr_t memoffset;

void mm_init_offset()
{
    memoffset = (intptr_t)phys_kernel_start - KERNEL_VIRT_BASE;
}

phys_bytes virt_to_phys(virt_bytes va)
{
    return (phys_bytes)(va + memoffset);
}

virt_bytes phys_to_virt(phys_bytes pa)
{
    return (virt_bytes)(pa - memoffset);
}

/* ------------------- Physical memory management --------------------------- */

// Bitfield
// 0 is allocated, 1 is free
// Page frame number ordering is MSB->LSB within each 32-bit word. Word 0 is
// the first PFN. This makes printf make more sense and works in the favor of
// a potential `bfffo` version.

#define BITMAP_SIZE 128 // 16MiB maximum

typedef struct _pmm_state_t {
    phys_bytes base;
    size_t npages; // total pages tracked
    uint32_t page_bitmap[BITMAP_SIZE];
} pmm_state_t;

static pmm_state_t p_state = { 0 };

#define WORD_BITS (32u)
#define NUM_WORDS ((p_state.npages + WORD_BITS - 1) / WORD_BITS)

// Returns bit index into page_bitmap from physical address
static inline size_t pfn_from_pa(phys_bytes pa) {
    return (pa - p_state.base) / PAGE_SIZE;
}

// Returns physical address from bit index into page_bitmap
static inline phys_bytes pa_from_pfn(size_t pfn) {
    return p_state.base + pfn * PAGE_SIZE;
}

static inline phys_bytes pmm_end(void)
{
    return p_state.base + (phys_bytes)(p_state.npages * PAGE_SIZE);
}

static inline bool pmm_clamp_range(phys_bytes *start, phys_bytes *end)
{
    const phys_bytes limit = pmm_end();

    if (*end <= p_state.base || *start >= limit) {
        return false;
    }
    if (*start < p_state.base) {
        *start = p_state.base;
    }
    if (*end > limit) {
        *end = limit;
    }
    return *end > *start;
}

/* --- bit operations --- */

static inline uint32_t pmm_mask_msbfirst(size_t pfn)
{
    // PFN within its 32-bit word: 0..31
    const uint32_t k = (uint32_t)(pfn & (WORD_BITS - 1));
    // MSB-first mapping: k=0 -> bit 31, k=31 -> bit 0
    return 1u << (31u - k);
}

static inline void pmm_set_free(size_t pfn)
{
    uint32_t *word = &p_state.page_bitmap[pfn >> 5];
    uint32_t mask  = pmm_mask_msbfirst(pfn);

    if ((*word & mask) != 0) {
        return;
    }
    *word |= mask;
}

static inline void pmm_set_free_or_trap(size_t pfn)
{
    uint32_t *word = &p_state.page_bitmap[pfn >> 5];
    uint32_t mask  = pmm_mask_msbfirst(pfn);

    if ((*word & mask) != 0) {
        __builtin_trap(); // already free (double-free)
    }
    *word |= mask;
}

static inline void pmm_clear_free(size_t pfn)
{
    uint32_t *word = &p_state.page_bitmap[pfn >> 5];
    uint32_t mask  = pmm_mask_msbfirst(pfn);

    if ((*word & mask) == 0) {
        return;
    }
    *word &= ~mask;
}

static inline void pmm_clear_free_or_trap(size_t pfn)
{
    uint32_t *word = &p_state.page_bitmap[pfn >> 5];
    uint32_t mask  = pmm_mask_msbfirst(pfn);

    if ((*word & mask) == 0) {
        __builtin_trap(); // already allocated (double-alloc)
    }
    *word &= ~mask;
}

static inline int first_set_bit_msbfirst_u32(uint32_t word)
{
    if (!word) return -1;
    return __builtin_clz(word);
}

/*
static void pmm_init_all_allocated(void)
{
    for (size_t i = 0; i < NUM_WORDS; i++)
    {
        p_state.page_bitmap[i] = 0u;
    }
}
*/

static void pmm_init_all_free(void)
{
    LOG_T("pmm_init_all_free()\n");
    for (size_t i = 0; i < NUM_WORDS; i++)
    {
        p_state.page_bitmap[i] = ~0u;
    }
}

/*
Mark a range of pages as free.
start_pfn: page frame number
count: number of pages
*/
static void pmm_free_range(size_t start_pfn, phys_pages count)
{
    if (start_pfn >= p_state.npages) return;
    if (count > (p_state.npages - start_pfn)) {
        count = p_state.npages - start_pfn;
    }
    for (size_t pfn = start_pfn; pfn < start_pfn + count; pfn++)
    {
        pmm_set_free(pfn);
    }
}

/*
Mark a range of pages as allocated.
*/
static void pmm_alloc_range(size_t start_pfn, phys_pages count)
{
    if (start_pfn >= p_state.npages) return;
    if (count > (p_state.npages - start_pfn)) {
        count = p_state.npages - start_pfn;
    }
    for (size_t pfn = start_pfn; pfn < start_pfn + count; pfn++)
    {
        pmm_clear_free(pfn);
    }
}

static void print_bitmap()
{
    for (uint32_t i = 0; i < NUM_WORDS; i++)
    {
        printk("%.32b %08lx\n", p_state.page_bitmap[i], p_state.page_bitmap[i]);
    }
}

/*
Allocate 1 page.
Returns:
    physical address (multiple of PAGE_SIZE) on success
    0xFFFFFFFF on failure
*/
phys_bytes pmm_alloc_page(void)
{
    for (size_t w = 0; w < NUM_WORDS; w++) {
        uint32_t word = p_state.page_bitmap[w];
        if (word == 0) continue;

        int bit = first_set_bit_msbfirst_u32(word);
        if (bit < 0) continue;
        
        size_t pfn = w * WORD_BITS + (size_t)bit;
        if (pfn >= p_state.npages) break;

        pmm_clear_free_or_trap(pfn);
        return pa_from_pfn(pfn);
    }
    return 0xFFFFFFFFu;
}

/*
Free 1 page by physical address.
phys_addr: Physical address of page frame (must be aligned!)
*/
void pmm_free_page(phys_bytes phys_addr)
{
    if ((phys_addr & (PAGE_SIZE - 1u)) != 0u) __builtin_trap();
    if (phys_addr < p_state.base || phys_addr >= pmm_end()) __builtin_trap();

    size_t pfn = pfn_from_pa(phys_addr);
    pmm_set_free_or_trap(pfn);
}

static inline phys_bytes align_up(phys_bytes addr, phys_bytes align)
{
    return (addr + (align - 1)) & ~(align - 1);
}

static inline phys_bytes align_down(phys_bytes addr, phys_bytes align)
{
    return addr & ~(align - 1);
}

void pmm_release_range(phys_bytes base, phys_bytes size)
{
    LOG_T("Base=0x%08lx Size=0x%08lx\n", base, size);

    phys_bytes start = align_up(base, PAGE_SIZE);
    phys_bytes end   = align_down(base + size, PAGE_SIZE);
    if (!pmm_clamp_range(&start, &end)) {
        return;
    }

    const size_t start_pfn = pfn_from_pa(start);
    const phys_pages count = (end - start) / PAGE_SIZE;
    LOG_T("pmm_free_range(start_pfn=0x%lx, count=0x%lx)\n", start_pfn, count);
    pmm_free_range(start_pfn, count);
}

void pmm_reserve_range(phys_bytes base, phys_bytes size)
{
    LOG_T("Base=0x%08lx Size=0x%08lx\n", base, size);

    phys_bytes start = align_down(base, PAGE_SIZE);
    phys_bytes end   = align_up(base + size, PAGE_SIZE);
    if (!pmm_clamp_range(&start, &end)) {
        return;
    }

    const size_t start_pfn = pfn_from_pa(start);
    const phys_pages count = (end - start) / PAGE_SIZE;
    LOG_T("pmm_alloc_range(0x%08lx, 0x%08lx)\n", start_pfn, count);
    pmm_alloc_range(start_pfn, count);
}

void pmm_init(phys_bytes base, phys_bytes size)
{
    p_state.base   = base;
    p_state.npages = size / PAGE_SIZE;

    LOG_T("base=0x%08lx npages=%u\n", p_state.base, p_state.npages);

    const phys_bytes bitmap_size = NUM_WORDS * sizeof(uint32_t);
    if (bitmap_size > BITMAP_SIZE * sizeof(uint32_t)) {
        LOG_E("Bitmap not large enough for %d pages\n", p_state.npages);
        __builtin_trap();
    }

    // Initialize the bitmap to known state
    pmm_init_all_free();
}

static int popcount(uint32_t n) {
    int c = 0;
    for (size_t i = 0; i < 32; i++) {
        if ((n & (1u << i)) != 0) c++;
    }
    return c;
}

void pmm_print_free_mem(void)
{
    int free_pages = 0;
    for (size_t i = 0; i < NUM_WORDS; i++)
    {
        free_pages += popcount(p_state.page_bitmap[i]);
    }
    uint32_t free = free_pages * PAGE_SIZE;
    LOG("%d (0x%08lx) bytes free\n", free, free);
    print_bitmap();
}

/* -------------------- Virtual Memory Management --------------------------- */

/* root/pointer/page table allocation */

typedef enum {
    PTBLK_512 = 512,
    PTBLK_256 = 256,
} ptblk_t;

typedef struct pt_pool_page {
    struct pt_pool_page *next;  // 4KiB-aligned base
    ptblk_t ty;
    phys_bytes pa;
    uint16_t free_mask;         // bit=1 => free block (use 8 bits for 512, 16 bits for 256)
} pt_pool_page_t;

static pt_pool_page_t g_pool_pages[64];

typedef struct pt_pool {
    size_t index;   // points to next free pt_pool_page_t in array
    pt_pool_page_t *free_512;
    pt_pool_page_t *free_256;
} pt_pool_t;

static pt_pool_t g_ptpool = {
    .index    = 0,
    .free_512 = NULL,
    .free_256 = NULL,
};

// Get a node with free slots, allocate a new page if none are found.
static inline pt_pool_page_t* pool_get_node(ptblk_t ty)
{
    pt_pool_page_t** pp = (ty == PTBLK_256)
        ? &g_ptpool.free_256
        : &g_ptpool.free_512;
    
    while (*pp != NULL) {
        pt_pool_page_t *node = *pp;
        if (node->ty != ty) {
            LOG_E("Invalid node type in the %d list!\n", ty);
            __builtin_trap();
        }
        if (node->free_mask != 0) {
            return node;
        }
        pp = &node->next;
    }

    // TODO: backing page stays owned by pool forever.
    //       we can free pages when free_mask becomes all-ones
    phys_bytes pa = pmm_alloc_page();
    if (pa == PMM_INVALID_PA) {
        LOG_E("Failed to allocate page for pt_pool_page_t!\n");
        __builtin_trap();
    }

    // Check that we haven't exhausted the buffer
    if (g_ptpool.index >= ARRAY_LEN(g_pool_pages)) {
        LOG_E("Ran out of `pt_pool_page_t`s!\n");
        __builtin_trap();
    }

    pt_pool_page_t *new = &g_pool_pages[g_ptpool.index++];
    new->pa = pa;
    new->ty = ty;
    new->free_mask = (ty == PTBLK_256)
        ? 0xFFFF
        : 0x00FF;
    new->next = *pp;
    *pp = new;
    return new;
}

static void print_pool_page(pt_pool_page_t* p)
{
    printk("pt_pool_page_t at %08lx\n", (uint32_t)(uintptr_t)p);
    printk("    pa=%08lx\n", p->pa);
    printk("  free=%016b\n", (uint32_t)p->free_mask);
    printk("  next=%08lx\n", (uint32_t)(uintptr_t)p->next);
}

static void print_ptpool(void)
{
    pt_pool_page_t** pp = &g_ptpool.free_256;

    printk("256...%08lx\n", (uint32_t)(uintptr_t)pp);
    while (*pp != NULL) {
        pt_pool_page_t *node = *pp;
        print_pool_page(node);
        pp = &node->next;
    }

    pp = &g_ptpool.free_512;
    printk("512...%08lx\n", (uint32_t)(uintptr_t)pp);
    while (*pp != NULL) {
        pt_pool_page_t *node = *pp;
        print_pool_page(node);
        pp = &node->next;
    }
}

static inline void pool_clear_block_mem(const phys_bytes slot_pa, const ptblk_t ty)
{
    // Clear (byte_size / 4) u32s
    LOG_T("0x%08lx size=%d\n", slot_pa, ty);
    uint32_t* va = (uint32_t*)(uintptr_t)phys_to_virt(slot_pa);
    for (size_t i = 0; i < ((size_t)ty / 4); i++)
    {
        va[i] = 0;
    }
}

static inline phys_bytes pool_alloc_block_from_node(pt_pool_page_t* const node, const ptblk_t ty)
{
    // find an index for any 1 bit
    size_t limit = (ty == PTBLK_256)
        ? 16
        :  8;
    for (size_t i = 0; i < limit; i++)
    {
        // Loop until we find a 1 bit
        if ((node->free_mask & (uint16_t)(1u << i)) == 0)
            continue;

        // `i` is now the index
        node->free_mask &= ~(1 << i); // clear the bit
        const phys_bytes pa = node->pa + i * ty;
        pool_clear_block_mem(pa, ty);
        LOG_T("%08lx\n", pa);
        return pa;
    }
    return PMM_INVALID_PA;
}

static inline void pool_free_block_from_node(const phys_bytes pa, const ptblk_t ty)
{
    if ((pa & (ty - 1)) != 0) {
        LOG_E("Tried to free %d block for misaligned pa=%08lx\n", ty, pa);
        __builtin_trap();
    }

    const size_t limit = (ty == PTBLK_256)
        ? 16
        :  8;

    phys_bytes base = pa & PAGE_ADDR_MASK;
    for (size_t i = 0; i < g_ptpool.index; i++)
    {
        pt_pool_page_t *node = &g_pool_pages[i];

        if (node->pa != base || node->ty != ty)
            continue;

        LOG_T("Check that (pa=%08lx - node->pa=%08lx) %% block_size=%d == 0\n", pa, node->pa, ty);
        if ((pa - node->pa) % ty != 0) {
            LOG_E("Tried to free misaligned block!\n");
            __builtin_trap();
        }

        const size_t slot = (pa - node->pa) / (size_t)ty;
        if (slot < limit) {
            node->free_mask |= (1 << slot);
            return;
        } else {
            LOG_E("Tried to free %d slot #%d out of range\n", ty, slot);
            __builtin_trap();
        }
    }
    LOG_E("Failed to find allocated %d block for pa=%08lx\n", ty, pa);
    __builtin_trap();
}

static phys_bytes pt_alloc_table_512_phys(void)
{
    pt_pool_page_t *node = pool_get_node(PTBLK_512);
    return pool_alloc_block_from_node(node, PTBLK_512);
}

static phys_bytes pt_alloc_table_256_phys(void)
{
    pt_pool_page_t *node = pool_get_node(PTBLK_256);
    return pool_alloc_block_from_node(node, PTBLK_256);
}

static void pt_free_table_512_phys(const phys_bytes pa)
{
    pool_free_block_from_node(pa, PTBLK_512);
}

static void pt_free_table_256_phys(const phys_bytes pa)
{
    pool_free_block_from_node(pa, PTBLK_256);
}

typedef uint32_t desc_t;

typedef struct vm_space {
    phys_bytes root_pa; // physical address of root table
} vm_space_t;

static inline desc_t *ptr_table_va_from_desc(desc_t d)
{
    phys_bytes table_pa = (phys_bytes)(d & RPTABLE_ALIGN_MASK);
    return (desc_t*)(uintptr_t)phys_to_virt(table_pa);
}

static inline desc_t *pg_table_va_from_desc(desc_t d)
{
    phys_bytes table_pa = (phys_bytes)(d & PGTABLE_ADDR_MASK);
    return (desc_t*)(uintptr_t)phys_to_virt(table_pa);
}

static desc_t *vm_ensure_ptr_table(vm_space_t *as, virt_bytes va)
{
    desc_t *root = (desc_t*)(uintptr_t)phys_to_virt(as->root_pa);
    const uint32_t ri = ROOT_INDEX(va);

    desc_t d = root[ri];
    if (!desc_is_table(d)) {
        LOG_T("requesting new pointer table...\n");
        phys_bytes pa = pt_alloc_table_512_phys();

        // Root/pointer tables are 512 bytes, require bits 8..0 == 0.
        if ((pa & ~RPTABLE_ALIGN_MASK) != 0)
        {
            LOG_E("table is not aligned! (%08lx)\n", pa);
            __builtin_trap();
        }

        root[ri] = mk_ptr_table_desc((uint32_t)pa, PTE_ACCESSED);
        d = root[ri];
    }
    return ptr_table_va_from_desc(d);
}

static desc_t *vm_ensure_page_table(vm_space_t *as, virt_bytes va)
{
    desc_t *ptr = vm_ensure_ptr_table(as, va);
    const uint32_t pi = PTR_INDEX(va);

    desc_t d = ptr[pi];
    if (!desc_is_table(d)) {
        LOG_T("(%08lx) %d requesting new page table...\n", d, desc_is_table(d));
        phys_bytes pa = pt_alloc_table_256_phys();

        // Page tables are 256 bytes, require bits 7..0 == 0.
        if ((pa & ~PGTABLE_ALIGN_MASK) != 0)
        {
            LOG_E("page table is not aligned! (%08lx)\n", pa);
            __builtin_trap();
        }

        ptr[pi] = mk_pg_table_desc((uint32_t)pa, PTE_ACCESSED);
        d = ptr[pi];
    }
    return pg_table_va_from_desc(d);
}

/*
 * Returns pointer to the leaf PTE slot for `va`
 * Allocates intermediate tables as needed.
 */
static desc_t *vm_walk_create(vm_space_t *as, virt_bytes va)
{
    desc_t *page = vm_ensure_page_table(as, va);
    return &page[PAGE_INDEX(va)];
}

/*
 * Map one 4KiB page.
 *
 * - `as` gives the root table
 * - `va` and `pa` must be page-aligned
 * - `pte_flags` is attributes only (no DESC_TYPE_* bits)
 */
void vm_map_page(vm_space_t *as, virt_bytes va, phys_bytes pa, uint32_t pte_flags)
{
    LOG_T("as->root_pa=%08lx va=%08lx pa=%08lx flags=%08lx\n",
        as->root_pa,
        va,
        pa,
        pte_flags);

    if ((va & (PAGE_SIZE - 1u)) != 0u) {
        LOG_E("Virtual address not aligned!\n");
        __builtin_trap();
    }
    if ((pa & (PAGE_SIZE - 1u)) != 0u) {
        LOG_E("Physical address not aligned!\n");
        __builtin_trap();
    }

    desc_t *pte = vm_walk_create(as, va);

    //TODO: decide policy:
    //        - trap on conflict
    //        - allow identical remap
    //        - allow overwrite?
    if (desc_is_page(*pte)) {
        LOG_E("Descriptor is page! %08lx\n", (uint32_t)*pte);
        __builtin_trap();
    } else if (*pte != 0) {
        LOG_E("PTE non-zero but not a page descriptor!\n");
        // Non-zero but not a page desc
        __builtin_trap();
    }

    *pte = mk_page_desc((uint32_t)pa, pte_flags);
}

void vm_init(phys_bytes base, phys_bytes size, virt_bytes load_base)
{
    LOG("base=%08lx size=%08lx load_base=%08lx\n", base, size, load_base);
    vm_space_t as = { 0 };

    LOG_T("Requesting new root table...\n");
    as.root_pa = pt_alloc_table_512_phys();
    LOG_T("got 0x%08lx\n", as.root_pa);
    if ((as.root_pa & ~RPTABLE_ALIGN_MASK) != 0) {
        LOG_E("Root table isn't aligned properly!\n");
        __builtin_trap();
    }

    const size_t page_count = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;
    for (size_t i = 0; i < page_count; i++)
    {
        const uint32_t offset = (uint32_t)(i * PAGE_SIZE);
        vm_map_page(&as, load_base + offset, base + offset, KERNEL_PTE_FLAGS);
    }

    mmu_print_040((uint32_t*)(uintptr_t)phys_to_virt(as.root_pa));
    print_ptpool();

    // Set SRP to the new root pointer
    __asm__ __volatile__ (
        "nop                \n\t"
        "cinva      %%bc    \n\t"
        "nop                \n\t"
        "pflusha            \n\t"
        "nop                \n\t"
        "movec      %0,%%srp\n\t"
        "nop                \n\t"
        "cinva      %%bc    \n\t"
        "nop                \n\t"
        "pflusha            \n\t"
        :
        : "a"(as.root_pa)
        : "cc", "memory"
    );
}
