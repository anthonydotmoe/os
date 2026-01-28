#include <stdint.h>
#include <stddef.h>

#include "asm/init.h"
#include "asm/sections.h"
#include "arch/boot.h"
#include "asm/bootinfo.h"
#include "asm/bootinfo-a68040pc.h"
#include "kernel/early_alloc.h"
#include "kernel/printk.h"

#include "head.h"
#include "mm.h"
#include "exception.h"

// filled in by head.S
unsigned long bi_machtype;
unsigned long bi_cputype;
unsigned long bi_fputype;
unsigned long bi_mmutype;

static struct boot_params params __initdata = { 0 };

/*
    head.S has arranged the following:
    - Stack pointer is set to _stext (1KiB total)
    - MMU is enabled and we are mapped in at vaddrs
    - The first `init_mapped_size` bytes are available to us
    - `availmem` has been set to first free physical page address
*/
void __init arch_early_init(void)
{
    // Set up early alloc
    // Hack: boot_params() here to not be dependent on when the kernel calls it
    const struct boot_params* p = boot_params();
    for (unsigned int i = 0; i < p->nranges; i++) {
        LOG("Adding memory %.8x size: %.8x\n", p->ranges[i].addr, p->ranges[i].size);
        ea_add_memory(p->ranges[i].addr, p->ranges[i].size);
        kputchar('\n');
    }

    // Set up the `phys_to_virt`/`virt_to_phys` functions
    mm_init_offset();

    /* Seed the early allocator with the memory regions we know about */
    // 
    // Reserve kernel memory regions
    /*
    Here's what I'm thinking:
    1. Reserve [virt_to_phys(_stext) , availmem)
    1. Allocate room for kernel page tables
    1. Re-do the initial mapping in the new space
    1. Switch over to the new mappings
        - Now [__init_end , availmem) can be freed (the old mappings were there)
    1. Whenever the kernel sets up the real allocator, [__init_start,__init_end)
       can be freed as well.
    */

    // TODO: I need to somehow not allow allocations in the first page for now.
    // But after the page allocator is up, it is okay to do that as long as they
    // aren't mapped to the first page [0x0 - 0x1000)
    phys_addr_t base = virt_to_phys(0);
    phys_addr_t size = (phys_addr_t)availmem - base;
    LOG("Reserving memory at %.8x size: %.8x\n", base, size);
    ea_reserve(base, size);

    traps_init();

    mm_init();
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

const struct boot_params *boot_params(void)
{
    parse_bootinfo((const struct bi_record*)_end);
    return &params;
}
