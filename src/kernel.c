#include "gpio.h"

#include <stdint.h>

static void delay(void)
{
    /* A visible first experiment, not a calibrated time interval. */
    for (uint32_t i = 0; i < 10000000; ++i) {
        __asm__ volatile("nop");
    }
}

void kernel_main(void)
{
    const unsigned int led = 17;
    gpio_write(led, false);
    gpio_set_output(led);

    for (;;) {
        gpio_write(led, true);
        delay();
        gpio_write(led, false);
        delay();
    }
}
