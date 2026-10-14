#include "gpio.h"
#include "timer.h"

void kernel_main(void)
{
    const unsigned int led = 17;
    gpio_write(led, false);
    gpio_set_output(led);

    if (!timer_init()) {
        /* Missing firmware frequency metadata: leave the LED low. */
        for (;;) {
            __asm__ volatile("wfe");
        }
    }

    for (;;) {
        gpio_write(led, true);
        timer_delay_ms(500);
        gpio_write(led, false);
        timer_delay_ms(500);
    }
}
