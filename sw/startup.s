.globl _start
_start:
    ldr sp, =_stack_end
    bl main
    b .

.section .vectors, "ax"
    b _start
    .space 28
