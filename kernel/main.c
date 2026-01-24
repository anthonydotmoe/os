#include <stdint.h>

#include "asm/init.h"
#include "arch/m68k/irq.h"
#include "asm/sections.h"
#include "asm/bootinfo.h"
#include "asm/bootinfo-a68040pc.h"
#include "printk.h"

// filled in by head.s
unsigned long bi_machtype;
unsigned long bi_cputype;
unsigned long bi_fputype;
unsigned long bi_mmutype;
extern unsigned long init_mapped_size;
extern unsigned long availmem;

#define NUM_MEMINFO 4
#define MAX_CMDLINE 255
int num_memory;
unsigned long memoffset;
struct mem_info memory[NUM_MEMINFO];
char cmdline[MAX_CMDLINE];

static void setup(char **cmdline_p) __init;

/***
 
    === OS Entry Point ===

    head.S has arranged the following:
    - Stack pointer is set to _stext (1KiB total)
    - MMU is enabled and we are mapped in at vaddrs
    - The first `init_mapped_size` bytes are available to us
    - `availmem` has been set to first free physical page address
 */
void __init __attribute__((__noreturn__)) start_kernel(void)
{
    LOG_I("Welcome to the kernel!\n");
    LOG("0x%08lx\n", bi_machtype);
    LOG("0x%08lx\n", bi_cputype);
    LOG("0x%08lx\n", bi_mmutype);
    LOG("0x%08lx\n", bi_fputype);
    LOG("init_mapped_size: 0x%08lx\n", init_mapped_size);
    LOG("availmem: 0x%08lx\n", availmem);

    char* cmdline;
    setup(&cmdline);

    die();

    __builtin_unreachable();
}

static void __init parse_bootinfo(const struct bi_record *record)
{
    const struct bi_record *first_record = record;
    uint16_t tag;

    while ((tag = record->tag) != BI_LAST)
    {
        int unknown = 0;
        const void *data = record->data;
        uint16_t size = record->size;

        LOG("got record id=0x%04x size=0x%04x\n", tag, size);

        switch(tag) {
        case BI_MACHTYPE:
        case BI_CPUTYPE:
        case BI_FPUTYPE:
        case BI_MMUTYPE:
            /* Already set up by head.S */
            break;
        
        case BI_MEMCHUNK:
            if (num_memory < NUM_MEMINFO)
            {
                const struct mem_info *m = data;
                memory[num_memory].addr = m->addr;
                memory[num_memory].size = m->size;
                num_memory++;
            } else
            {
                LOG("too many memory chunks\n");
            }
            break;
        default:
            LOG("unknown tag 0x%04x ignored\n", tag);
        }

        record = (struct bi_record*)((unsigned long)record + size);
    }
}

static void __init setup(char **cmdline_p)
{
    /* The bootinfo is located right after the kernel */
    parse_bootinfo((const struct bi_record*)_end);
}
