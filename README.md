# Arty Z7-10 SoC — Bare-Metal Diagnostic + Linux Boot Test

Two independent tests on a Digilent Arty Z7-10: a bare-metal ARM Cortex-A9 diagnostic suite (no OS, no FSBL, loaded directly over JTAG — the default `make` target) and a full Linux boot from SD card (FSBL + U-Boot + kernel + BusyBox, built from source, running from DDR).

## Hardware

- **Board**: Digilent Arty Z7-10 (Xilinx Zynq-7010)
- **LEDs**: 4 user LEDs, active high (pins R14, P14, N16, M14)
- **Boot mode**: JP4 in JTAG position for the bare-metal test (below); SD position for the Linux boot test (see `linux/`)

## Quickstart

```
make
```

Builds the `arty-z7-soc` bitstream and `sw` firmware if their sources changed (skipped otherwise), then programs and runs on hardware. Requires `hw_server` running and the board connected over JTAG. The PL bitstream flash itself is also skipped when the bitstream is unchanged and the FPGA already reports configured (checked live via `xsdb`, not just a timestamp) — everything else (ELF download, PS init, test execution) always runs. See `make help` for other targets (`build`, `clean`).

For step-by-step bare-metal diagnostics, keep a serial terminal open at 115200 baud on `/dev/ttyUSB1`, then run the numbered targets from the repo root:

```
make step-01-uart
make step-02-ddr
make step-03-buttons
make step-04-timer
make step-05-gic
make step-06-axi-timer
make step-07-custom-axi
```

`make steps-working` runs all currently implemented numbered steps.
`make regress-baremetal` runs every implemented bare-metal test, keeps going after failures, and prints a final PASS/FAIL/SKIP summary.

The top-level numbered targets first build any stale hardware/software outputs, then run the selected JTAG test. The `sw/` targets also preflight the required bitstream and `ps7_init.tcl`; if either is missing, they fail with a direct rebuild hint instead of an opaque `xsdb` error. During execution, the `xsdb` harness checks the live FPGA state and programs the PL when the bitstream is missing from the device or changed from the last stamped run.

## Projects

### `arty-z7-counter/` — PL-only LED counter

Pure RTL design (no PS). 8-bit binary counter drives the 4 LEDs at ~1 step/second. BTN0 resets.

```
cd arty-z7-counter && make all && make program
```

### `arty-z7-soc/` — Zynq SoC with AXI GPIO + AXI Timer + custom AXI-Lite

Zynq PS7 + AXI GPIO + AXI Timer + custom AXI-Lite block design. Exposes LEDs at `0x41200000`, buttons at `0x41210000`, AXI Timer 0 at `0x42800000`, and the custom AXI-Lite register block at `0x43C00000`.

```
cd arty-z7-soc && make all && make program
```

Pre-built bitstream and XSA are committed in `arty-z7-soc/hw/`.

### `sw/` — Bare-metal diagnostic suite

Runs on ARM Cortex-A9 core #0 from on-chip SRAM (OCM). UART0 is the primary debug channel, so keep a 115200 baud serial terminal open on the board's second USB-serial port while running tests. The suite currently reports each stage over UART and tests:

- AXI GPIO LED write/readback through the PL.
- AXI GPIO button sampling, with observed high/low masks printed over UART.
- ARM global timer sanity.
- GIC interrupt delivery using the Cortex-A9 private timer.
- AXI Timer PL peripheral counting over PS `M_AXI_GP0`.
- Custom AXI-Lite ID/scratch/counter/status registers over PS `M_AXI_GP0`.
- DDR memory at `0x00100000` using four 64 KiB pattern passes.

The LEDs show coarse stage/fail state, and the `xsdb` harness still polls an OCM PASS/FAIL sentinel so automation does not depend on watching UART manually.

```
cd sw && make
```

**To run on hardware:**

```
cd sw && make run
```

