#include "gpio.h"
#include "mmio.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

static uint32_t read_value;
static uintptr_t read_address;
static uintptr_t write_address;
static uint32_t write_value;
static unsigned int reads;
static unsigned int writes;

uint32_t mmio_read32(uintptr_t address)
{
    assert(reads == 0 && writes == 0);
    read_address = address;
    ++reads;
    return read_value;
}

void mmio_write32(uintptr_t address, uint32_t value)
{
    assert(writes == 0);
    write_address = address;
    write_value = value;
    ++writes;
}

static void reset(uint32_t initial)
{
    read_value = initial;
    read_address = 0;
    write_address = 0;
    write_value = 0;
    reads = 0;
    writes = 0;
}

static void check_gpio17(void)
{
    reset(UINT32_C(0xa5a5a5a5));
    gpio_set_output(17);
    assert(reads == 1 && writes == 1);
    assert(read_address == UINT32_C(0xfe200004));
    assert(write_address == UINT32_C(0xfe200004));
    assert((write_value & UINT32_C(0x00e00000)) == UINT32_C(0x00200000));
    assert((write_value & UINT32_C(0xff1fffff)) ==
           (read_value & UINT32_C(0xff1fffff)));
}

static void check_function_fields(void)
{
    static const uintptr_t addresses[] = {
        UINT32_C(0xfe200000), UINT32_C(0xfe200004),
        UINT32_C(0xfe200008), UINT32_C(0xfe20000c),
    };
    static const uint32_t patterns[] = {
        0, UINT32_MAX, UINT32_C(0xaaaaaaaa), UINT32_C(0x55555555),
    };

    unsigned int pin = 0;
    for (unsigned int bank = 0; bank < 4; ++bank) {
        for (unsigned int bit = 0; bit < 30 && pin < 32; bit += 3, ++pin) {
            uint32_t mask = UINT32_C(7) << bit;
            for (unsigned int i = 0; i < sizeof(patterns) / sizeof(patterns[0]); ++i) {
                reset(patterns[i]);
                gpio_set_output(pin);
                assert(reads == 1 && writes == 1);
                assert(read_address == addresses[bank]);
                assert(write_address == addresses[bank]);
                assert(((write_value >> bit) & 7) == 1);
                assert((write_value & ~mask) == (patterns[i] & ~mask));
            }
        }
    }
    assert(pin == 32);
}

static void check_set_clear(void)
{
    for (unsigned int pin = 0; pin < 32; ++pin) {
        for (unsigned int high = 0; high < 2; ++high) {
            reset(UINT32_MAX);
            gpio_write(pin, high != 0);
            assert(reads == 0 && writes == 1);
            assert(write_address == (high ? UINT32_C(0xfe20001c) :
                                           UINT32_C(0xfe200028)));
            assert(write_value == (uint32_t)(UINT64_C(1) << pin));
        }
    }
}

static void check_invalid_pins(void)
{
    static const unsigned int invalid[] = {32, 53, UINT_MAX};
    for (unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        reset(UINT32_MAX);
        gpio_set_output(invalid[i]);
        gpio_write(invalid[i], true);
        gpio_write(invalid[i], false);
        assert(reads == 0 && writes == 0);
    }
}

int main(void)
{
    check_gpio17();
    check_function_fields();
    check_set_clear();
    check_invalid_pins();
    puts("GPIO host checks passed");
    return 0;
}
