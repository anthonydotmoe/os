#pragma once

#include "arch/objects/structures.h"

struct tcb {
    arch_tcb_t tcbArch;

    /* previous and next pointers for scheduler queues */
    struct tcb *tcbSchedNext;
    struct tcb *tcbSchedPrev;
};
typedef struct tcb tcb_t;
