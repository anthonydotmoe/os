#pragma once

#include "arch/context.h"

struct arch_tcb {
    /* saved user-level context of thread */
    m68k_user_ctx_t tcbContext;
};
typedef struct arch_tcb arch_tcb_t;
