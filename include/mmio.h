#ifndef BOOTPI_MMIO_H
#define BOOTPI_MMIO_H

#include <stdint.h>

static inline uint32_t mmio_read32(uintptr_t address)
{
    /* Each access must reach the register, rather than a compiler-cached value. */
    return *(volatile uint32_t *)address;
}

static inline void mmio_write32(uintptr_t address, uint32_t value)
{
    *(volatile uint32_t *)address = value;
}

#endif
