#pragma once

#include "asm/init.h"

// Definitions from head.S

// Logical address of the kernel page table page.
extern char kernel_pg_dir[];

// Physical kernel load address
extern __initdata unsigned long phys_kernel_start;

// Size of memory already mapped
extern __initdata unsigned long init_mapped_size;

// Physical address of start of free memory
extern unsigned long availmem;