# AGENTS.md

## Board Info

- **Board**: Digilent Arty Z7-10 (xc7z010clg400-1)
- **FPGA**: Xilinx Zynq-7010 (28K logic cells, 80 DSP slices, 120 BRAM)
- **Clock**: 125 MHz SYSCLK on pin H16 (LVCMOS33)
- **LEDs**: 4 user LEDs (R14, P14, N16, M14) — active high
- **Buttons**: 4 push buttons (D19=BTN0, etc.) — active high
- **Boot mode**: JP4 in JTAG position (MIO[5:4]=00) for the bare-metal test below; must be moved to SD position for the Linux boot test (see `linux/`)
- **PS_CLK oscillator**: **50 MHz** (Digilent Arty Z7 Reference Manual, "Clock Sources" — *not* the 33.333 MHz assumed by upstream `zynq-7000.dtsi` boilerplate or other Zynq boards' overlays). `apply_board_preset`'s raw PLL `FBDIV` values (ARM=26, DDR=21, IO=20) are computed against this real 50 MHz input and are correct — at 50 MHz they land exactly on 1300/1050/1000 MHz respectively. The PS7 IP's own GUI "target frequency" properties (`PCW_*_FREQMHZ`) are stale display labels computed against an assumed 33.333 MHz PS_CLK and will look like they don't match the FBDIV-derived actual frequency; **this is cosmetic, do not "fix" it** by touching `PCW_UART_PERIPHERAL_FREQMHZ` or any PLL `FBDIV`/`FREQMHZ` property — see the Linux Boot gotcha below for what actually breaks if you get this wrong, and where the real fix belongs (device tree, not Vivado).
- **PS7 config**: matches Digilent's official Vivado board preset (`apply_board_preset` in `arty-z7-soc/scripts/build.tcl`, sourced from `<Vivado>/data/boards/board_files/arty-z7-10/A.0/preset.xml`) — DDR3 `MT41J256M16` (512MB, 16-bit bus), UART0 on MIO 14..15 (routed through the same USB connector as JTAG), SD0 on MIO 40..45 (card detect MIO 47), QSPI and ENET0 also enabled but unused by either test

## Host Package Requirements

The bare-metal test (`sw/`) and hardware build (`arty-z7-soc/`) need only the bundled Xilinx/Vivado/Vitis tools above. The Linux boot chain (`linux/`) additionally needs these host packages (checked on this Arch Linux machine via `which`/`pacman -Qs`):

| Package | Status | Needed for |
|---------|--------|-------------|
| `git` | present | cloning `u-boot-xlnx`, `linux-xlnx`, `busybox` |
| `gcc`, `flex`, `bison`, `make` | present | host-side build tools invoked by the kernel/U-Boot/BusyBox build systems (`scripts/kconfig`, `scripts/dtc`, etc.) — separate from the `arm-linux-gnueabihf-` cross-compiler used for the target binaries |
| `bsdtar` (`libarchive`) | present | packaging the initramfs as a cpio archive (`cpio` itself is **not** installed; `bsdtar --format=newc` is used instead — `bsdtar --format cpio` defaults to the old ASCII "odc" variant, magic `070707`, which the kernel's initramfs unpacker rejects with "use -H newc option"; `--format=newc` emits the `070701` magic it actually expects) |
| `bc` | **not installed** — install with `sudo pacman -S bc` | kernel build (`include/generated/timeconst.h`); build fails with `Kbuild:24: ... Error 127` without it |

## Toolchain

| Tool | Path |
|------|------|
| ARM GCC (bare-metal) | `/home/per/tools/Xilinx/2026.1/2026.1/gnu/aarch32/lin/gcc-arm-none-eabi/bin/arm-none-eabi-gcc` |
| ARM GCC (Linux, hard-float) | `/home/per/tools/Xilinx/2026.1/2026.1/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-gcc` |
| xsdb | `/home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/xsdb` |
| Vitis (Python CLI) | `/home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/vitis -s <script.py>` — classic `xsct` project-creation API is disabled in 2026.1; used for FSBL generation (see `linux/fsbl/build_fsbl.py`) |
| bootgen | `/home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/bootgen` |
| Vivado | `vivado` (in PATH via sourced settings) |
| hw_server | `/home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/hw_server` (or started automatically by Vivado) |
| xlsclients shim | a no-op `xlsclients` script placed ahead of Vitis on `PATH` (Arch Linux fix — Vitis's headless tools probe for this X11 utility even in batch mode; not packaged on this system). `linux/Makefile` creates one at `linux/.xlsclients-shim/` automatically. |

## PL-only Counter (arty-z7-counter/)

RTL template: 8-bit binary counter driving 4 LEDs at ~1 s/step. Press BTN0 to reset.

```
make all      # synth → place → route → bitstream
make program  # program via JTAG
make clean
```

| File | Purpose |
|------|---------|
| `src/counter.sv` | 8-bit counter with clock enable and reset |
| `src/top.sv` | 23-bit clock divider + counter + LED mapping |
| `src/top.xdc` | Pin constraints |
| `scripts/build.tcl` | Batch build |
| `scripts/program.tcl` | JTAG programming |
| `bin/top.bit` | Pre-built bitstream |

## Zynq SoC (arty-z7-soc/)

Zynq PS7 + AXI GPIO: 4-bit LED output at `0x41200000`, 4-bit button input at `0x41210000`.

```
make all      # create BD, generate targets, synth, impl, bitstream + XSA
make program  # program via JTAG
make clean
```

| File | Purpose |
|------|---------|
| `src/top.xdc` | Pin constraints for led[3:0] and btn[3:0] |
| `scripts/build.tcl` | Batch build script (BD → wrapper → synth → impl → bitstream) |
| `scripts/program.tcl` | JTAG programming |
| `hw/arty_z7_soc.bit` | Pre-built bitstream |
| `hw/arty_z7_soc.xsa` | XSA (contains ps7_init data) |
| `hw/ps7_init.tcl` | PS7 initialization script (sourced by xsdb) |

### Critical Build Lessons

- **Never call `synth_design`/`opt_design`/`place_design`/`route_design` directly on the BD wrapper** — always use `launch_runs synth_1` then `launch_runs impl_1 -to_step write_bitstream`. AXI GPIO (and other) IPs default to `GENERATE_SYNTH_CHECKPOINT=1` (out-of-context synthesis). Calling `synth_design` directly on the flattened wrapper skips the OOC run that produces each IP's checkpoint, so elaboration fails with `[Synth 8-439] module 'design_1_axi_gpio_btn_0' not found` (or similar) even though the IP's `.xci`/generated sources are present and correctly registered.
- **`generate_target all [get_files *.bd]` is sufficient** — do not also manually `add_files -norecurse` the generated `ip/*/synth/*.vhd` or `ipshared/*/hdl/*.vhd` files. `generate_target` already registers them in the project with the correct VHDL library associations as part of each IP's sub-design scope; re-adding them explicitly only produces `CRITICAL WARNING: [filemgmt 20-1440]` and risks corrupting that scope.
- **Wrapper must be explicitly added** — `add_files -norecurse` on the wrapper path + `set_property top design_1_wrapper`.
- **Bitstream/XSA come from the run, not from manual `write_bitstream`** — after `launch_runs impl_1 -to_step write_bitstream`, copy `[get_property DIRECTORY [get_runs impl_1]]/design_1_wrapper.bit` to the desired output path, and `open_run impl_1` before `write_hw_platform`.

## Bare-Metal Software (sw/)

GPIO test: sets LED GPIO as output, blinks 0xA ↔ 0x5 (~500 ms each). Static 0x1 or 0x2 = FAIL.

### Build

```
cd sw && make
```

Requires ARM GCC in PATH, or set `CC` to the full path. Produces `gpio_test_low.elf` (use this for JTAG).

| File | Purpose |
|------|---------|
| `sw/gpio_test.c` | Main test: FCLK enable, LED/BTN GPIO config, blink loop with readback |
| `sw/startup.s` | Reset vector + stack init (5 lines) |
| `sw/lscript_low.ld` | **Use this**: OCM at `0x00000000` (JTAG boot alias), stack at `0x0000FFFC` |
| `sw/lscript.ld` | OCM at `0xFFFC0000` — DAP-blocked, cannot be used from xsdb |
| `sw/run_gpio_test.tcl` | Complete xsdb script: program → download → init → run |
| `sw/Makefile` | Builds both `gpio_test.elf` and `gpio_test_low.elf` |

### Run on Hardware

```
xsdb sw/run_gpio_test.tcl
```

The script does exactly this, in this order:

1. `connect` to hw_server
2. Select `ARM Cortex-A9 MPCore #0` → `stop`
3. Select `xc7z010` → `fpga arty_z7_soc.bit`
4. Back to ARM core → **`dow gpio_test_low.elf`** (loads to `0x00000000`)
5. `source ps7_init.tcl` → `ps7_init` → `ps7_post_config`
6. `rwr pc 0x00000020; rwr sp 0x0000FFFC; con`

### Critical JTAG Rules

- **`dow` must come before `ps7_init`** — ps7_init reconfigures PLLs which corrupts the DAP APB-AP (status 0xF0000021). After that, all memory writes fail. `rwr`/`con` still work.
- **Use `lscript_low.ld` / `gpio_test_low.elf`** — OCM at `0xFFFC0000` is DAP-blocked. In JTAG boot mode, the same OCM is aliased to `0x00000000` and is writable.
- **FCLK_CLK0 is not enabled by `ps7_init`** — `gpio_test.c` calls `enable_fclk()` which sets it from ARM code. The DAP cannot set this bit (silently ignored).
- **Power cycle required** to recover from DAP corruption (status 0xF0000021).
- **xsdb replaces xsct** in Vitis 2026.1.

### Vivado Notes

- Board part: `digilentinc.com:arty-z7-10:part0:1.1`
- Part: `xc7z010clg400-1`
- Use `vivado -mode batch -source <script>.tcl -notrace` for headless builds

## Linux Boot from SD Card (linux/)

Full boot chain built from source (no PetaLinux): FSBL + U-Boot + Linux kernel + BusyBox initramfs, packaged as `BOOT.BIN` + `boot.scr` + `zImage`/`devicetree.dtb`/`uramdisk.image.gz` for an SD card. Independent of the bare-metal `sw/` test above — `make` at the repo root still defaults to bare-metal/JTAG.

```
cd linux && make all              # build the whole chain
make -C linux sdcard SDMNT=/path/to/mounted/fat32/partition
```

`sdcard` only copies files onto an **already-formatted, already-mounted** FAT32 partition (`SDMNT` must be a mountpoint, not a raw device) — partitioning/formatting the card is left to the user, deliberately, since scripting writes to a raw block device is destructive if the wrong device is picked.

### Boot Sequence (BootROM → FSBL → U-Boot → kernel)

1. Power-on reset latches the JP4 jumper state into an SLCR mode register, telling BootROM this is SD boot mode. (A later SRST reruns BootROM but reuses the already-latched value — JP4 changes need a real power cycle to take effect, not just SRST.)
2. **BootROM** (silicon-masked ROM, immutable, not user code, no JTAG involved in SD boot mode) does minimal SD controller setup, reads `BOOT.BIN`'s boot header off the SD card, and copies the FSBL partition into OCM (256KB on-chip SRAM) — the only memory usable this early, since DDR isn't configured yet. It then jumps to the FSBL's entry point in OCM.
3. **FSBL** runs entirely from OCM. Its first job is `ps7_init()` (`arty-z7-soc/hw/ps7_init.c`): PLLs, peripheral clocks, MIO pinmux, and — critically — DDR3 controller/PHY calibration against the board's actual DDR chips. Only after this completes does DDR become valid memory for the first time in the boot.
4. FSBL then reads `u-boot.elf` off the SD card and copies it into DDR (U-Boot doesn't fit in the 256KB OCM), and jumps to U-Boot's entry point *in DDR*. FSBL/OCM's job is done at this point — nothing after this uses OCM again.
5. **U-Boot**, running from DDR, autoboots `boot.scr`, which `fatload`s `zImage`, `devicetree.dtb`, and `uramdisk.image.gz` off the SD card into fixed DDR addresses (observed in practice: kernel `@ 0x200000`, FDT `@ 0x1000000`, ramdisk `@ 0x2000000`), then `bootz` jumps into the kernel entry point, also in DDR.
6. **Kernel**, running from DDR, unpacks the initramfs cpio archive directly into a DDR-backed rootfs and execs `/init` (PID 1) — reaching the `~ #` shell.

Note: SD (and QSPI, an unused third boot mode on this board) is never memory-mapped or executed in place — BootROM/FSBL/U-Boot each treat it purely as a block/file device to *copy* bytes from. OCM and DDR are the only two places anything ever actually executes; the whole point of stage 0–1 is bridging "only OCM is usable" to "DDR is finally ready."

| File | Purpose |
|------|---------|
| `linux/fsbl/build_fsbl.py` | Vitis Python-client script: creates a platform from `arty-z7-soc/hw/arty_z7_soc.xsa` and builds the FSBL (`fsbl/fsbl.elf`) |
| `linux/u-boot/zynq-arty-z7.dts` | Custom board DTS (memory=512MB, uart0, sdhci0) layered on U-Boot's `zynq-7000.dtsi`; copied into the cloned `u-boot-xlnx` tree on first build. **`ps-clk-frequency` must be `<50000000>`** — see gotcha below |
| `linux/kernel/zynq-arty-z7.dts` | Same, for `linux-xlnx`'s device tree. Same `ps-clk-frequency` requirement |
| `linux/rootfs/init` | PID1 shell script for the initramfs (mounts proc/sys, execs a shell) |
| `linux/boot/boot.bif` | bootgen image description: `[bootloader]fsbl.elf` + `u-boot.elf` → `BOOT.BIN` (bitstream deliberately not included — SD0/UART0/DDR are PS-only, exposing the PL AXI GPIO LEDs to Linux is a follow-up, not done here) |
| `linux/boot/boot.cmd`/`boot.scr` | U-Boot script: `fatload` zImage/dtb/initramfs from the SD card, `bootz` — written explicitly rather than relying on U-Boot's default `distro_bootcmd`, which expects different file naming |
| `linux/Makefile` | Orchestrates all of the above; `make clean` removes all cloned sources and build output |

### Key Choices / Gotchas

- **`ps-clk-frequency` must be `<50000000>` in both `linux/u-boot/zynq-arty-z7.dts` and `linux/kernel/zynq-arty-z7.dts` — this is the single most important fix in this whole boot chain, and getting it wrong produces a fully garbled serial console that looks like a baud-rate/hardware problem but isn't.** Symptom: FSBL's own `ps7_init`-programmed UART divisors are correct and don't depend on this property, so the console is briefly fine, but U-Boot (and the kernel) read `ps-clk-frequency` from the DT to compute every PLL-derived clock live from the SLCR registers — including the UART reference clock — and reprogram the baud generator early in boot. With the wrong (33.333 MHz, copied from generic upstream board overlays) value, U-Boot computes IO_PLL as 666.67 MHz instead of the real 1000 MHz (FBDIV=20 × the *real* 50 MHz PS_CLK), miscalculates the baud divisor, and clobbers the UART into transmitting at the wrong rate — permanently garbling the console from that point on, indistinguishable at first glance from a genuine clock-tree misconfiguration. **Do not chase this by touching Vivado/PS7 properties** (see the PS_CLK note under Board Info) — the PS7 hardware config is already correct; only the device tree's stated oscillator frequency was wrong. Fix: `ps-clk-frequency = <50000000>;` in both DTS files, then rebuild U-Boot (full rebuild, DTB is compiled in) and the kernel `dtbs` target, and if the source trees are already cloned, manually re-copy the DTS into `u-boot/src/arch/arm/dts/` and `kernel/src/arch/arm/boot/dts/xilinx/` first — the Makefile only copies it in on first clone.
- **Reset buttons**: **PORB confirmed working** — full power-on-style reset, re-runs BootROM → FSBL → U-Boot → kernel, produces a fresh boot log over UART. **SRST was tested and produced zero UART output**, contrary to the reference manual's description (system reset without re-latching JP4); inconclusive whether that's a hardware quirk on this board or the wrong button was pressed — use PORB or a full power cycle for a reliable reset until this is investigated further.
- **No Arty Z7 defconfig upstream in u-boot-xlnx/linux-xlnx.** Both now use one generic `xilinx_zynq_virt_defconfig` / `xilinx_zynq_defconfig` plus a per-board device tree (`DEVICE_TREE=zynq-arty-z7` for U-Boot; kernel picks up any `.dts` registered in `arch/arm/boot/dts/xilinx/Makefile`). The closest upstream board (Digilent Zybo Z7, same SoC family) uses `uart1`; **Arty Z7's Digilent board preset uses `uart0`** — copying the Zybo Z7 dts wholesale would pick the wrong console UART.
- **xsct project creation is disabled in Vitis 2026.1** ("`use the Vitis Python Console for script-based projects creation and build`"). FSBL is generated via the Python `vitis` module (`client.create_platform_component(...)`, see `linux/cli/examples/embedded/platform_uc4_zynq.py` for the pattern this is based on) — not the classic `xsct sdk create_app_project` API described in older Xilinx docs.
- **BusyBox must be installed via `make CONFIG_PREFIX=... install`, not `--install -s`** — the latter requires *executing* the freshly-built ARM binary to enumerate its applets, which doesn't work when cross-compiling on an x86_64 host with no `qemu-arm` available. `make install` derives the applet symlink list from the build config instead, no execution needed.
- See "Host Package Requirements" above for the `bc`/`bsdtar` host-package notes.
- Physical steps to actually boot (cannot be automated over JTAG): move JP4 to the SD boot position, insert the prepared SD card, open a serial terminal at 115200 baud on the *second* USB-serial port exposed by the board's FTDI chip (the first is JTAG; `lsusb` shows an FT2232 dual-UART/FIFO device providing both), then power-cycle.
- **Confirmed working end-to-end** (2026-07-06): with the `ps-clk-frequency` and `bsdtar --format=newc` fixes above, the board boots cleanly from a real SD card through FSBL → U-Boot (autoboots `boot.scr`) → kernel → BusyBox initramfs to an interactive `~ #` shell over UART, and correctly enumerates the boot SD card itself as `mmcblk0` from within Linux.
