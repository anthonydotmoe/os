PHDRS
{
    text PT_LOAD FLAGS (7);
    data PT_LOAD FLAGS (7);
}

#include <form_os/config.h>

OUTPUT_FORMAT("elf32-m68k", "elf32-m68k", "elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(__start)

SECTIONS
{
    . = KERNEL_VIRT_BASE;
    _start_kernel_image = .;
    _text = .;
    .text : {
        . = ALIGN(8);
        *(.text .text.*)
        . = ALIGN(PAGE_SIZE);
    } :text = 0x4e75

    _etext = .;

    _sdata = .;

    .rodata : {
        __start_rodata = .;
        *(.rodata) *(.rodata.*) *(.data.rel.ro*)
        . = ALIGN(8);
        __rodata_end = .;
    } :text

    .data : {
        __data_start = .;
        *(.data .data.*)
        . = ALIGN(8);
        __data_end = .;
    } :data
    
    __bss_start = .;

    .bss : {
        . = ALIGN(PAGE_SIZE);
        *(.bss .bss.*)
        *(COMMON)
    } :data

    __bss_stop = .;

    _edata = .;

    /* Will be freed after init */
    . = ALIGN(PAGE_SIZE);
    __init_begin = .;

    . = ALIGN(PAGE_SIZE);
    .init.text : {
        _sinittext = .;
        *(.init.text .init.text.*)
        *(.text.startup .text.startup.*)
        _einittext = .;
    } : data

    .init.data : {
        *(.init.data .init.data.*)
        *(.init.rodata .init.rodata.*)
        . = ALIGN(16);
        __setup_start = .;
        KEEP(*(.init.setup))
        __setup_end = .;
    } :data
    .init_end : {
        /* This ALIGN is in a section so that _end is
           at the end of the load segment. */
        . = ALIGN(PAGE_SIZE);
        __init_end = .;
    }

    _end = .;
}
