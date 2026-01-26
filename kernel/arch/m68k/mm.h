#pragma once

#include "kernel/mm.h"

// Set up the `virt_to_phys()` and `phys_to_virt()` functions
void mm_init_offset(void);

void mm_init(void);
