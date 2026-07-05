ENTRY(_start)

MEMORY {
    ocm : ORIGIN = 0xFFFC0000, LENGTH = 0x40000
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
