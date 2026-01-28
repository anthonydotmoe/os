#pragma once

// Called extremely early from start_kernel
// Seeds early allocator with memory regions
void arch_early_init(void);
