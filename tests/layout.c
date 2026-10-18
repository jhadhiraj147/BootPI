#include <stdint.h>

/* Exercise sections absent or tiny in the real blink kernel. */
volatile uint64_t layout_data = UINT64_C(0x0123456789abcdef);
const unsigned char layout_rodata[] = {0x23, 0x45, 0x67};
volatile unsigned char layout_bss[19];

void kernel_main(void)
{
    for (;;) {
        layout_bss[0] = (unsigned char)layout_data + layout_rodata[0];
    }
}
