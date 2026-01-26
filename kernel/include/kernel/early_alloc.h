#pragma once

#include "kernel/mm.h"

// Add usable memory to the allocator. Architecture-specific startup code
// has this responibility.
void ea_add_memory(phys_addr_t base, phys_addr_t size);

// Reserve unallocatable space.
void ea_reserve(phys_addr_t base, phys_addr_t size);

// Allocate from usable memory.
phys_addr_t ea_alloc(phys_addr_t size, phys_addr_t align,
                     phys_addr_t min_addr, phys_addr_t max_addr);
