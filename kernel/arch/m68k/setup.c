#include <stdint.h>
#include <stddef.h>

#include "asm/init.h"
#include "asm/sections.h"
#include "arch/boot.h"
#include "asm/bootinfo.h"
#include "asm/bootinfo-a68040pc.h"
#include "kernel/printk.h"
#include "mm.h"

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
    /*
        TODO: Do initialization things
        - install vector table (VBR)
        - configure caching policy from
          head.S cachemode_pgtable, cachemode_supervisor
        - ...?
    */
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