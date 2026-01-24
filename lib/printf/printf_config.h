#pragma once

#define PRINTF_DISABLE_SUPPORT_PTRDIFF_T
#define PRINTF_DISABLE_SUPPORT_FLOAT
#define PRINTF_DISABLE_SUPPORT_EXPONENTIAL
#define PRINTF_DISABLE_SUPPORT_LONG_LONG

// Hack: provide a dummy _putchar definition to satisfy the library
static void _putchar(char character) { (void)character; }
