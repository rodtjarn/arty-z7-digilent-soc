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

run: gpio_test_low.elf hw_server
	xsdb run_gpio_test.tcl

# Start hw_server in the background if nothing is already listening on its
# default port. It's a long-lived daemon meant to be reused across many
# run/debug sessions, so we only launch it here rather than tying its
# lifecycle to this target.
hw_server:
	@if ! (exec 3<>/dev/tcp/localhost/3121) 2>/dev/null; then \
		echo "Starting hw_server..."; \
		nohup hw_server >/tmp/hw_server.log 2>&1 & disown; \
		for i in $$(seq 1 20); do \
			(exec 3<>/dev/tcp/localhost/3121) 2>/dev/null && exit 0; \
			sleep 0.5; \
		done; \
		echo "hw_server did not come up, see /tmp/hw_server.log" >&2; \
		exit 1; \
	fi

clean:
	rm -f *.o *.elf *.bin gpio_test_low.elf

.PHONY: all clean run hw_server
