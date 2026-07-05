CC      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
CFLAGS  := -mcpu=cortex-a9 -mfloat-abi=soft -O2 -g -nostdlib -ffreestanding
LFLAGS  := -mcpu=cortex-a9 -mfloat-abi=soft -nostdlib -Wl,-T,lscript.ld -Wl,-gc-sections -Wl,--build-id=none

all: gpio_test.elf gpio_test.bin gpio_test_low.elf

gpio_test.elf: gpio_test.o startup.o
	$(LD) $(LFLAGS) $^ -o $@

# gpio_test_low.elf: OCM at 0x00000000 (JTAG boot alias) — required for xsdb dow
gpio_test_low.elf: gpio_test.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test.o: gpio_test.c
	$(CC) $(CFLAGS) -c $< -o $@

startup.o: startup.s
	$(CC) $(CFLAGS) -c $< -o $@

gpio_test.bin: gpio_test.elf
	$(OBJCOPY) -O binary $< $@

run: gpio_test_low.elf
	xsdb run_gpio_test.tcl

clean:
	rm -f *.o *.elf *.bin gpio_test_low.elf

.PHONY: all clean run
