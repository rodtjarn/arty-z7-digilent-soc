.section .vectors, "ax"
    b _start
    b .
    b .
    b .
    b .
    b .
    b irq_entry
    b .

.section .text.startup, "ax"
.globl _start
_start:
    mov r0, #0
    mcr p15, 0, r0, c12, c0, 0
    mrc p15, 0, r1, c1, c0, 0
    bic r1, r1, #0x2000
    mcr p15, 0, r1, c1, c0, 0

    mrs r0, cpsr
    bic r1, r0, #0x1f
    orr r1, r1, #0xd2
    msr cpsr_c, r1
    ldr sp, =_irq_stack_end
    msr cpsr_c, r0

    ldr sp, =_stack_end
    bl main
    b .

.globl irq_entry
irq_entry:
    sub lr, lr, #4
    push {r0-r3, r12, lr}
    bl irq_handler
    pop {r0-r3, r12, lr}
    subs pc, lr, #0
