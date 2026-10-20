# Addresses, sections and registers

BootPI executes at physical addresses with the ARM MMU and caches disabled by
the standard firmware stub. `kernel_address=0x80000` in `config.txt` matches
the linker origin. This keeps the program above the low-memory firmware stub
and its handoff data. The address is explicit because firmware defaults can
change; see [the boot contract](boot-process.md#address-and-entry-contract).

## RAM layout

| Region | Contents | Initialization |
| --- | --- | --- |
| `.text`, beginning at `0x80000` | `_start` first, then C instructions | Loaded from image |
| `.rodata` | Read-only constants, when present | Loaded from image |
| `.data` | Initialized writable globals, when present | Loaded at their final RAM addresses |
| `.bss` | Zero-initialized globals, including counter frequency | Cleared by assembly |
| `.stack` | 16 KiB, growing down from `__stack_top` | Reserved, not cleared |

The linker aligns sections to 16 bytes and pads BSS to that alignment. Startup
clears BSS using eight-byte stores, including the padding. `__bss_start` and
`__bss_end` delimit that operation; the end is exclusive. `__stack_bottom` is
at or above BSS end. `__stack_top` is aligned to the AAPCS64 stack requirement.
Linker assertions reject overlaps and inconsistent alignment.

The ELF has separate RX and RW load segments. Those flags describe the layout;
they do not enforce access permissions with the MMU off. The raw image omits
ELF headers, symbols, BSS and stack reservation. `.data` has equal load and
execution addresses, so no ROM-to-RAM copy is needed. `__image_end` marks the
end of the data layout; if `.data` is empty, its alignment can put this symbol
slightly beyond the last raw image byte. Use `make inspect` and
`build/kernel8.map` for the current boundaries.

## GPIO in the physical address space

CPU registers such as `x0` hold values inside a core. RAM stores program bytes
and ordinary data. Peripheral registers belong to hardware controllers, but
occupy addresses in the CPU's physical address space. An ordinary 32-bit load
or store targeting such an address becomes a peripheral transaction:

```text
ARM STR instruction -> SoC interconnect -> GPIO controller -> output pin
```

The BCM2711 manual uses the legacy bus address `0x7e200000` for GPIO. In the
firmware's low peripheral mode, the ARM accesses it at `0xfe200000`. This is
a physical mapping supplied by the platform, not an application page table.
See the [BCM2711 peripheral manual](https://datasheets.raspberrypi.org/bcm2711/bcm2711-peripherals.pdf),
sections 1.2.3–1.2.4 and 5.2.

| Register for GPIO17 | ARM physical address | Operation |
| --- | --- | --- |
| `GPFSEL1` | `0xfe200004` | Read-modify-write bits 23:21 to `001` for output |
| `GPSET0` | `0xfe20001c` | Write bit 17 as one to set the output latch |
| `GPCLR0` | `0xfe200028` | Write bit 17 as one to clear the output latch |

Function-select writes preserve every other pin's field. SET/CLR are write-only
write-one registers: zero bits do nothing, and reading them for an RMW would
be wrong. Clearing the latch before selecting output establishes an initial
low level. The driver handles only bank 0 (GPIO0–31), ignores invalid pins,
and has a single writer on core 0. See manual section 5.2, pages 66–70.

`mmio.h` uses `volatile uint32_t` accesses to keep the compiler from removing
or caching register operations. Volatile is not a CPU ordering barrier or a
multicore synchronization mechanism. With the MMU off, ARM data accesses use
Device-nGnRnE attributes; the GPIO-only accesses are ordered. The BCM2711
manual section 1.3 discusses ordering when crossing between peripherals.
See also Arm's [memory attributes guide, section 3.2](https://documentation-service.arm.com/static/63a43e333f28e5456434e18b).

## Counter-based delays

`CNTFRQ_EL0` and `CNTPCT_EL0` are architectural system registers, read with
`MRS`, rather than MMIO addresses. The standard stub supplies the frequency
metadata; hardware does not infer the value of `CNTFRQ_EL0`. EL2 can read the
physical counter directly. EL1/EL0 would require considering higher-level
access controls, which this program avoids by staying at EL2.

The timer caches the frequency after checking it is nonzero, and converts
milliseconds to ticks with a 64-bit product and upward rounding. The product
plus 999 fits for all two 32-bit inputs; even the maximum result is below
`2^55` ticks. Elapsed subtraction is masked to 56 bits, the architectural
minimum counter width, so crossing that boundary works even on wider
counters. Measurements must span less than one `2^56`-tick period.

Each timestamp has a preceding `ISB` to prevent speculative counter reads.
A nonzero delay also executes `DSB SY` before its baseline to complete the
preceding GPIO write before measuring the pin's hold interval. Compiler memory
clobbers keep accesses on the intended side of these barriers. No barrier is
blindly added to every MMIO read or write. See Arm's
[Generic Timer guide](https://documentation-service.arm.com/static/6294c39db334256d9ea8ae53?token=).

Polling makes the interval independent of the CPU's instruction rate, unlike
the earlier counted loop. Counter resolution, polling overhead and firmware's
frequency metadata still limit precision. It is a busy wait, not a timer
interrupt or a sleep that releases the CPU to other work.
