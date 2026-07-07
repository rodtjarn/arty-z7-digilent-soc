# Top-level build/program/run pipeline for the Arty Z7 SoC + bare-metal GPIO test.
# `make` alone builds whatever is stale (hw bitstream, sw ELF) via the sub-project
# Makefiles' own file-based dependency checks, then always programs and runs on
# hardware.

.PHONY: all build run clean help linux-boot \
	step-01-uart step-02-ddr step-03-buttons step-04-timer step-05-gic \
	step-06-axi-timer step-07-custom-axi step-08-axi-bram \
	step-09-cache-mmu step-10-sd-raw steps-working regress-baremetal

all: run

build:
	$(MAKE) -C arty-z7-soc all
	$(MAKE) -C sw all

# Builds the independent Linux-boot-from-SD-card test (FSBL + U-Boot + kernel
# + BusyBox rootfs + BOOT.BIN/boot.scr). Not part of the default `all`/`run`
# flow -- the bare-metal JTAG/OCM test above stays the default. See
# linux/README or AGENTS.md for the physical steps (JP4 jumper, SD card,
# UART) needed to actually boot it on hardware.
linux-boot:
	$(MAKE) -C linux all

step-01-uart: build
	$(MAKE) -C sw step-01-uart

step-02-ddr: build
	$(MAKE) -C sw step-02-ddr

step-03-buttons: build
	$(MAKE) -C sw step-03-buttons

step-04-timer: build
	$(MAKE) -C sw step-04-timer

step-05-gic:
	$(MAKE) -C sw step-05-gic

step-06-axi-timer:
	$(MAKE) -C sw step-06-axi-timer

step-07-custom-axi:
	$(MAKE) -C sw step-07-custom-axi

step-08-axi-bram:
	$(MAKE) -C sw step-08-axi-bram

step-09-cache-mmu:
	$(MAKE) -C sw step-09-cache-mmu

step-10-sd-raw:
	$(MAKE) -C sw step-10-sd-raw

steps-working: build
	$(MAKE) -C sw steps-working

regress-baremetal: build
	$(MAKE) -C sw regress-baremetal

# Programs the bitstream, downloads the ELF, inits the PS, and runs the test.
# Programming and running can't be split into separate Make targets: the ELF
# must be downloaded before ps7_init runs (see sw/run_gpio_test.tcl), so sw's
# `run` target does both in one xsdb session. This phony target always
# executes, even when build was a no-op -- but within that session, the PL
# bitstream flash itself is skipped when unchanged (sw/.program_stamp) and the
# FPGA already reports configured via a live `fpga -state` query. The ELF
# download, PS init, and test execution always run regardless.
run: build
	$(MAKE) -C sw run

clean:
	$(MAKE) -C arty-z7-soc clean
	$(MAKE) -C sw clean

help:
	@echo "Usage: make [target]"
	@echo "  all        - Build hw+sw if needed, then program and run on hardware (default)"
	@echo "  build      - Build bitstream and ELF only, skipping steps already up to date"
	@echo "  run        - Same as 'all'"
	@echo "  linux-boot - Build the Linux-boot-from-SD-card test (see linux/README)"
	@echo "  step-01-uart      - Run bare-metal UART smoke test"
	@echo "  step-02-ddr       - Run bare-metal DDR pattern test"
	@echo "  step-03-buttons   - Run bare-metal button sampling test"
	@echo "  step-04-timer     - Run bare-metal ARM global timer test"
	@echo "  steps-working     - Run all implemented step tests"
	@echo "  regress-baremetal - Run implemented bare-metal regression tests with summary"
	@echo "  clean      - Clean hw and sw build artifacts"
