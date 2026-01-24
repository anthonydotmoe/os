#include "kernel/irq.h"
#include "arch/irq.h"

irq_flags_t irq_save(void)
{
    return arch_irq_save_disable();
}

void irq_restore(irq_flags_t flags)
{
    arch_irq_restore(flags);
}
