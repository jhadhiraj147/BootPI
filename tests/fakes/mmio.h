#ifndef BOOTPI_TEST_MMIO_H
#define BOOTPI_TEST_MMIO_H

#include <stdint.h>

uint32_t mmio_read32(uintptr_t address);
void mmio_write32(uintptr_t address, uint32_t value);

#endif
