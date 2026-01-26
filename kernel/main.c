#include <stdint.h>
#include <string.h>

#include "asm/init.h"
#include "asm/sections.h"
#include "asm/bootinfo.h"
#include "asm/bootinfo-a68040pc.h"
#include "arch/boot.h"
#include "arch/setup.h"
#include "kernel/printk.h"
#include "kernel/format.h"

// For formatting sizes
static char sizbuf[16];

/***
 
    === OS Entry Point ===
    - Call into architecture initialization code
    - Retrieve the boot_params from early boot
 */
void __init __attribute__((__noreturn__)) start_kernel(void)
{
    const struct boot_params *params;
    arch_early_init();
    params = boot_params();

    LOG_I("Welcome to the kernel!\n");

    // Demonstrate that boot_params works
    format_bytes_iec_1dp(params->ranges[0].size, sizbuf, sizeof(sizbuf) - 1);
    LOG("Got %s memchunk at 0x%08lx\n", sizbuf, params->ranges[0].addr);

    asm volatile ("trap #0" : : : "memory");

    //arch_halt();
    while (1) {
        asm volatile ("stop #0x2700" : : : "cc");
    }

    __builtin_unreachable();
}
