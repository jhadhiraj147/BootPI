#include "timer_math.h"

#include <assert.h>
#include <stdio.h>

static void check_tick_conversion(void)
{
    static const uint32_t frequencies[] = {
        0, 1, 999, 1000, 1001, 32768, 19200000, 54000000, UINT32_MAX,
    };
    static const uint32_t delays[] = {
        0, 1, 2, 499, 500, 999, 1000, 1001, 60000, UINT32_MAX,
    };

    assert(ticks_for_ms(54000000, 0) == 0);
    assert(ticks_for_ms(54000000, 500) == 27000000);
    assert(ticks_for_ms(1, 1) == 1);
    assert(ticks_for_ms(3, 500) == 2);
    assert(ticks_for_ms(32768, 1) == 33);

    for (unsigned int f = 0; f < sizeof(frequencies) / sizeof(frequencies[0]); ++f) {
        uint64_t previous = 0;
        for (unsigned int d = 0; d < sizeof(delays) / sizeof(delays[0]); ++d) {
            __uint128_t product = (__uint128_t)frequencies[f] * delays[d];
            __uint128_t expected = product / 1000;
            if (product % 1000 != 0) {
                ++expected;
            }

            uint64_t actual = ticks_for_ms(frequencies[f], delays[d]);
            assert(actual == expected);
            assert(actual >= previous);
            assert((__uint128_t)actual * 1000 >= product);
            assert((__uint128_t)actual * 1000 < product + 1000);
            assert(actual < (UINT64_C(1) << 55));
            previous = actual;
        }
    }
}

static void check_counter_wrap(void)
{
    static const uint64_t starts[] = {
        0, UINT64_C(0x00fffffffffffffd), UINT64_C(0x00ffffffffffffff),
        UINT64_C(0x9bffffffffffffff), UINT64_MAX,
    };
    static const uint64_t intervals[] = {
        0, 1, 27, 27000000, (UINT64_C(1) << 55) - 1,
    };

    assert(COUNTER_MASK == UINT64_C(0x00ffffffffffffff));
    assert(elapsed_ticks(17, 17) == 0);
    assert(elapsed_ticks(17, 32) == 15);
    assert(elapsed_ticks(UINT64_C(0x00fffffffffffffd), 7) == 10);
    assert(elapsed_ticks(UINT64_MAX - 5, 7) == 13);
    assert(elapsed_ticks(UINT64_C(0xff00000000000010),
                         UINT64_C(0x0100000000000035)) == 37);

    for (unsigned int s = 0; s < sizeof(starts) / sizeof(starts[0]); ++s) {
        for (unsigned int i = 0; i < sizeof(intervals) / sizeof(intervals[0]); ++i) {
            uint64_t now = starts[s] + intervals[i];
            assert(elapsed_ticks(starts[s], now) == intervals[i]);
            assert(elapsed_ticks(starts[s] & COUNTER_MASK,
                                 now & COUNTER_MASK) == intervals[i]);
        }
    }
}

int main(void)
{
    check_tick_conversion();
    check_counter_wrap();
    puts("Timer arithmetic host checks passed (no hardware counter access)");
    return 0;
}
