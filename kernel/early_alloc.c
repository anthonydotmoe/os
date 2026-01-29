#include <stdint.h>
#include <stdbool.h>

#include <form_os/type.h>

#include "kernel/early_alloc.h"

// TODO: Tune this with more thought
#define MEM_CAP 32
#define RES_CAP 64

typedef struct _region_t {
    phys_bytes base;
    phys_bytes size;
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

static inline phys_bytes ea_min(phys_bytes a, phys_bytes b) { return a < b ? a : b; }
static inline phys_bytes ea_max(phys_bytes a, phys_bytes b) { return a > b ? a : b; }

// Return true if base+size overflows
static inline bool add_overflow_phys(phys_bytes base, phys_bytes size, phys_bytes *out_end)
{
    phys_bytes end = base + size;
    if (end < base) {
        return true;
    }
    *out_end = end;
    return false;
}

// align must be power-of-two. If align == 0 treat as 1
static inline phys_bytes align_up(phys_bytes x, phys_bytes align)
{
    if (align == 0) align = 1;
    return (x + (align - 1)) & ~(align - 1);
}

/*
static inline phys_bytes align_down(phys_bytes x, phys_bytes align)
{
    if (align == 0) align = 1;
    return x & ~(align - 1);
}

static inline bool ranges_overlap_or_adjacent(phys_bytes a0, phys_bytes a1,
                                              phys_bytes b0, phys_bytes b1)
{
    // Treat adjacency as mergeable: [a0,a1) touches [b0,b1) if a1 == b0.
    return !(a1 < b0 || b1 < a0);
}
*/

// Because these are half-open intervals, overlap/adjacency condition is:
// prev_end >= cur_base
static inline bool mergeable(phys_bytes prev_base, phys_bytes prev_end,
                             phys_bytes  cur_base, phys_bytes  cur_end)
{
    (void)prev_base;
    (void)cur_end;
    return prev_end >= cur_base;
}

/* Insert [base,end) into list (base<end), keep sorted and merged */
static void region_list_insert_merge(region_list_t *l, phys_bytes base, phys_bytes end)
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
    l->r[i].size = (phys_bytes)(end - base);
    l->nr++;

    // Merge with neighbors (both directions)
    // First merge backwards if needed
    if (i > 0) {
        uint32_t p = i - 1;
        phys_bytes p_base = l->r[p].base;
        phys_bytes p_end;
        if (add_overflow_phys(p_base, l->r[p].size, &p_end))
        {
            __builtin_trap();
        }

        phys_bytes c_base = l->r[i].base;
        phys_bytes c_end;
        if (add_overflow_phys(c_base, l->r[i].size, &c_end))
        {
            __builtin_trap();
        }

        if (mergeable(p_base, p_end, c_base, c_end)) {
            // Merge current into previous
            phys_bytes new_base = p_base;
            phys_bytes new_end  = ea_max(p_end, c_end);

            l->r[p].base = new_base;
            l->r[p].size = (phys_bytes)(new_end - new_base);

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
        phys_bytes a_base = l->r[i].base;
        phys_bytes a_end;
        if (add_overflow_phys(a_base, l->r[i].size, &a_end)) {
            __builtin_trap();
        }

        phys_bytes b_base = l->r[i + 1].base;
        phys_bytes b_end;
        if (add_overflow_phys(b_base, l->r[i + 1].size, &b_end)) {
            __builtin_trap();
        }

        if (!mergeable(a_base, a_end, b_base, b_end)) {
            break;
        }

        // Merge b into a
        phys_bytes new_base = a_base;
        phys_bytes new_end  = ea_max(a_end, b_end);

        l->r[i].base = new_base;
        l->r[i].size = (phys_bytes)(new_end - new_base);

        // remove i+1
        for (uint32_t j = i + 1; j + 1 < l->nr; j++) {
            l->r[j] = l->r[j + 1];
        }
        l->nr--;
    }
}

// Common entry point for adding an interval to a list
static void region_list_add(region_list_t *l, phys_bytes base, phys_bytes size)
{
    if (size == 0) {
        return;
    }

    phys_bytes end;
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
static bool find_free_range_bottom_up(phys_bytes size, phys_bytes align,
                                      phys_bytes min_addr, phys_bytes max_addr,
                                      phys_bytes *out_base)
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
        phys_bytes u_base = state.memory.r[mi].base;
        phys_bytes u_end;
        if (add_overflow_phys(u_base, state.memory.r[mi].size, &u_end)) {
            __builtin_trap();
        }

        // Clamp usable region to requested window
        phys_bytes w_base = ea_max(u_base, min_addr);
        phys_bytes w_end  = u_end;

        if (max_addr != 0) {
            w_end = ea_min(w_end, max_addr);
        }

        if (w_end <= w_base) {
            continue;
        }

        // Sweep reserved regions that intersect [w_base, w_end)
        phys_bytes cursor = w_base;

        for (uint32_t ri = 0; ri < state.reserved.nr; ri++) {
            phys_bytes r_base = state.reserved.r[ri].base;
            phys_bytes r_end;
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
            phys_bytes gap_end = ea_min(r_base, w_end);
            if (gap_end > cursor) {
                phys_bytes cand = align_up(cursor, align);
                phys_bytes cand_end;

                if (!add_overflow_phys(cand, size, &cand_end) &&
                    cand >= cursor &&
                    cand_end <= gap_end)
                {
                    *out_base = cand;
                    return true;
                }
            }

            // Move cursor past this reserved region (clamped to window)
            phys_bytes new_cursor = ea_max(cursor, r_end);
            cursor = new_cursor;

            if (cursor >= w_end) {
                break;
            }
        }

        // Tail gap: [cursor, w_end)
        if (cursor < w_end) {
            phys_bytes cand = align_up(cursor, align);
            phys_bytes cand_end;

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

static void iter_region_list(region_list_t *l, ea_range_cb cb, void *ctx)
{
    for (uint32_t i = 0; i < l->nr; i++)
    {
        cb(l->r[i].base, l->r[i].size, ctx);
    }
}

/* --- Debugging helper --- */

#define DEBUG
#ifdef DEBUG
#include "kernel/printk.h"

static inline void ea_print_list(region_list_t *l)
{
    for (uint32_t i = 0; i < l->nr; i++)
    {
        phys_bytes end = l->r[i].base + l->r[i].size;
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

void ea_add_memory(phys_bytes base, phys_bytes size)
{
    region_list_add(&state.memory, base, size);
    ea_print();
}

void ea_reserve(phys_bytes base, phys_bytes size)
{
    region_list_add(&state.reserved, base, size);
    ea_print();
}

phys_bytes ea_alloc_or_panic(phys_bytes size,
                     phys_bytes align,
                     phys_bytes min_addr,
                     phys_bytes max_addr)
{
    phys_bytes base;
    if (!find_free_range_bottom_up(size, align, min_addr, max_addr, &base)) {
        LOG_E("failed for size=%08lx align=%08lx min_addr=%08lx max_addr=%08lx\n",
            size, align, min_addr, max_addr);
        ea_print();
        __builtin_trap();
    }

    // Reserve it so future allocations don't reuse it.
    ea_reserve(base, size);
    return base;
}

void ea_for_each_memory(ea_range_cb cb, void *ctx)
{
    iter_region_list(&state.memory, cb, ctx);
}

void ea_for_each_reserved(ea_range_cb cb, void *ctx)
{
    iter_region_list(&state.reserved, cb, ctx);
}
