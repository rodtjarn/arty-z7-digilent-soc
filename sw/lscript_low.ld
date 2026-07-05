/* Linker script for JTAG boot: OCM remapped to 0x00000000 in JTAG mode */
ENTRY(_start)

MEMORY {
    ocm : ORIGIN = 0x00000000, LENGTH = 0x10000
}

SECTIONS {
    .text : {
        KEEP(*(.vectors))
        *(.text)
    } > ocm

    .data : {
        *(.data)
    } > ocm

    .bss : {
        *(.bss)
    } > ocm

    _stack_end = ORIGIN(ocm) + LENGTH(ocm);
}
