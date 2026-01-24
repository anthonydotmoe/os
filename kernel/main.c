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
 */

//TODO: Describe what head.S has arranged

void __init __attribute__((__noreturn__)) start_kernel(void)
{
    printk("start_kernel big printing\n");

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
                printk("%s: too many memory chunks\n", __func__);
            }
        default:
            printk("%s: unknown tag 0x%04x ignored\n", __func__);
        }

        record = (struct bi_record*)((unsigned long)record + size);
    }
}

static void __init setup(char **cmdline_p)
{
    /* The bootinfo is located right after the kernel */
    parse_bootinfo((const struct bi_record*)_end);
}
