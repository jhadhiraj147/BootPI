#include "timer.h"
#include "timer_math.h"

static uint32_t counter_frequency;

bool timer_init(void)
{
    counter_frequency = timer_frequency_hz();
    return counter_frequency != 0;
}

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
    return elapsed_ticks(start, timer_counter());
}

void timer_delay_ms(uint32_t milliseconds)
{
    uint64_t ticks = ticks_for_ms(counter_frequency, milliseconds);
    if (ticks == 0) {
        return;
    }

    /* Complete the GPIO write before starting its measured hold interval. */
    __asm__ volatile("dsb sy" ::: "memory");
    uint64_t start = timer_counter();
    while (timer_elapsed_ticks(start) < ticks) {
        __asm__ volatile("yield");
    }
}
