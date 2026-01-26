#include <stdbool.h>
#include "kernel/printk.h"
#include "kernel/early_alloc.h"
#include "traps.h"

static char* vector_names_0[] = {
    /*  0 */ "(Reset Initial Interrupt Stack Pointer)",
    /*  1 */ "(Reset Initial Program Counter)",
    /*  2 */ "Access Fault",
    /*  3 */ "Address Error",
    /*  4 */ "Illegal Instruction",
    /*  5 */ "Integer Divide by Zero",
    /*  6 */ "CHK, CHK2 Instruction",
    /*  7 */ "FTRAPcc, TRAPcc, TRAPV Instructions",
    /*  8 */ "Privilege Violation",
    /*  9 */ "Trace",
    /* 10 */ "Line 1010 Emulator",
    /* 11 */ "Line 1111 Emulator",
    /* 12 */ NULL,
    /* 13 */ "Coprocessor Protocol Violation",
    /* 14 */ "Format Error",
    /* 15 */ "Unitialized Interrupt"
};

static char* vector_names_48[] = {
    /* 48 */ "Floating-Point Branch or Set on Unordered Condition",
    /* 49 */ "Floating-Point Inexact Result",
    /* 50 */ "Floating-Point Zero Divide",
    /* 51 */ "Floating-Point Underflow",
    /* 52 */ "Floating-Point Operand Error",
    /* 53 */ "Floating-Point Overflow",
    /* 54 */ "Floating-Point SNAN",
    /* 55 */ "Floating-Point Unimplemented Data Type",
    
    // Unused by 68040
    /* 56 */ "MMU Configuration Error",
    /* 57 */ "MMU Illegal Operation",
    /* 58 */ "MMU Access Level Violation",
};

static char* siz_str[] = {
    "byte",
    "word",
    "long word",
    "line"
};

static void print_sr(uint16_t sr)
{
    int  tr = (((int)sr) >> 14) & 0x3;
    int  im = (((int)sr) >>  8) & 0x7;
    bool su = (sr & (1u << 13)) !=  0;
    bool mi = (sr & (1u << 12)) !=  0;
    bool  x = (sr & (1u <<  4)) !=  0;
    bool  n = (sr & (1u <<  3)) !=  0;
    bool  z = (sr & (1u <<  2)) !=  0;
    bool  v = (sr & (1u <<  1)) !=  0;
    bool  c = (sr & (1u <<  0)) !=  0;

    printk("SR = %04lx T:%d I:%d %s %c%c%c%c%c\n",
        (uint32_t)sr,
        tr,
        im,
        su ? (mi ? "MSP" : "ISP") : "USP",
        x  ? 'X' : '-',
        n  ? 'N' : '-',
        z  ? 'Z' : '-',
        v  ? 'V' : '-',
        c  ? 'C' : '-'
    );
}

static void crack_vector_names(uint8_t vector_num)
{
    // Get vector number
    int num = (uint32_t)vector_num;
    printk("Vector %.2X: ", num);

    if (num < 16 && num != 12)
    {
        printk("%s\n", vector_names_0[num]);
    }

    if (
        (num >= 16 && num < 24) ||
         num == 12 ||
        (num >= 59 && num < 64))
    {
        printk("(Unassigned, Reserved)\n");
        return;
    }

    if (num == 24)
    {
        printk("Spurious Interrupt\n");
        return;
    }

    if (num >= 25 && num < 32)
    {
        printk("Level %d Interrupt Autovector\n", num - 24);
        return;
    }

    if (num >= 32 && num < 48)
    {
        printk("TRAP #%d Instruction", num - 32);
        return;
    }

    if (num >= 48 && num < 59)
    {
        printk("%s\n", vector_names_48[num - 48]);
        return;
    }

    if (num >= 64 && num < 256)
    {
        printk("User Defined Vector $%02X (%d)\n", num, num);
        return;
    }

    return;
}

