#pragma once

#include <stdint.h>

/*
 * User-mode CPU context saved in the process table.
 * This is not the variable-length exception frame.
 */
typedef struct {
    uint32_t d[8];   /* d0-d7 */
    uint32_t a[7];   /* a0-a6 */
    uint32_t usp;    /* user stack pointer (a7) */
    uint32_t pc;
    uint16_t sr;
    uint16_t pad0;   /* keep size 32-bit aligned */
} m68k_user_ctx_t;
