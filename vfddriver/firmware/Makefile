TARGET = target/avr-none/debug/firmware.elf
SRC = $(wildcard src/*.rs)

firmware.hex: $(TARGET)
	avr-objcopy -O ihex -R .eeprom $< $@
	avr-size $@

$(TARGET): $(SRC)
	cargo build
