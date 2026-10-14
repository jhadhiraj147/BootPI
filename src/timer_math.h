#ifndef BOOTPI_TIMER_MATH_H
#define BOOTPI_TIMER_MATH_H

#include <stdint.h>

/* Use the minimum architectural width, including on wider counters. */
#define COUNTER_MASK ((UINT64_C(1) << 56) - 1)

static inline uint64_t elapsed_ticks(uint64_t start, uint64_t now)
{
    return (now - start) & COUNTER_MASK;
}

static inline uint64_t ticks_for_ms(uint32_t frequency, uint32_t milliseconds)
{
    /* The product plus 999 fits uint64_t for all uint32_t inputs. Round up. */
    return ((uint64_t)frequency * milliseconds + 999) / 1000;
}

#endif
