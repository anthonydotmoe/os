#pragma once

// Called very early (before allocator, before interrupts)
// Must be safe in early boot context; may be polled and slow.
void earlycon_putc(int ch);
