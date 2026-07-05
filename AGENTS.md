# AGENTS.md

## Board Info

- **Board**: Digilent Arty Z7-10 (xc7z010clg400-1)
- **FPGA**: Xilinx Zynq-7010 (28K logic cells, 80 DSP slices, 120 BRAM)
- **Clock**: 125 MHz SYSCLK on pin H16 (LVCMOS33)
- **LEDs**: 4 user LEDs (R14, P14, N16, M14) — active high
- **Buttons**: 4 push buttons (D19=BTN0, etc.) — active high
- **Boot mode**: JP4 in JTAG position (MIO[5:4]=00)

## Toolchain

| Tool | Path |
|------|------|
| ARM GCC | `/home/per/tools/Xilinx/2026.1/2026.1/gnu/aarch32/lin/gcc-arm-none-eabi/bin/arm-none-eabi-gcc` |
| xsdb | `/home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/xsdb` |
| Vivado | `vivado` (in PATH via sourced settings) |
| hw_server | `/home/per/tools/Xilinx/2026.1/2026.1/Vitis/bin/hw_server` (or started automatically by Vivado) |
| xlsclients shim | `/tmp/opencode/bin/xlsclients` (Arch Linux fix for Vitis dependency) |

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

- **Never use `synth_design` directly on BDs** — always use `launch_runs synth_1`.
- **`generate_target all [get_files *.bd]` alone is insufficient** — also call `generate_target {synth} [get_files *.xci]`.
- **Wrapper must be explicitly added** — `add_files -norecurse` on the wrapper path + `set_property top design_1_wrapper`.
- **VHDL IP files must be explicitly added** — `add_files -norecurse` on all `ip/*/synth/*.vhd` and `ipshared/*/hdl/*.vhd`.

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
