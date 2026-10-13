#include "timer.h"

/* The architectural counter implements at least 56 bits. */
#define COUNTER_MASK ((UINT64_C(1) << 56) - 1)

uint32_t timer_frequency_hz(void)
{
    uint64_t frequency;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    return (uint32_t)frequency;
}

uint64_t timer_counter(void)
{
    uint64_t count;
    /* ISB prevents an early, speculative timestamp before preceding work. */
    __asm__ volatile("isb\n\tmrs %0, cntpct_el0" : "=r"(count) :: "memory");
    return count;
}

uint64_t timer_elapsed_ticks(uint64_t start)
{
    return (timer_counter() - start) & COUNTER_MASK;
}
