# Arty Z7-10 SoC — Bare-Metal GPIO Test + Linux Boot Test

Two independent tests on a Digilent Arty Z7-10: a bare-metal ARM Cortex-A9 GPIO blinker (no OS, no FSBL, loaded directly over JTAG — the default `make` target) and a full Linux boot from SD card (FSBL + U-Boot + kernel + BusyBox, built from source, running from DDR).

## Hardware

- **Board**: Digilent Arty Z7-10 (Xilinx Zynq-7010)
- **LEDs**: 4 user LEDs, active high (pins R14, P14, N16, M14)
- **Boot mode**: JP4 in JTAG position for the bare-metal test (below); SD position for the Linux boot test (see `linux/`)

## Quickstart

```
make
```

Builds the `arty-z7-soc` bitstream and `sw` firmware if their sources changed (skipped otherwise), then programs and runs on hardware. Requires `hw_server` running and the board connected over JTAG. The PL bitstream flash itself is also skipped when the bitstream is unchanged and the FPGA already reports configured (checked live via `xsdb`, not just a timestamp) — everything else (ELF download, PS init, test execution) always runs. See `make help` for other targets (`build`, `clean`).

## Projects

### `arty-z7-counter/` — PL-only LED counter

Pure RTL design (no PS). 8-bit binary counter drives the 4 LEDs at ~1 step/second. BTN0 resets.

```
cd arty-z7-counter && make all && make program
```

### `arty-z7-soc/` — Zynq SoC with AXI GPIO

Zynq PS7 + AXI GPIO block design. Exposes LEDs at `0x41200000` and buttons at `0x41210000`.

```
cd arty-z7-soc && make all && make program
```

Pre-built bitstream and XSA are committed in `arty-z7-soc/hw/`.

### `sw/` — Bare-metal GPIO test

Runs on ARM Cortex-A9 core #0 from on-chip SRAM (OCM). Blinks LEDs in a 0xA ↔ 0x5 pattern (~500 ms each). Static 0x1 or 0x2 indicates a readback failure.

```
cd sw && make
```

**To run on hardware:**

```
cd sw && make run
```

This programs the SoC bitstream (unless it's unchanged and the FPGA is already configured), downloads the ELF over JTAG, initializes the PS, and starts execution. LEDs blink immediately. Note that `arty-z7-soc`'s `make program` only programs the FPGA fabric — it does not init the PS or load/run software, so use `sw`'s `make run` (or the top-level `make`, or `xsdb sw/run_gpio_test.tcl` directly) to actually see the test execute.

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
