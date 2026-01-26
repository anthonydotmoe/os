#include <stdint.h>
#include <stdbool.h>
#include "kernel/early_alloc.h"
#include "kernel/mm.h"

// TODO: Tune this with more thought
#define MEM_CAP 32
#define RES_CAP 64

typedef struct _region_t {
    phys_addr_t base;
    phys_addr_t size;
} region_t;

typedef struct _region_list_t {
    region_t *r;
    uint32_t nr;
    uint32_t cap;
} region_list_t;

typedef struct _ea_state_t {
    region_list_t memory;
    region_list_t reserved;
} ea_state_t;

static region_t mem_regions[MEM_CAP] = { 0 };
static region_t res_regions[RES_CAP] = { 0 };

static ea_state_t state = {
    .memory   = { .r = mem_regions, .nr = 0, .cap = MEM_CAP },
    .reserved = { .r = res_regions, .nr = 0, .cap = RES_CAP },
};

/* --- helpers --- */

static inline phys_addr_t ea_min(phys_addr_t a, phys_addr_t b) { return a < b ? a : b; }
static inline phys_addr_t ea_max(phys_addr_t a, phys_addr_t b) { return a > b ? a : b; }

// Return true if base+size overflows
static inline bool add_overflow_phys(phys_addr_t base, phys_addr_t size, phys_addr_t *out_end)
{
    phys_addr_t end = base + size;
    if (end < base) {
        return true;
    }
    *out_end = end;
    return false;
}

// align must be power-of-two. If align == 0 treat as 1
static inline phys_addr_t align_up(phys_addr_t x, phys_addr_t align)
{
    if (align == 0) align = 1;
    return (x + (align - 1)) & ~(align - 1);
}

/*
static inline phys_addr_t align_down(phys_addr_t x, phys_addr_t align)
{
    if (align == 0) align = 1;
    return x & ~(align - 1);
}

static inline bool ranges_overlap_or_adjacent(phys_addr_t a0, phys_addr_t a1,
                                              phys_addr_t b0, phys_addr_t b1)
{
    // Treat adjacency as mergeable: [a0,a1) touches [b0,b1) if a1 == b0.
    return !(a1 < b0 || b1 < a0);
}
*/

// Because these are half-open intervals, overlap/adjacency condition is:
// prev_end >= cur_base
static inline bool mergeable(phys_addr_t prev_base, phys_addr_t prev_end,
                             phys_addr_t  cur_base, phys_addr_t  cur_end)
{
    (void)prev_base;
    (void)cur_end;
    return prev_end >= cur_base;
}

/* Insert [base,end) into list (base<end), keep sorted and merged */
static void region_list_insert_merge(region_list_t *l, phys_addr_t base, phys_addr_t end)
{
    // Find insertion index: first region with base > new base
    uint32_t i = 0;
    while (i < l->nr && l->r[i].base <= base) {
        i++;
    }

    // Ensure capacity for a new entry (worst-case: no merges)
    if (l->nr >= l->cap) {
        // Since it's early boot, fail hard.
        __builtin_trap();
    }

    // Shift up to make room
    for (uint32_t j = l->nr; j > i; j--) {
        l->r[j] = l->r[j - 1];
    }

    l->r[i].base = base;
    l->r[i].size = (phys_addr_t)(end - base);
    l->nr++;

    // Merge with neighbors (both directions)
    // First merge backwards if needed
    if (i > 0) {
        uint32_t p = i - 1;
        phys_addr_t p_base = l->r[p].base;
        phys_addr_t p_end;
        if (add_overflow_phys(p_base, l->r[p].size, &p_end))
        {
            __builtin_trap();
        }

        phys_addr_t c_base = l->r[i].base;
        phys_addr_t c_end;
        if (add_overflow_phys(c_base, l->r[i].size, &c_end))
        {
            __builtin_trap();
        }

        if (mergeable(p_base, p_end, c_base, c_end)) {
            // Merge current into previous
            phys_addr_t new_base = p_base;
            phys_addr_t new_end  = ea_max(p_end, c_end);

            l->r[p].base = new_base;
            l->r[p].size = (phys_addr_t)(new_end - new_base);

            // Remove current entry i.
            for (uint32_t j = i; j + 1 < l->nr; j++) {
                l->r[j] = l->r[j + 1];
            }
            l->nr--;
            i = p;
        }
    }

    // Then merge forward repeatedly
    while (i + 1 < l->nr) {
        phys_addr_t a_base = l->r[i].base;
        phys_addr_t a_end;
        if (add_overflow_phys(a_base, l->r[i].size, &a_end)) {
            __builtin_trap();
        }

        phys_addr_t b_base = l->r[i + 1].base;
        phys_addr_t b_end;
        if (add_overflow_phys(b_base, l->r[i + 1].size, &b_end)) {
            __builtin_trap();
        }

        if (!mergeable(a_base, a_end, b_base, b_end)) {
            break;
        }

        // Merge b into a
        phys_addr_t new_base = a_base;
        phys_addr_t new_end  = ea_max(a_end, b_end);

        l->r[i].base = new_base;
        l->r[i].size = (phys_addr_t)(new_end - new_base);

        // remove i+1
        for (uint32_t j = i + 1; j + 1 < l->nr; j++) {
            l->r[j] = l->r[j + 1];
        }
        l->nr--;
    }
}

