# Arch ships the RISC-V cross toolchain as either riscv64-unknown-elf-* or
# riscv64-elf-* depending on the package; detect whichever is present.
CROSS  ?= $(if $(shell command -v riscv64-unknown-elf-gcc 2>/dev/null),riscv64-unknown-elf-,\
           $(if $(shell command -v riscv64-elf-gcc 2>/dev/null),riscv64-elf-,riscv64-unknown-elf-))
CC     := $(CROSS)gcc
OBJDUMP:= $(CROSS)objdump

ARCH   ?= rv32i
ABI    ?= ilp32

WARN := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
        -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Werror

BUILD  := build
TARGET := $(BUILD)/firmware
OBJS   := $(BUILD)/start.o $(BUILD)/main.o

CFLAGS  := -march=$(ARCH) -mabi=$(ABI) -mcmodel=medany -nostdlib -ffreestanding \
           -std=c17 $(WARN) -fno-builtin -Os -g
LDFLAGS := -march=$(ARCH) -mabi=$(ABI) -nostdlib -T linker.ld \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/firmware.map

QEMU ?= qemu-system-riscv32

.PHONY: all clean run debug gdb disasm

all: $(TARGET).elf disasm

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/start.o: start.S | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/main.o: src/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

disasm: $(TARGET).elf
	$(OBJDUMP) -d $< | head -40

run: $(TARGET).elf
	timeout 5 $(QEMU) -machine virt -bios none -kernel $< -nographic -monitor none || true

# Stop at reset and expose a GDB stub on localhost:1234.
debug: $(TARGET).elf
	$(QEMU) -machine virt -bios none -kernel $< -nographic -monitor none \
		-S -gdb tcp::1234

gdb: $(TARGET).elf
	gdb -q $< \
		-ex 'set architecture riscv:rv32' \
		-ex 'target remote localhost:1234' \
		-ex 'break main' -ex 'continue'

clean:
	rm -rf $(BUILD)
