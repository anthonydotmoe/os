#include <stdint.h>

#include "kernel/mm.h"
#include "kernel/printk.h"
#include "mm.h"
#include "pgtable.h"
#include "head.h"
#include "asm/sections.h"

static int32_t memoffset;

void mm_init()
{
    memoffset = (int32_t)phys_kernel_start - PAGE_OFFSET;
}

phys_addr_t virt_to_phys(virt_addr_t va)
{
    return (phys_addr_t)(va + memoffset);
}

virt_addr_t phys_to_virt(phys_addr_t pa)
{
    return (virt_addr_t)(pa - memoffset);
}
