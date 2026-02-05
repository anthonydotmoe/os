#include <stdint.h>

#include "arch/uart68681.h"
#include "arch/earlycon.h"

// Hard-coded DUART at 0x2000_0000 for now

void earlycon_putc(int ch)
{
    char c = (char)ch;

    if (c == '\n')
        earlycon_putc('\r');
    
    uint8_t status_val;
    do {
        status_val = uart->sra;
    } while (!(status_val & (1 << 2)));

    uart->tba = (uint8_t)c;
}
