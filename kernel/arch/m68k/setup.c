#include <stdint.h>
#include <stddef.h>

#include <form_os/type.h>

#include "asm/init.h"
#include "asm/sections.h"
#include "arch/boot.h"
#include "asm/bootinfo.h"
#include "kernel/mm.h"
#include "kernel/printk.h"

#include "head.h"
#include "mm.h"
#include "exception.h"
#include "pgtable.h"

// filled in by head.S
unsigned long bi_machtype;
unsigned long bi_cputype;
unsigned long bi_fputype;
unsigned long bi_mmutype;

static bool init_params_inited __initdata = false;
static struct boot_params params __initdata = { 0 };

/*
    head.S has arranged the following:
    - MMU is enabled and we are mapped in at vaddrs
    - The first `init_mapped_size` bytes are available to us
    - `availmem` has been set to first free physical page address
*/
void __init arch_early_init(void)
{
    // Set up exception vector table
    traps_init();

    /* Seed the physical memory manager */
    // Hack: boot_params() here to not be dependent on when the kernel calls it
    const struct boot_params* p = boot_params();
    if (p->nranges == 0)
    {
        LOG_E("No memory ranges??\n");
        __builtin_trap();
    }

    // Set up the `phys_to_virt`/`virt_to_phys` functions
    mm_init_offset();

    // Give PMM the RAM space
    pmm_init(p->ranges[0].addr, p->ranges[0].size);

    // Reserve kernel image + early boot allocations up to availmem
    phys_bytes kbase = virt_to_phys((virt_bytes)(uintptr_t)_start_kernel_image);
    phys_bytes kend  = (phys_bytes)availmem;
    pmm_reserve_range(kbase, kend - kbase);

    // Build a new kernel page-table tree using PMM (not the boot bump area)
    // `vm_init` must switch SRP to the new tree before returning.
    // Map all of memory for simplicity
    phys_bytes mem_start = p->ranges[0].addr;
    phys_bytes mem_size  = p->ranges[0].size;
    virt_bytes v_base    = phys_to_virt(mem_start);
    vm_init(mem_start, mem_size, v_base);

    pmm_print_free_mem();

    // Now that we're not using the boot bump-allocated PT pages anymore,
    // free the bump region.
    phys_bytes old_pt_base = virt_to_phys((virt_bytes)(uintptr_t)_end);
    phys_bytes old_pt_end  = (phys_bytes)availmem;
    if (old_pt_end > old_pt_base) {
        pmm_release_range(old_pt_base, old_pt_end - old_pt_base);
    }
    pmm_print_free_mem();
}

static void __init parse_bootinfo(const struct bi_record *record)
{
    uint16_t tag;
    params.nranges = 0;

    while ((tag = record->tag) != BI_LAST)
    {
        const void *data = record->data;
        uint16_t size = record->size;

        switch(tag) {
        case BI_MACHTYPE:
        case BI_CPUTYPE:
        case BI_FPUTYPE:
        case BI_MMUTYPE:
            /* Already set up by head.S */
            break;
        
        case BI_MEMCHUNK:
            if (params.nranges < BOOT_MAX_MEMRANGES)
            {
                const struct mem_info *m = data;

                params.ranges[params.nranges].addr = m->addr;
                params.ranges[params.nranges].size = m->size;
                params.nranges++;
            } else
            {
                LOG("too many memory chunks\n");
            }
            break;
        
        case BI_COMMAND_LINE:
            // strcpy(cmdline, data);
            LOG("got command line: \"%s\"\n", (char*)data);
            break;
        default:
            //LOG("unknown tag 0x%04x ignored\n", tag);
        }

        record = (struct bi_record*)((unsigned long)record + size);
    }
}

static void __init init_boot_params(void)
{
    parse_bootinfo((const struct bi_record*)_end);
    init_params_inited = true;
}


const struct boot_params *boot_params(void)
{
    if (init_params_inited != true) {
        init_boot_params();
    }
    return &params;
}
