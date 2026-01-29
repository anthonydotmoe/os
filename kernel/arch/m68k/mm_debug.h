#pragma once

#define DEBUG

#ifdef DEBUG
#include <stdint.h>

void mmu_print_040(uint32_t *root);
#else
#define mmu_print_040(x)    ;
#endif