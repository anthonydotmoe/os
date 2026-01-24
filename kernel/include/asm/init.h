#pragma once

// linux/arch/um/include/shared/init.h

/*
 *
 * These macros are used to mark some functions
 * or initialized data as 'init' functions. The kernel
 * can take this as a hint that the function is used only
 * during the initialization phase and free up used memory
 * resources after.
 * 
 * Usage:
 * For functions:
 * 
 * Add __init immediately before the function name, like:
 * 
 * static void __init initme(int x, int y)
 * {
 *     extern int z; z = x * y;
 * }
 * 
 * If the function has a prototype somewhere, you can also add
 * __init between the closing parenthesis of the prototype and the semicolon:
 * 
 * extern int initialize_device(int, int, int) __init;
 * 
 * For initialized data:
 * You should insert __initdata between the variable name and equal sign
 * followed by value:
 * 
 * static int init_variable __initdata = 0;
 * static const char linux_logo[] __initconst = { 0x32, 0x36, ... };
 * 
 * Don't forget to initialize data not at file scope, i.e. within a function,
 * as gcc otherwise puts the data into the bss section and not the init section.
 * 
 * Also note, that this data cannot be "const".
 */

#ifndef __section
#define __section(x) __attribute__((__section__(x)))
#endif

#define __init      __section(".init.text")
#define __initconst __section(".init.rodata")
#define __initdata  __section(".init.data")
#define __exitdata  __section(".exit.data")
#define __exit_call __used __section(".exitcall.exit")
