#include <stddef.h>
#include <stdint.h>

#include "kernel/printk.h"
#include "kernel/format.h"

static const char *k_iec_units[] = { "B", "KiB", "MiB", "GiB" };

void format_bytes_iec_1dp(uint32_t bytes, char *out, size_t out_sz)
{
    uint32_t whole = bytes;
    uint32_t frac_tenths = 0;
    unsigned unit = 0;

    // Chose unit up to GiB
    while (unit < 3 && whole >= 1024) {
        uint32_t rem = whole & 1023u;   // remainder mod 1024
        whole >>= 10;                   // divide by 1024

        // Only compute fraction for the final chosen unit.
        // We'll compute tenths from the remainder at the last step.
        // If we continue scaling further, we overwrite it anyway.
        frac_tenths = (rem * 10u + 512u) >> 10; // rounded tenths
        unit++;
    }


    // Handle rounding carry, e.g., 1023 bytes -> 1.0 KiB ok,
    // but e.g. 1024*X + rem could round to X+1.0
    if (unit != 0 && frac_tenths >= 10) {
        whole += 1;
        frac_tenths = 0;

        // If whole hits 1024, promote unit (up to GiB)
        while (unit < 3 && whole >= 1024) {
            whole >>= 10;
            unit++;
        }
    }

    if (unit == 0) {
        // Bytes: no decimal
        (void)snprintk(out, out_sz, "%u %s", bytes, k_iec_units[0]);
        return;
    } else {
        (void)snprintk(out, out_sz, "%u.%u %s",
            whole, frac_tenths, k_iec_units[unit]);
    }
}
