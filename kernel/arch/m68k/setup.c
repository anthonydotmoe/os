#include <stdint.h>

#include "init.h"
#include "sections.h"
#include "asm/bootinfo.h"
#include "asm/bootinfo-a68040pc.h"

// filled in by head.s
unsigned long bi_machtype;
unsigned long bi_cputype;
unsigned long bi_fputype;
unsigned long bi_mmutype;

static void __init parse_bootinfo(const struct bi_record *record)
{
    const struct bi_record *first_record = record;
    uint16_t tag;
}

void setup(char **cmdline_p)
{
    /* The bootinfo is located right after the kernel */
    parse_bootinfo((const struct bi_record*)_end);
}