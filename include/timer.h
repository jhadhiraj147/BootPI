#ifndef BOOTPI_TIMER_H
#define BOOTPI_TIMER_H

#include <stdint.h>

uint32_t timer_frequency_hz(void);
uint64_t timer_counter(void);
uint64_t timer_elapsed_ticks(uint64_t start);

#endif
