#include "gpio.h"
#include "mmio.h"

/* BCM2711's legacy 0x7e200000 bus address in low-peripheral ARM mapping. */
#define GPIO_BASE ((uintptr_t)0xfe200000)
#define GPSET0 (GPIO_BASE + 0x1c)
#define GPCLR0 (GPIO_BASE + 0x28)

void gpio_set_output(unsigned int pin)
{
    if (pin >= 32) {
        return;
    }

    uintptr_t address = GPIO_BASE + (pin / 10) * 4;
    unsigned int shift = (pin % 10) * 3;
    uint32_t value = mmio_read32(address);

    /* Each pin owns three bits; other pins may already have a function. */
    value = (value & ~(UINT32_C(7) << shift)) | (UINT32_C(1) << shift);
    mmio_write32(address, value);
}

void gpio_write(unsigned int pin, bool high)
{
    if (pin >= 32) {
        return;
    }

    /* SET/CLR are write-one registers, never read-modify-write targets. */
    mmio_write32(high ? GPSET0 : GPCLR0, UINT32_C(1) << pin);
}
