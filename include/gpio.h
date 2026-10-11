#ifndef BOOTPI_GPIO_H
#define BOOTPI_GPIO_H

#include <stdbool.h>

/* The initial driver supports bank 0 (GPIO0..31), including GPIO17. */
void gpio_set_output(unsigned int pin);
void gpio_write(unsigned int pin, bool high);

#endif
