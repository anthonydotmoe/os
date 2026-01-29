#pragma once

#include <form_os/type.h>

// Add usable memory to the allocator. Architecture-specific startup code
// has this responibility.
void ea_add_memory(phys_bytes base, phys_bytes size);

// Reserve unallocatable space.
void ea_reserve(phys_bytes base, phys_bytes size);

// Allocate from usable memory.
phys_bytes ea_alloc_or_panic(phys_bytes size, phys_bytes align,
                             phys_bytes min_addr, phys_bytes max_addr);

/* --- For handing off to a real allocator --- */

typedef void (*ea_range_cb)(phys_bytes base, phys_bytes size, void *ctx);

// Iterate over all memory ranges
void ea_for_each_memory(ea_range_cb cb, void *ctx);

// Iterate over all reserved ranges
void ea_for_each_reserved(ea_range_cb cb, void *ctx);

// Seal the allocator
// This is done when the final allocator is ready to take over. Further accesses
// to the early allocator will panic
void ea_finalize(void);
