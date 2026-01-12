# compiler selection
CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size

# avr settings
MCU = attiny414
F_CPU = 20000000L # 20 MHz
AVRFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra -Isrc \
					 -std=gnu11 -fno-fat-lto-objects -ffunction-sections -fdata-sections -flto

# output directory
BUILD_DIR = build

# output files
TARGET = $(BUILD_DIR)/$(notdir $(CURDIR))
ELF = $(TARGET).elf
HEX = $(TARGET).hex

# treat all .c files in src/ as source
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# default target compiles .hex
.PHONY: firmware
firmware: $(HEX)

# build directory
$(BUILD_DIR):
	mkdir -p $@

# compile each .c file to .o object
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(AVRFLAGS) -c $< -o $@

# link all .o objects to create ELF
$(ELF): $(OBJS) | $(BUILD_DIR)
	$(CC) $(AVRFLAGS) $^ -o $@

# convert ELF to HEX
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# print size and memory usage
.PHONY: size
size: $(ELF)
	$(SIZE) -C --mcu=$(MCU) $(ELF)

# clean built files
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

