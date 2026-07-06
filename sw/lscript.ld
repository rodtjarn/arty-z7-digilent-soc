ENTRY(_start)

MEMORY {
    ocm : ORIGIN = 0xFFFC0000, LENGTH = 0x40000
}

SECTIONS {
    .text : {
        KEEP(*(.vectors))
        *(.text.startup)
        *(.text*)
        *(.rodata*)
    } > ocm

    .data : {
        *(.data*)
    } > ocm

    .bss (NOLOAD) : {
        *(.bss*)
        *(COMMON)
    } > ocm

    _stack_end = ORIGIN(ocm) + LENGTH(ocm);
}