This programs the SoC bitstream (unless it's unchanged and the FPGA is already configured), downloads the ELF over JTAG, initializes the PS, and starts execution. Watch UART0 at 115200 baud for detailed stage output; LEDs show coarse progress/fail state. Note that `arty-z7-soc`'s `make program` only programs the FPGA fabric — it does not init the PS or load/run software, so use `sw`'s `make run` (or the top-level `make`, or `xsdb sw/run_gpio_test.tcl` directly) to actually see the test execute.

Focused targets are also available:

```
cd sw && make run-uart   # UART-only smoke test, no PL/AXI dependency
cd sw && make run-ddr    # DDR pattern test, reported over UART
cd sw && make run-gpio   # AXI GPIO LED write/readback test
cd sw && make run-buttons # AXI GPIO button sampling test
cd sw && make run-timer  # ARM global timer sanity test
cd sw && make run-gic    # GIC/private timer interrupt test
cd sw && make run-axi-timer # AXI Timer PL peripheral test
cd sw && make run-custom-axi # custom AXI-Lite register test
cd sw && make run        # full UART + AXI GPIO + buttons + timer + GIC + AXI Timer + custom AXI + DDR suite
cd sw && make regress-baremetal # all implemented tests with summary
```

#### Bare-metal 10-step plan

Run these from the repo root. Every implemented step reports progress over UART and also writes the OCM PASS/FAIL sentinel used by the `xsdb` harness.

| Step | Target | Status | What it proves |
|------|--------|--------|----------------|
| 1 | `make step-01-uart` | Working | UART0 output is readable at 115200 baud after `ps7_init` |
| 2 | `make step-02-ddr` | Working | DDR is usable from OCM-loaded bare-metal code; four 64 KiB pattern passes at `0x00100000` |
| 3 | `make step-03-buttons` | Working | PS-to-PL AXI GPIO can sample the four user buttons and report observed high/low masks |
| 4 | `make step-04-timer` | Working | ARM global timer counts while bare-metal code runs |
| 5 | `make step-05-gic` | Working | GIC setup and IRQ entry/return using Cortex-A9 private timer interrupt ID 29 |
| 6 | `make step-06-axi-timer` | Working | AXI Timer IP in PL at `0x42800000` counts when accessed over PS `M_AXI_GP0` |
| 7 | `make step-07-custom-axi` | Working | Custom AXI-Lite block at `0x43C00000` exposes ID, scratch, counter, and derived status registers |
| 8 | `make step-08-axi-bram` | Planned | AXI BRAM controller memory-pattern test through PS-to-PL AXI |
| 9 | `make step-09-cache-mmu` | Planned | MMU/cache enable smoke test without breaking DDR or AXI MMIO |
| 10 | `make step-10-sd-raw` | Planned | Bare-metal SD0 raw-sector read without U-Boot or Linux |

Planned targets intentionally fail fast with a clear "not implemented yet" message until their firmware/hardware support exists.

For a board-level smoke regression, use:

```
make regress-baremetal
```

This runs UART, DDR, GPIO, buttons, timer, GIC, AXI Timer, custom AXI-Lite, and the full suite. It reports each implemented test as `PASS` or `FAIL`, reports steps 8-10 as `SKIP` until implemented, and exits nonzero if any implemented test fails.

### `linux/` — Linux boot from SD card

Full boot chain built from source (no PetaLinux): FSBL, U-Boot, Linux kernel, and a minimal BusyBox initramfs, targeting the same `arty-z7-soc` hardware but running from DDR instead of OCM. **Confirmed working** on real hardware — boots to an interactive BusyBox shell over UART.

#### 1. Host packages

`bc` is required for the kernel build and is usually not installed by default:

```
sudo pacman -S bc   # or your distro's equivalent
```

`bsdtar` (from `libarchive`) is also required, to package the initramfs — see `AGENTS.md` if `cpio` isn't installed either.

#### 2. Build the boot chain

```
cd linux && make all
```

Clones and builds U-Boot, the Linux kernel, and BusyBox from source (first run takes a while), then packages `BOOT.BIN` (FSBL + U-Boot) and `boot.scr` (U-Boot boot script).

#### 3. Prepare the SD card

Insert an SD card (8GB+) into a reader on your PC and identify its device node — **be certain you have the right device before formatting**, this is destructive:

```
lsblk    # find your card, e.g. /dev/sda — confirm size/model, don't guess
```

Format it as a single FAT32 partition (replace `/dev/sdX1` with your card's first partition):

```
sudo mkfs.vfat -F 32 -n BOOT /dev/sdX1
```

Mount it, then copy the boot files over:

```
make -C linux sdcard SDMNT=/path/to/mounted/fat32/partition
```

`sdcard` only copies files onto an **already-formatted, already-mounted** partition — it deliberately never touches a raw block device itself, so the format step above is on you.

#### 4. Boot on hardware

Can't be done over JTAG — needs physical access:

1. Move jumper JP4 from JTAG to SD boot mode.
2. Insert the prepared SD card into the board's microSD slot.
3. Open a serial terminal at 115200 baud, 8N1, no flow control, on the board's *second* USB-serial port (the same USB cable used for JTAG also carries UART0 — the board's FTDI chip exposes both, typically `/dev/ttyUSB1` on Linux):
   ```
   picocom -b 115200 /dev/ttyUSB1
   ```
4. Power-cycle the board (or press the **PORB** button — confirmed to trigger a full reboot; **SRST** did not produce any UART output in testing, use PORB or a full power cycle instead).
5. Watch U-Boot autoboot and the kernel boot to a `~ #` BusyBox shell.

See `AGENTS.md` for the full boot-chain breakdown, the Digilent board-preset details that make this work (correct DDR part, SD0/UART0 MIO pins), and — importantly — the `ps-clk-frequency` device-tree fix that's required to get a readable (non-garbled) serial console.

#### 5. Networking (Ethernet)

**Confirmed working** — bring the interface up and give it a few seconds for gigabit autonegotiation before checking status:

```
ifconfig eth0 up
sleep 3
cat /sys/class/net/eth0/carrier   # 1 once link is up
```

- **On a real network** (router/switch with DHCP): `udhcpc -i eth0` gets a lease normally.
- **Direct cable to a PC** (no DHCP server on either end): assign static IPs instead —
  ```
  ifconfig eth0 192.168.7.2 netmask 255.255.255.0 up   # on the board
  ```
  and a matching static IP (e.g. `192.168.7.1/24`) on the PC's Ethernet interface, then `ping` between them.

See `AGENTS.md` for why U-Boot's own "No ethernet found" at boot is expected/harmless (only the kernel's Ethernet driver supports the PHY reset line on this board).

#### 6. SSH

**Confirmed working.** The initramfs runs a statically cross-compiled Dropbear SSH server, started automatically at boot. Once the board has an IP (see above), just:

```
ssh root@192.168.7.2   # or whatever IP the board has
```

No password, no prompt — logs straight in as root. This is a deliberate, minimal setup for an isolated test board only: blank-password login, and every image built from this repo shares the same SSH host key (see `AGENTS.md` for why, and for the caveats on both).

## Toolchain

- Vivado / Vitis 2026.1
- `arm-none-eabi-gcc` (Xilinx GNU Toolchain, bundled with Vitis) — bare-metal test
- `arm-linux-gnueabihf-gcc` (Xilinx GNU Toolchain, bundled with Vitis) — Linux boot test

## Key JTAG Insight

OCM at `0xFFFC0000` is blocked by the DAP AXI-AP. In JTAG boot mode, Zynq aliases OCM to `0x00000000`, which the DAP can write. The ELF is built to load at `0x00000000` (`lscript_low.ld`) and must be downloaded **before** `ps7_init` runs — PLL reconfiguration during `ps7_init` corrupts the DAP APB-AP, blocking all subsequent memory writes.