static void print_access_fault(exc_fmt7_t* e)
{
    const uint32_t ea = e->ea;
    const uint16_t sw = e->ssw;

    bool cp  = (sw & (1u << 15)) != 0;
    bool cu  = (sw & (1u << 14)) != 0;
    bool ct  = (sw & (1u << 13)) != 0;
    bool cm  = (sw & (1u << 12)) != 0;
    bool ma  = (sw & (1u << 11)) != 0;
    bool atc = (sw & (1u << 10)) != 0;
    bool lk  = (sw & (1u <<  9)) != 0;
    bool rw  = (sw & (1u <<  8)) != 0;
    int size = (int)((sw >> 5) & 0x3);
    int tt   = (int)((sw >> 3) & 0x3);
    int tm   = (int)((sw >> 0) & 0x7);

    printk("Invalid %s %s to 0x%.8lx... ", siz_str[size], rw ? "read" : "write", ea);
    switch(tt) {
        case 0b01:
            printk("MOVE16 ");
            __attribute__((fallthrough));
        case 0b00: // Normal Access
            switch(tm) {
                case 0b000:
                    printk("Data Cache Push Access\n");
                    break;
                case 0b001:
                    printk("User Data Access\n");
                    break;
                case 0b010:
                    printk("User Code Access\n");
                    break;
                case 0b011:
                    printk("MMU Table Search Data Access\n");
                    break;
                case 0b100:
                    printk("MMU Table Search Code Access\n");
                    break;
                case 0b101:
                    printk("Supervisor Data Access\n");
                    break;
                case 0b110:
                    printk("Supervisor Code Access\n");
                    break;
                case 0b111:
                    printk("(Reserved)\n");
                    break;
                default:
                    break;
            }
            break;
        case 0b10:
            printk("Alternate Logical Function Code (%d) Access\n", tm);
            break;
        case 0b11:
            printk("Acknowledge Access\n");
            break;
        default:
    }

    if (cp)  { printk("Continuation of Floating-Point Post-Instruction Exception Pending\n"); }
    if (cu)  { printk("Continuation of Unimplemented Floating-Point Instruction Exception Pending\n"); }
    if (ct)  { printk("Continuation of Trace Exception Pending\n"); }
    if (cm)  { printk("Continuation of MOVEM Instruction Execution Pending\n"); }
    if (ma)  { printk("Misaligned Access\n"); }
    if (atc) { printk("ATC Fault\n"); }
    if (lk)  { printk("Locked Transfer (Read-Modify-Write)\n"); }
}

static void print_regs(saved_regs_t *r)
{
    printk("D0: %.8lx   A0: %.8lx\n", r->d0, r->a0);
    printk("D1: %.8lx   A1: %.8lx\n", r->d1, r->a1);
    printk("D2: %.8lx   A2: %.8lx\n", r->d2, r->a2);
    printk("D3: %.8lx   A3: %.8lx\n", r->d3, r->a3);
    printk("D4: %.8lx   A4: %.8lx\n", r->d4, r->a4);
    printk("D5: %.8lx   A5: %.8lx\n", r->d5, r->a5);
    printk("D6: %.8lx   A6: %.8lx\n", r->d6, r->a6);
    printk("D7: %.8lx\n", r->d7);
}

void ExceptionHandler_c(saved_regs_t *r)
{
    exc_frame_header_t *f = (exc_frame_header_t*)((uint8_t *)r + sizeof(*r));
    kputchar('\n');
    for (int i = 0; i < 80; i++)
    {
        kputchar('-');
    }
    printk("\n\n*** STOP: 0x%08x\n", f->pc);
    print_regs(r);
    print_sr(f->sr);
    printk("F $%.1x / ", (uint32_t)exc_format(f));
    crack_vector_names(exc_vector(f));

    if (f->fmtvec == FMTVEC(7, 2))
    {
        print_access_fault(exc_as_fmt7(f));
    }

    printk("\nContact your system administrator or technical support group.\n");

    while (1) {
        asm volatile ("stop #0x2700" : : : "cc");
    }
}

extern char ExceptionHandler[];
void traps_init(phys_addr_t evt_base)
{
    virt_addr_t evt = phys_to_virt(evt_base);

    // Get the logical address of the exception handler
    uint32_t handler = (uint32_t)(uintptr_t)(ExceptionHandler);
    
    LOG("Filling in vector table at: 0x%08x\n", evt);

    uint32_t* p = (uint32_t*)(uintptr_t)evt;
    for (int i = 0; i < 256; i++)
    {
        *p++ = handler;
    }

    // Set the VBR to the new table
    asm volatile (" movec %0,%/vbr" : : "r"(evt));
    LOG("Done.\n");
}
