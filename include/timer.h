#ifndef BOOTPI_TIMER_H
#define BOOTPI_TIMER_H

#include <stdbool.h>
#include <stdint.h>

bool timer_init(void);
uint32_t timer_frequency_hz(void);
uint64_t timer_counter(void);
uint64_t timer_elapsed_ticks(uint64_t start);
/* Call after successful timer_init(); completes preceding device writes. */
void timer_delay_ms(uint32_t milliseconds);

#endif
