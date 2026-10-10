.section .text.boot, "ax"
.global _start
.type _start, %function
_start:
    msr daifset, #0xf
    mrs x0, mpidr_el1
    and x0, x0, #0xff
    cbnz x0, .Lpark

    // Use the current exception level's stack, with AAPCS64's 16-byte alignment.
    msr spsel, #1
    ldr x0, =__stack_top
    mov sp, x0

    // NOLOAD sections have no bytes in the firmware-loaded image.
    ldr x0, =__bss_start
    ldr x1, =__bss_end
.Lzero_bss:
    cmp x0, x1
    b.hs .Lenter_c
    str xzr, [x0], #8
    b .Lzero_bss
.Lenter_c:
    bl kernel_main
.Lpark:
    wfe
    b .Lpark
.size _start, . - _start

.section .note.GNU-stack, "", %progbits
