OUTPUT_ARCH(aarch64)
ENTRY(_start)

PHDRS
{
    code PT_LOAD FLAGS(5);
    ram PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x80000;
    __image_start = .;

    .text : ALIGN(16) {
        KEEP(*(.text.boot))
        *(.text .text.*)
    } :code

    .rodata : ALIGN(16) { *(.rodata .rodata.*) } :code

    .data : ALIGN(16) {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } :ram
    __image_end = .;

    .bss (NOLOAD) : ALIGN(16) {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    } :ram

    .stack (NOLOAD) : ALIGN(16) {
        __stack_bottom = .;
        . += 16K;
        __stack_top = .;
    } :ram

    __memory_end = .;
    /DISCARD/ : { *(.comment .note* .eh_frame* .llvm_addrsig) }
}
