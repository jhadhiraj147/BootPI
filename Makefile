CC := clang
LD := ld.lld
OBJCOPY := llvm-objcopy

TARGET_FLAGS := --target=aarch64-none-elf -mcpu=cortex-a72 -mgeneral-regs-only
CFLAGS := $(TARGET_FLAGS) -std=c11 -O2 -Wall -Wextra -Werror \
	-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -Iinclude
ASFLAGS := $(TARGET_FLAGS)
LDFLAGS := -T linker.ld --build-id=none -z max-page-size=0x1000
OBJECTS := build/boot/start.o build/src/kernel.o build/src/gpio.o build/src/timer.o
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean
all: kernel8.img

kernel8.img: build/kernel8.elf
	$(OBJCOPY) -O binary $< $@

build/kernel8.elf: $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -Map=build/kernel8.map $(OBJECTS) -o $@

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build kernel8.img

-include $(DEPS)
