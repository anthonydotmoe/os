#include "system.h"
#include "kernel/printk.h"
#include "proc.h"

// syscall handling

void kernel_call(struct proc *p)
{
    (void)p;
    printk("I got a kernel call!\n");
    printk("Call Number: %d\n", p->p_reg.d[0]);
    printk("Message Ptr: %08lx\n", p->p_reg.d[2]);
    while(1) { }
}