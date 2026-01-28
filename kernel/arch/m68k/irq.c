#include <stdint.h>
#include "arch/irq.h"

static inline uint16_t read_sr(void)
{
    uint16_t sr;
    __asm__ __volatile__ ("movew %%sr,%0" : "=d" (sr) : : "memory");
    return sr;
}

static inline void write_sr(uint16_t sr)
{
    __asm__ __volatile__ ("movew %0,%%sr" : : "d" (sr) : "memory");
}

struct irq_flags arch_irq_save_disable(void)
{
    struct irq_flags flags;
    flags.sr = read_sr();
    __asm__ __volatile__ ("oriw #0x0700,%%sr" : : : "cc", "memory");
    return flags;
}

void arch_irq_restore(struct irq_flags state)
{
    write_sr(state.sr);
}
