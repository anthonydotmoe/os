#include <stdarg.h>

#include "kernel/printk.h"
#include "arch/earlycon.h"

/** libprintf provides this: */
extern int vfctprintf(void (*out)(char, void*), void* arg, const char* fmt, va_list va);

static void printk_putchar(char c, void* arg)
{
    (void)arg;
    earlycon_putc((unsigned char)c);
}

int kputchar(int ch)
{
    earlycon_putc((unsigned char)ch);
    return (int)(unsigned char)ch;
}

int printk(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    const int ret = vfctprintf(&printk_putchar, NULL, fmt, va);
    va_end(va);
    return ret;
}