// Common entry point for adding an interval to a list
static void region_list_add(region_list_t *l, phys_addr_t base, phys_addr_t size)
{
    if (size == 0) {
        return;
    }

    phys_addr_t end;
    if (add_overflow_phys(base, size, &end)) {
        __builtin_trap();
    }

    // Reject pathological "wrap to 0" case: size != 0 but end == base implies overflow
    if (end <= base) {
        __builtin_trap();
    }

    region_list_insert_merge(l, base, end);
}

// Find an allocation of `size` with `align` within [min_addr, max_addr),
// searching usable memory minus reserved. Returns true on success.
static bool find_free_range_bottom_up(phys_addr_t size, phys_addr_t align,
                                      phys_addr_t min_addr, phys_addr_t max_addr,
                                      phys_addr_t *out_base)
{
    if (size == 0) {
        return false;
    }
    if (align == 0) {
        align = 1;
    }
    if ((align & (align - 1)) != 0) {
        // enforce power-of-two alignment
        return false;
    }
    if (max_addr != 0 && max_addr <= min_addr) {
        return false;
    }

    // For each usable memory region...
    for (uint32_t mi = 0; mi < state.memory.nr; mi++) {
        phys_addr_t u_base = state.memory.r[mi].base;
        phys_addr_t u_end;
        if (add_overflow_phys(u_base, state.memory.r[mi].size, &u_end)) {
            __builtin_trap();
        }

        // Clamp usable region to requested window
        phys_addr_t w_base = ea_max(u_base, min_addr);
        phys_addr_t w_end  = u_end;

        if (max_addr != 0) {
            w_end = ea_min(w_end, max_addr);
        }

        if (w_end <= w_base) {
            continue;
        }

        // Sweep reserved regions that intersect [w_base, w_end)
        phys_addr_t cursor = w_base;

        for (uint32_t ri = 0; ri < state.reserved.nr; ri++) {
            phys_addr_t r_base = state.reserved.r[ri].base;
            phys_addr_t r_end;
            if (add_overflow_phys(r_base, state.reserved.r[ri].size, &r_end)) {
                __builtin_trap();
            }

            // Skip reserved ranges entirely below our window
            if (r_end <= cursor) {
                continue;
            }

            // If reserved starts beyond our window end,
            // we're done for this usable region
            if (r_base >= w_end) {
                break;
            }

            // Gap is [cursor, min(r_base, w_end))
            phys_addr_t gap_end = ea_min(r_base, w_end);
            if (gap_end > cursor) {
                phys_addr_t cand = align_up(cursor, align);
                phys_addr_t cand_end;

                if (!add_overflow_phys(cand, size, &cand_end) &&
                    cand >= cursor &&
                    cand_end <= gap_end)
                {
                    *out_base = cand;
                    return true;
                }
            }

            // Move cursor past this reserved region (clamped to window)
            phys_addr_t new_cursor = ea_max(cursor, r_end);
            cursor = new_cursor;

            if (cursor >= w_end) {
                break;
            }
        }

        // Tail gap: [cursor, w_end)
        if (cursor < w_end) {
            phys_addr_t cand = align_up(cursor, align);
            phys_addr_t cand_end;

            if (!add_overflow_phys(cand, size, &cand_end) &&
                cand >= cursor &&
                cand_end <= w_end)
            {
                *out_base = cand;
                return true;
            }
        }
    }

    return false;
}

/* --- Debugging helper --- */

#define DEBUG
#ifdef DEBUG
#include "kernel/printk.h"

static inline void ea_print_list(region_list_t *l)
{
    for (uint32_t i = 0; i < l->nr; i++)
    {
        phys_addr_t end = l->r[i].base + l->r[i].size;
        LOG("  [%.8lx , %.8lx)\n", l->r[i].base, end);
    }
}

static void ea_print(void)
{
    region_list_t *l;

    for (int i = 0; i < 80; i++)
    {
        kputchar('-');
    }
    kputchar('\n');

    LOG("Available:\n");
    l = &state.memory;
    ea_print_list(l);

    LOG("Reserved:\n");
    l = &state.reserved;
    ea_print_list(l);

    kputchar('\n');
}

#else
#define ea_print()
#endif /* DEBUG */

/* --- Public API --- */

void ea_add_memory(phys_addr_t base, phys_addr_t size)
{
    region_list_add(&state.memory, base, size);
    ea_print();
}

void ea_reserve(phys_addr_t base, phys_addr_t size)
{
    region_list_add(&state.reserved, base, size);
    ea_print();
}

phys_addr_t ea_alloc(phys_addr_t size,
                     phys_addr_t align,
                     phys_addr_t min_addr,
                     phys_addr_t max_addr)
{
    phys_addr_t base;
    if (!find_free_range_bottom_up(size, align, min_addr, max_addr, &base)) {
        ea_print();
        return ~(phys_addr_t)0; //TODO: sentinel failure value... I miss `Result<T>`
    }

    // Reserve it so future allocations don't reuse it.
    ea_reserve(base, size);
    return base;
}
