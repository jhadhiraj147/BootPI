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

    ASSERT(_start == __image_start, "startup must be the first image bytes")
    ASSERT((__bss_start % 8) == 0 && (__bss_end % 8) == 0,
           "BSS must support startup's eight-byte stores")
    ASSERT(__image_end <= __bss_start, "image overlaps BSS")
    ASSERT(__bss_end <= __stack_bottom, "BSS overlaps the stack")
    ASSERT((__stack_top % 16) == 0, "C stack must be 16-byte aligned")
    ASSERT(__stack_top - __stack_bottom == 16K, "stack reservation changed")
}
