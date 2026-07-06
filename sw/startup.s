.section .vectors, "ax"
    b _start
    .space 28

.section .text.startup, "ax"
.globl _start
_start:
    ldr sp, =_stack_end
    bl main
    b .
