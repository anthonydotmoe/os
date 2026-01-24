#include "asm/init.h"
#include "printk.h"
#include "uart68681.h"

/** libprintf provides this: */
extern int vfctprintf(void (*out)(char, void*), void* arg, const char* fmt, va_list va);

// Hard-coded DUART at 0x2000_0000 for now

static void duart_putchar(char c, void* arg)
{
    (void)arg;

    // A little recursion
    if (c == '\n')
        duart_putchar('\r', NULL);

    uint8_t status_val;
    do {
        status_val = uart->sra;
    }
    while (!(status_val & (1 << 2)));

    uart->tba = c;
    return;
}

int __init printk(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    const int ret = vfctprintf(&duart_putchar, NULL, fmt, va);
    va_end(va);
    return ret;
}
