/*
This file contains a simple exception handler. Exceptions in user processes
are converted to signals. Exceptions in a kernel task cause a panic.
*/

#include <stdbool.h>
#include <stdint.h>

#include <form_os/type.h>

#include "kernel/printk.h"
#include "arch/exception.h"

typedef enum {
    K_SIG_NONE = 0,
    K_SIGILL,
    K_SIGFPE,
    K_SIGSEGV,
    K_SIGBUS,
    K_SIGTRAP,
    K_SIGSYS,
} ksig_t;

typedef struct {
    const char *msg;
    ksig_t sig;
} exc_info_t;

static const exc_info_t exc_info[256] = {
    [2]  = { "Access fault",                   K_SIGSEGV  },
    [3]  = { "Address error",                  K_SIGBUS   },
    [4]  = { "Illegal instruction",            K_SIGILL   },
    [5]  = { "Divide by zero",                 K_SIGFPE   },
    [6]  = { "CHK, CHK2 instruction",          K_SIG_NONE },
    [7]  = { "FTRAPcc, TRAPcc, TRAPV",         K_SIG_NONE },
    [8]  = { "Privilege violation",            K_SIGILL   },
    [9]  = { "Trace",                          K_SIGTRAP  },
    [10] = { "Line 1010 emulator",             K_SIGILL   },
    [11] = { "Line 1111 emulator",             K_SIGILL   },
    [13] = { "Coprocessor protocol violation", K_SIGILL   },
    [14] = { "Format error",                   K_SIGILL   },
    [15] = { "Unitialized interrupt",          K_SIG_NONE },
    [24] = { "Spurious interrupt",             K_SIG_NONE },
    [48] = { "FP branch/set unordered",        K_SIGFPE   },
    [49] = { "FP inexact",                     K_SIGFPE   },
    [50] = { "FP divide by zero",              K_SIGFPE   },
    [51] = { "FP underflow",                   K_SIGFPE   },
    [52] = { "FP operand error",               K_SIGFPE   },
    [53] = { "FP overflow",                    K_SIGFPE   },
    [54] = { "FP SNAN",                        K_SIGFPE   },
    [55] = { "FP unimplemented datatype",      K_SIGILL   },
    [56] = { "MMU configuration error",        K_SIG_NONE },
    [57] = { "MMU illegal operation",          K_SIG_NONE },
    [58] = { "MMU access level violation",     K_SIG_NONE },
};

static inline bool exc_from_user(const exc_frame_header_t *f)
{
    return (f->sr & (1u << 13)) == 0; // S bit
}

enum { SYSCALL_TRAPNO = 0 };
enum { SYSCALL_VECTOR = 32 + SYSCALL_TRAPNO };

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

    // (Check for the compiler)
    if (num > 255) return;

    printk("Vector $%.2X: ", num);

    if (num >= 25 && num < 32)
    {
        printk("Level %d Interrupt Autovector\n", num - 24);
    }
    else if (num >= 32 && num < 48)
    {
        printk("TRAP #%d Instruction\n", num - 32);
    }
    else if (num >= 64 && num < 256)
    {
        printk("User Defined Vector $%02X (%d)\n", num, num);
    }
    else if (exc_info[num].msg) {
        printk("%s\n", exc_info[num].msg);
    }
    else {
        printk("(Unassigned, Reserved)\n");
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

static void deliver_exception_to_user(uint8_t vec, saved_regs_t *r, exc_frame_header_t *f)
{
    const exc_info_t *info = &exc_info[vec];

    /* TODO: I don't have signals yet

    This is where we'd:
    - print a user-mode crash message
    - mark current process as dead
    - schedule next process
    - never return to this context
    
    This is where Minix does cause_sig(proc_nr(...), ep->signum)
    */
    (void)r;

    printk("User exception vec=%u", vec);
    if (info->msg) printk(" (%s)", info->msg);
    printk(" at PC=0x%08lx\n", f->pc);

    if (f->fmtvec == FMTVEC(7, 2)) {
        print_access_fault(exc_as_fmt7(f));
    }

    // TODO:
    // signal_or_kill_current(info->sig ? info->sig : K_SIGSEGV, r, f);

    while (1) __asm__ __volatile__ ("stop #0x2700": : : "cc");
}

static void panic_kernel_exception(uint8_t vec, saved_regs_t *r, exc_frame_header_t *f)
{
    const exc_info_t *info = &exc_info[vec];

    kputchar('\n');
    for (int i = 0; i < 80; i++) kputchar('-');
    printk("\n\nKERNEL EXCEPTION vec=%u", vec);
    if (info->msg) printk(" (%s)", info->msg);

    printk("\n*** STOP: PC=0x%08lx\n", f->pc);
    print_regs(r);
    print_sr(f->sr);
    printk("F $%.1x / ", (uint32_t)exc_format(f));
    crack_vector_names(vec);

    if (f->fmtvec == FMTVEC(7, 2)) {
        print_access_fault(exc_as_fmt7(f));
    }

    while (1) __asm__ __volatile__ ("stop #0x2700" : : : "cc");
}

#define SET_SFC(x) \
    __asm__ __volatile__ (" movec %0,%/sfc" : : "d" (x))

#define SET_DFC(x) \
    __asm__ __volatile__ (" movec %0,%/dfc" : : "d" (x))

#define GET_BYTE(addr,value) \
    __asm__ __volatile__ (" moves.b %1@, %0": "=d"(value) : "a"(addr))

void syscall_dispatch(saved_regs_t *r, exc_frame_header_t *f)
{
    (void)f;
    const uint32_t call_no = r->d0;
    LOG("Got syscall: %d\n", call_no);

    if (call_no == 1) {
        // print(char*, size)
        LOG("print(str=0x%08lx, size=%u)\n", r->d1, r->d2);
        uintptr_t str = (uintptr_t)r->d1;

        // Try retrieving the string using `moves`
#define FC_U_DATA 0b001
#define FC_S_DATA 0b101

        SET_SFC(FC_U_DATA);
        SET_DFC(FC_S_DATA);

        unsigned char c;
        GET_BYTE(str, c);
        while (c != 0) {
            GET_BYTE(str, c);
            kputchar((int)c);
            str += 1;
        }

    }
    if (call_no == 9) {
        LOG("exit()\n");
    }
    return;
}

void ExceptionHandler_c(saved_regs_t *r)
{
    exc_frame_header_t *f = (exc_frame_header_t*)((uint8_t *)r + sizeof(*r));
    uint8_t vec = (uint8_t)exc_vector(f);

    // Special case: syscall vector
    if (vec == SYSCALL_VECTOR) {
        if (exc_from_user(f)) {
            syscall_dispatch(r, f);
            return; // assembly stuff will restore regs and RTE
        }
        // syscall trap from supervisor is a kernel bug
        panic_kernel_exception(vec, r, f);
    }

    // Spurious interrupt logging
    if (vec == 24) {
        printk("Spurious interrupt\n");
        return;
    }

    if (exc_from_user(f)) {
        deliver_exception_to_user(vec, r, f);
        // not reached if we kill the process properly
        return;
    }

    panic_kernel_exception(vec, r, f);
}
