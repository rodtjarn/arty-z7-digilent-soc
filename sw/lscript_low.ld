/* Linker script for JTAG boot: OCM remapped to 0x00000000 in JTAG mode */
ENTRY(_start)

MEMORY {
    ocm : ORIGIN = 0x00000000, LENGTH = 0xF000
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
