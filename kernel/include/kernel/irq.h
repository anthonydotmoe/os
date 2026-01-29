#pragma once

/*
 * irq_flags_t:
 * Opaque interrupt state saved by irq_save() and restored by irq_restore()
 * Contents are architecture-defined. Kernel code must not inspect it.
 */
typedef struct irq_flags irq_flags_t;

irq_flags_t irq_save(void);
void irq_restore(irq_flags_t flags);
