#pragma once

#include <stdint.h>
#include <form_os/type.h>

#define PMM_INVALID_PA  0xFFFFFFFFu

// Set up the `virt_to_phys()` and `phys_to_virt()` functions
void mm_init_offset(void);

// Set up physical memory management
void pmm_init(phys_bytes base, phys_bytes size);

// Reserve a region of physical memory
void pmm_reserve_range(phys_bytes base, phys_bytes size);

// Release a region of physical memory
void pmm_release_range(phys_bytes base, phys_bytes size);

void pmm_print_free_mem(void);

void vm_init(phys_bytes base, phys_bytes size, virt_bytes load_base);

void mm_init(void);
