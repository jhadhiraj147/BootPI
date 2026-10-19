CC := clang
LD := ld.lld
OBJCOPY := llvm-objcopy
READELF := llvm-readelf
OBJDUMP := llvm-objdump
HOSTCC := clang
HOSTCFLAGS := -std=c11 -O2 -Wall -Wextra -Werror
PYTHON := python3

TARGET_FLAGS := --target=aarch64-none-elf -mcpu=cortex-a72 -mgeneral-regs-only
CFLAGS := $(TARGET_FLAGS) -std=c11 -O2 -Wall -Wextra -Werror \
	-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -Iinclude
ASFLAGS := $(TARGET_FLAGS)
LDFLAGS := -T linker.ld --build-id=none -z max-page-size=0x1000
OBJECTS := build/boot/start.o build/src/kernel.o build/src/gpio.o build/src/timer.o
DEPS := $(OBJECTS:.o=.d) build/tests/layout.d

.PHONY: all clean check inspect disasm
.DELETE_ON_ERROR:
all: kernel8.img

kernel8.img: build/kernel8.elf Makefile
	$(OBJCOPY) -O binary $< $@

build/kernel8.elf: $(OBJECTS) linker.ld Makefile
	$(LD) $(LDFLAGS) -Map=build/kernel8.map $(OBJECTS) -o $@

build/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: %.S Makefile
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -MMD -MP -c $< -o $@

build/tests/gpio_test: tests/gpio_test.c tests/fakes/mmio.h src/gpio.c include/gpio.h Makefile
	@mkdir -p $(@D)
	$(HOSTCC) $(HOSTCFLAGS) -Itests/fakes -Iinclude tests/gpio_test.c src/gpio.c -o $@

build/tests/timer_test: tests/timer_test.c src/timer_math.h Makefile
	@mkdir -p $(@D)
	$(HOSTCC) $(HOSTCFLAGS) -Isrc $< -o $@

build/tests/layout.elf: build/boot/start.o build/tests/layout.o linker.ld Makefile
	$(LD) $(LDFLAGS) build/boot/start.o build/tests/layout.o -o $@

build/tests/layout.img: build/tests/layout.elf Makefile
	$(OBJCOPY) -O binary $< $@

check: all build/tests/gpio_test build/tests/timer_test build/tests/layout.img
	./build/tests/gpio_test
	./build/tests/timer_test
	$(PYTHON) scripts/check_elf.py build/kernel8.elf kernel8.img config.txt
	$(PYTHON) scripts/check_elf.py build/tests/layout.elf build/tests/layout.img config.txt

inspect: build/kernel8.elf
	$(READELF) --file-header --program-headers --sections --symbols $<

disasm: build/kernel8.elf
	$(OBJDUMP) --disassemble $< > build/kernel8.dis
	@cat build/kernel8.dis

clean:
	rm -rf build kernel8.img

-include $(DEPS)
