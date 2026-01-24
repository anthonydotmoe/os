#pragma once
#include <stdint.h>

typedef uint32_t irq_t;
typedef void (*irq_handler_t)(irq_t irq, void *arg);

// Platform interrupt controller init.
void intr_init(void);

// Register a handler for a platform IRQ number.
int intr_register(irq_t irq, irq_handler_t handler, void *arg);

// Mask at the interrupt controller
void intr_mask(irq_t irq);

// Unmask at the interrupt controller
void intr_unmask(irq_t irq);

// Called by the low-level interrupt entry after determining "which irq fired"
void intr_dispatch(irq_t irq);
