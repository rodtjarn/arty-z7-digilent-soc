# Arty Z7-10 SoC — Bare-Metal GPIO Test

Bare-metal ARM Cortex-A9 firmware running on a Digilent Arty Z7-10, toggling LEDs via AXI GPIO — no OS, no FSBL, loaded directly over JTAG.

## Hardware

- **Board**: Digilent Arty Z7-10 (Xilinx Zynq-7010)
- **LEDs**: 4 user LEDs, active high (pins R14, P14, N16, M14)
- **Boot mode**: JP4 in JTAG position

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

## Toolchain

- Vivado / Vitis 2026.1
- `arm-none-eabi-gcc` (Xilinx GNU Toolchain, bundled with Vitis)

## Key JTAG Insight

OCM at `0xFFFC0000` is blocked by the DAP AXI-AP. In JTAG boot mode, Zynq aliases OCM to `0x00000000`, which the DAP can write. The ELF is built to load at `0x00000000` (`lscript_low.ld`) and must be downloaded **before** `ps7_init` runs — PLL reconfiguration during `ps7_init` corrupts the DAP APB-AP, blocking all subsequent memory writes.
