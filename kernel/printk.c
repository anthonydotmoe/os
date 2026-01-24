#include "asm/init.h"
#include "printk.h"
#include "uart68681.h"

/** libprintf provides this:
 * printf with output function
 * You may use this as dynamic alternative to printf() with its fixed _putchar() output
 * \param out An output function which takes one character and an argument pointer
 * \param arg An argument pointer for user data passed to output function
 * \param format A string that specifies the format of the output
 * \return The number of characters that are sent to the output function, not counting the terminating null character
 */
extern int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...);

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
    const int ret = fctprintf(&duart_putchar, NULL, fmt, va);
    va_end(va);
    return ret;
}
