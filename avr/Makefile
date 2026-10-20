MCU    ?= atmega328p
F_CPU  ?= 16000000UL
BAUD   ?= 9600

AVR_CC  ?= avr-gcc
OBJCOPY ?= avr-objcopy
SIZE    ?= avr-size
HOST_CC ?= cc

WARN := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
        -Wstrict-prototypes -Wmissing-prototypes -Wvla -Wundef -Werror

SIMAVR_CFLAGS ?= -I/usr/include/simavr
SIMAVR_LIBS   ?= -lsimavr

BUILD  := build
SRCS   := $(wildcard src/*.c)
OBJS   := $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))
TARGET := $(BUILD)/firmware

CFLAGS   := -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -g -std=c17 $(WARN) \
            -ffunction-sections -fdata-sections -fno-common
HOST_CFLAGS ?= -O2 -g -std=c17 $(WARN)
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections

.PHONY: all clean sim vcd wave debug size

all: $(TARGET).elf $(TARGET).hex size

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(AVR_CC) $(CFLAGS) -c -o $@ $<

$(TARGET).elf: $(OBJS)
	$(AVR_CC) $(LDFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# ---- host-side simulator harness -------------------------------------------
$(BUILD)/vcd_harness: sim/vcd_harness.c | $(BUILD)
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $< $(SIMAVR_CFLAGS) $(SIMAVR_LIBS)

# ---- run / verify ----------------------------------------------------------
sim: $(TARGET).hex
	timeout 5 simavr -m $(MCU) -f $(patsubst %UL,%,$(F_CPU)) $< || true

vcd: $(TARGET).hex $(BUILD)/vcd_harness
	$(BUILD)/vcd_harness $< $(BUILD)/firmware.vcd $(MCU) \
		$(patsubst %UL,%,$(F_CPU)) 1200

wave: vcd
	gtkwave $(BUILD)/firmware.vcd &

debug: $(TARGET).elf
	../scripts/avr-debug.sh $< $(MCU) $(patsubst %UL,%,$(F_CPU))

size: $(TARGET).elf
	$(SIZE) -C --mcu=$(MCU) $<

clean:
	rm -rf $(BUILD)
