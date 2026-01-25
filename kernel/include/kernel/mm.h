#pragma once
#include <stdint.h>

typedef uint32_t phys_addr_t;
typedef uint32_t virt_addr_t;

phys_addr_t virt_to_phys(virt_addr_t va);
virt_addr_t phys_to_virt(phys_addr_t pa);
