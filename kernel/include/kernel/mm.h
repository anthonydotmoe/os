#pragma once

#include <form_os/type.h>

phys_bytes virt_to_phys(virt_bytes va);
virt_bytes phys_to_virt(phys_bytes pa);

/*
 * vm_space_t:
 * Opaque address space object used by VM to manage per-process address spaces.
 * Contents are architecture-defined.
 */
typedef struct vm_space vm_space_t;

// Initialize a VM space for a process
void vm_space_init_user(vm_space_t *vm);

// Map one page of memory into a VM space
// TODO: `pte_flags` should probably be generic as well
void vm_space_map_page(vm_space_t *vm, virt_bytes va, phys_bytes pa, uint32_t pte_flags);

// Clear a VM space
void vm_space_destroy(vm_space_t *vm);
