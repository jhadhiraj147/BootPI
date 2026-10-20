# Running on a Raspberry Pi 4

Use a Raspberry Pi 4 Model B with its normal EEPROM bootloader configured to
try the microSD card. BootPI has not been tested on a physical board. Building
and host checks cannot establish that the board boots or drives the LED.

## Prepare the boot partition

Use a spare microSD card with an existing FAT32 boot partition. Mount that
partition at `/mnt/bootpi`; confirm its device and filesystem with `lsblk -f`
and `findmnt --mountpoint /mnt/bootpi`. The commands below copy files into that
partition and replace files with the same names. They do not format a card.
Save any existing files you intend to retain before using it for BootPI.

The required external files are `start4.elf`, its matching `fixup4.dat`, and
`bcm2711-rpi-4-b.dtb`. They are not included in this repository. This example
pins all three to Raspberry Pi firmware commit
[`016253866bbd6c77fcb2a744a97d81e79ad4884f`](https://github.com/raspberrypi/firmware/tree/016253866bbd6c77fcb2a744a97d81e79ad4884f/boot),
rather than downloading a changing `master` branch. It also keeps the upstream
`LICENCE.broadcom` notice with the firmware.

Run from the BootPI repository, with `curl` available:

```sh
make
make check
(
    set -eu
    bootpi_sd=/mnt/bootpi
    mountpoint -q "$bootpi_sd"
    test "$(findmnt -n -o FSTYPE --mountpoint "$bootpi_sd")" = vfat
    bootpi_revision=016253866bbd6c77fcb2a744a97d81e79ad4884f
    bootpi_download=$(mktemp -d)
    for bootpi_file in start4.elf fixup4.dat bcm2711-rpi-4-b.dtb LICENCE.broadcom; do
        curl --fail --location --retry 3 \
            "https://raw.githubusercontent.com/raspberrypi/firmware/$bootpi_revision/boot/$bootpi_file" \
            --output "$bootpi_download/$bootpi_file"
    done
    sudo install -m 644 "$bootpi_download/"* config.txt kernel8.img "$bootpi_sd/"
    sync
    sudo umount "$bootpi_sd"
    rm -r "$bootpi_download"
)
```

The partition root should now contain:

```text
config.txt
kernel8.img
start4.elf
fixup4.dat
bcm2711-rpi-4-b.dtb
LICENCE.broadcom
```

Use the repository's complete `config.txt`, which explicitly selects AArch64,
the image, its `0x80000` load address, and the low peripheral mapping. The
standard ARM stub is built into the VideoCore firmware: no separate
`armstub*.bin` is needed. Pi 4 does not use an SD-card `bootcode.bin`. This
program needs no Linux root filesystem, `cmdline.txt`, initramfs, or overlays;
it does not consume the DTB, but retains the normal firmware DTB-loading path.
No display output or serial console is implemented. See the official
[boot-file reference](https://www.raspberrypi.com/documentation/computers/configuration.html#boot-folder-contents)
and [firmware licence](https://github.com/raspberrypi/firmware/blob/016253866bbd6c77fcb2a744a97d81e79ad4884f/boot/LICENCE.broadcom).

## Connect the LED

Remove power before wiring. Use a red LED, a 330 ohm resistor, and jumper wires:

```text
GPIO17 (physical pin 11) -> 330 ohm resistor -> LED anode (+)
LED cathode (-) -> GND (physical pin 6)
```

The cathode is commonly the shorter lead and the side marked by a flat on the
LED package; check the component's markings. Physical header numbers are not
GPIO identifiers. Both the LED and Pi share the header ground. GPIO uses 3.3 V
logic: never apply 5 V to a GPIO and never connect the LED without its
current-limiting resistor. Raspberry Pi documents
[GPIO17 at pin 11 and GPIO electrical limits](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#40-pin-gpio-header).

The ARM executes stores to peripheral addresses. The SoC interconnect routes
those accesses to the GPIO controller, whose output latch controls the pin.
Writing a register changes hardware state; it is not writing ordinary RAM.
The [memory map](memory-map.md) describes the addresses and write-one semantics.

## Observe on hardware

Insert the unmounted card and power the Pi from a suitable USB-C supply.
Expected behavior is approximately 500 ms on, 500 ms off, continuously. Delay
rounding and instruction overhead can lengthen the interval slightly. A zero
counter frequency leaves GPIO17 low; an unsupported exception level parks
before GPIO setup. A dark LED alone does not distinguish either condition from
a boot-media or wiring problem.

Confirm repeated visible transitions, then measure GPIO17 against ground with
a scope or logic analyzer if available. Record the board revision, firmware
commit, build commit, and observed interval for a reproducible hardware result.
Power off and on again to check a cold boot. These are instructions for the
first physical test, not test results already obtained.
