# BootPI

A small AArch64 bare-metal program for the Raspberry Pi 4 Model B
(BCM2711, Cortex-A72). Startup parks secondary cores, sets up a stack,
clears BSS and enters C. The kernel toggles GPIO17 using the ARM generic
counter: 500 ms on, then 500 ms off. It reads firmware's frequency metadata
from `CNTFRQ_EL0` instead of assuming a CPU clock rate.

Build with Clang, LLD, LLVM binary utilities and Make:

```sh
make
```

The output is `kernel8.img`, linked at physical address `0x80000`.
No C runtime or libraries are linked. Compilation and ELF inspection
can be checked on the host; physical boot has not been tested.

With power disconnected, wire physical header pin 11 (GPIO17) through a
330-ohm resistor to a red LED's anode, then its cathode to pin 6 (GND).
The resistor limits LED current. GPIO is 3.3 V; never apply 5 V to it.
On hardware the intended behavior is a repeating on/off blink; this has
not yet been tested on a physical board.

Register offsets and ARM physical mapping come from the
[BCM2711 manual](https://datasheets.raspberrypi.org/bcm2711/bcm2711-peripherals.pdf),
sections 1.2 and 5.2. Header numbering follows the
[official GPIO guide](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#gpio).
