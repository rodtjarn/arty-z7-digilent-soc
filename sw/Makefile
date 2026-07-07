CC      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
CFLAGS  := -mcpu=cortex-a9 -mfloat-abi=soft -O2 -g -nostdlib -ffreestanding
LFLAGS  := -mcpu=cortex-a9 -mfloat-abi=soft -nostdlib -Wl,-gc-sections -Wl,--build-id=none
BIT_FILE := ../arty-z7-soc/hw/arty_z7_soc.bit
PS7_INIT := ../arty-z7-soc/hw/ps7_init.tcl

all: gpio_test.elf gpio_test.bin gpio_test_low.elf \
	gpio_test_uart_low.elf gpio_test_ddr_low.elf gpio_test_gpio_low.elf \
	gpio_test_buttons_low.elf gpio_test_timer_low.elf gpio_test_gic_low.elf \
	gpio_test_axi_timer_low.elf gpio_test_custom_axi_low.elf

gpio_test.elf: gpio_test.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript.ld $^ -o $@

# gpio_test_low.elf: OCM at 0x00000000 (JTAG boot alias) — required for xsdb dow
gpio_test_low.elf: gpio_test.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_uart_low.elf: gpio_test_uart.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_ddr_low.elf: gpio_test_ddr.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_gpio_low.elf: gpio_test_gpio.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_buttons_low.elf: gpio_test_buttons.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_timer_low.elf: gpio_test_timer.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_gic_low.elf: gpio_test_gic.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_axi_timer_low.elf: gpio_test_axi_timer.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test_custom_axi_low.elf: gpio_test_custom_axi.o startup.o
	$(LD) $(LFLAGS) -Wl,-T,lscript_low.ld $^ -o $@

gpio_test.o: gpio_test.c
	$(CC) $(CFLAGS) -c $< -o $@

gpio_test_uart.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_UART -c $< -o $@

gpio_test_ddr.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_DDR -c $< -o $@

gpio_test_gpio.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_GPIO -c $< -o $@

gpio_test_buttons.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_BUTTONS -c $< -o $@

gpio_test_timer.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_TIMER -c $< -o $@

gpio_test_gic.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_GIC -c $< -o $@

gpio_test_axi_timer.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_AXI_TIMER -c $< -o $@

gpio_test_custom_axi.o: gpio_test.c
	$(CC) $(CFLAGS) -DTEST_MODE=TEST_MODE_CUSTOM_AXI -c $< -o $@

startup.o: startup.s
	$(CC) $(CFLAGS) -c $< -o $@

gpio_test.bin: gpio_test.elf
	$(OBJCOPY) -O binary $< $@

run: gpio_test_low.elf check-hw-artifacts hw_server
	xsdb run_gpio_test.tcl

run-uart: gpio_test_uart_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_uart_low.elf xsdb run_gpio_test.tcl

run-ddr: gpio_test_ddr_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_ddr_low.elf xsdb run_gpio_test.tcl

run-gpio: gpio_test_gpio_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_gpio_low.elf xsdb run_gpio_test.tcl

run-buttons: gpio_test_buttons_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_buttons_low.elf xsdb run_gpio_test.tcl

run-timer: gpio_test_timer_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_timer_low.elf xsdb run_gpio_test.tcl

run-gic: gpio_test_gic_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_gic_low.elf xsdb run_gpio_test.tcl

run-axi-timer: gpio_test_axi_timer_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_axi_timer_low.elf xsdb run_gpio_test.tcl

run-custom-axi: gpio_test_custom_axi_low.elf check-hw-artifacts hw_server
	TEST_ELF=gpio_test_custom_axi_low.elf xsdb run_gpio_test.tcl

step-01-uart: run-uart
step-02-ddr: run-ddr
step-03-buttons: run-buttons
step-04-timer: run-timer
step-05-gic: run-gic
step-06-axi-timer: run-axi-timer
step-07-custom-axi: run-custom-axi

step-08-axi-bram:
	@echo "Step 08 AXI BRAM controller test is not implemented yet." >&2
	@exit 2

step-09-cache-mmu:
	@echo "Step 09 cache/MMU smoke test is not implemented yet." >&2
	@exit 2

step-10-sd-raw:
	@echo "Step 10 bare-metal SD raw-sector read test is not implemented yet." >&2
	@exit 2

steps-working: step-01-uart step-02-ddr step-03-buttons step-04-timer step-05-gic step-06-axi-timer step-07-custom-axi

regress-baremetal: all check-hw-artifacts hw_server
	@failures=0; total=0; results=""; \
	run_test() { \
		name="$$1"; target="$$2"; total=$$((total + 1)); \
		printf '\n=== %s ===\n' "$$name"; \
		if $(MAKE) --no-print-directory $$target; then \
			results="$${results}PASS $$name\n"; \
		else \
			rc=$$?; \
			results="$${results}FAIL $$name (exit $$rc)\n"; \
			failures=$$((failures + 1)); \
		fi; \
	}; \
	run_test "step-01-uart" "run-uart"; \
	run_test "step-02-ddr" "run-ddr"; \
	run_test "run-gpio" "run-gpio"; \
	run_test "step-03-buttons" "run-buttons"; \
	run_test "step-04-timer" "run-timer"; \
	run_test "step-05-gic" "run-gic"; \
	run_test "step-06-axi-timer" "run-axi-timer"; \
	run_test "step-07-custom-axi" "run-custom-axi"; \
	run_test "full-suite" "run"; \
	printf '\n=== Bare-metal regression summary ===\n'; \
	printf '%b' "$$results"; \
	printf 'SKIP step-08-axi-bram (not implemented)\n'; \
	printf 'SKIP step-09-cache-mmu (not implemented)\n'; \
	printf 'SKIP step-10-sd-raw (not implemented)\n'; \
	printf 'Ran %d implemented tests, %d failed.\n' "$$total" "$$failures"; \
	test "$$failures" -eq 0

check-hw-artifacts:
	@test -f "$(BIT_FILE)" || { \
		echo "Missing $(BIT_FILE). Run 'make build' from the repo root or 'make -C arty-z7-soc all'." >&2; \
		exit 2; \
	}
	@test -f "$(PS7_INIT)" || { \
		echo "Missing $(PS7_INIT). Run 'make build' from the repo root or 'make -C arty-z7-soc all'." >&2; \
		exit 2; \
	}

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
	rm -f *.o *.elf *.bin

.PHONY: all clean run run-uart run-ddr run-gpio run-buttons run-timer run-gic \
	run-axi-timer run-custom-axi \
	step-01-uart step-02-ddr step-03-buttons step-04-timer step-05-gic \
	step-06-axi-timer step-07-custom-axi step-08-axi-bram \
	step-09-cache-mmu step-10-sd-raw steps-working regress-baremetal \
	check-hw-artifacts hw_server
