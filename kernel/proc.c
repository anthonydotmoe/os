#include "proc.h"
#include "kernel/printk.h"

void switch_to_user(void)
{
    // This function is called the instant before proc_ptr is
    // to be scheduled again
    while (1) { }
}

void do_ipc(struct proc *p)
{
    (void)p;
    printk("I got an IPC call!\n");
    printk("Call Number: %d\n", p->p_reg.d[0]);
    printk("Endpoint: %08lx\n", p->p_reg.d[1]);
    printk("Message Ptr: %08lx\n", p->p_reg.d[2]);
    while (1) { }
}