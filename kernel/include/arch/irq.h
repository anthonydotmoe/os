#pragma once
#include <stdint.h>

struct irq_flags {
    uint16_t sr;
};

// Save current interrupt mask/state and disable interrupts
struct irq_flags arch_irq_save_disable(void);

// Restore the saved interrupt mask/state
void arch_irq_restore(struct irq_flags state);
