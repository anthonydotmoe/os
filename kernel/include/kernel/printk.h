#pragma once

#include <stdarg.h>
#include <stddef.h>

int printk(const char *fmt, ...);

// Provided by libprintf
#define snprintk  snprintf_
int  snprintf_(char* buffer, size_t count, const char* format, ...);

// Logging macros.
#define LOG_ANSI_RESET_ATTRIBS_     "\033[0m"
#define LOG_ANSI_SELECT_COLOR_BLU_  "\033[94m"
#define LOG_ANSI_SELECT_COLOR_RED_  "\033[31m"
#define LOG_ANSI_SELECT_COLOR_YEL_  "\033[33m"

#ifdef DEBUG
#define LOG_PREPEND_FMT_STR_    "%s:%d:%s: "
#define LOG_PREPEND_FMT_ARG_    __FILE__, __LINE__, __func__
#else
#define LOG_PREPEND_FMT_STR_    "%s: "
#define LOG_PREPEND_FMT_ARG_    __func__
#endif

// No color
#define LOG(fmt, ...) \
    printk(LOG_PREPEND_FMT_STR_ fmt, LOG_PREPEND_FMT_ARG_ __VA_OPT__(,) __VA_ARGS__)

// Light Blue
#define LOG_I(fmt, ...) \
    printk(LOG_ANSI_SELECT_COLOR_BLU_ LOG_PREPEND_FMT_STR_ fmt LOG_ANSI_RESET_ATTRIBS_, LOG_PREPEND_FMT_ARG_ __VA_OPT__(,) __VA_ARGS__)

// Yellow
#define LOG_W(fmt, ...) \
    printk(LOG_ANSI_SELECT_COLOR_YEL_ LOG_PREPEND_FMT_STR_ fmt LOG_ANSI_RESET_ATTRIBS_, LOG_PREPEND_FMT_ARG_ __VA_OPT__(,) __VA_ARGS__)

// Red
#define LOG_E(fmt, ...) \
    printk(LOG_ANSI_SELECT_COLOR_RED_ LOG_PREPEND_FMT_STR_ fmt LOG_ANSI_RESET_ATTRIBS_, LOG_PREPEND_FMT_ARG_ __VA_OPT__(,) __VA_ARGS__)

