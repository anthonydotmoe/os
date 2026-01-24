#pragma once
#include <stdint.h>

#define BOOT_MAX_MEMRANGES  8
#define BOOT_MAX_CMDLINE    256

struct mem_range {
    uint32_t addr;
    uint32_t size;
};

struct boot_params {
    unsigned nranges;
    struct mem_range ranges[BOOT_MAX_MEMRANGES];
    char cmdline[BOOT_MAX_CMDLINE];
};

// Architecture dependent code will return a pointer to assembled boot_params,
// however it may get that put together is it's own duty.
const struct boot_params* boot_params(void);
