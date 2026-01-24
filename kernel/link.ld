PHDRS
{
    text PT_LOAD FILEHDR PHDRS FLAGS (7);
    data PT_LOAD FLAGS (7);
}

OUTPUT_FORMAT("elf32-m68k", "elf32-m68k", "elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

SECTIONS
{
    . = 0x1000;
    _text = .;
    .text : {
        KEEP(*(.head.text))
        . = ALIGN(8);
        *(.text .text.*)
        . = ALIGN(4096);
        /* . = ALIGN(8); */
    } :text = 0x4e75

    _etext = .;

    _sdata = .;

    .rodata :
    {
        __start_rodata = .;
        *(.rodata) *(.rodata.*) *(.data.rel.ro*)
        . = ALIGN(8);
        __rodata_end = .;
    } :text

    .data :
    {
        __data_start = .;
        *(.data .data.*)
        . = ALIGN(8);
        __data_end = .;
    }
    
    __bss_start = .;

    .bss : AT(ADDR(.bss))
    {
        . = ALIGN(4096);
        *(.bss .bss.*)
        *(COMMON)
    }

    __bss_stop = .;

    _edata = .;

    /* Will be freed after init */
    . = ALIGN(4096);
    __init_begin = .;

    /* INIT_TEXT_SECTION(PAGE_SIZE) : data */
    . = ALIGN(4096);
    .init.text : AT(ADDR(.init.text))
    {
        _sinittext = .;
        *(.init.text .init.text.*)
        *(.text.startup .text.startup.*)
        _einittext = .;
    } : data

    /* INIT_DATA_SECTION(16) */
    .init.data : AT(ADDR(.init.data))
    {
        *(.init.data .init.data.*)
        *(.init.rodata .init.rodata.*)
        . = ALIGN(16);
        __setup_start = .;
        KEEP(*(.init.setup))
        __setup_end = .;
    }
    .init_end : {
        /* This ALIGN is in a section so that _end is
           at the end of the load segment. */
        . = ALIGN(4096);
        __init_end = .;
    }

    _end = .;
}
