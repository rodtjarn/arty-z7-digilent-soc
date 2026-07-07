# Arty Z7-10 SoC build system (Zynq PS + AXI GPIO + AXI Timer + custom AXI-Lite + AXI BRAM)
VIVADO      := vivado
SCRIPT_DIR  := scripts
SRC_DIR     := src
HW_DIR      := hw
BITSTREAM   := $(HW_DIR)/arty_z7_soc.bit
XSA         := $(HW_DIR)/arty_z7_soc.xsa

.PHONY: all
all: $(BITSTREAM)

$(BITSTREAM): $(SCRIPT_DIR)/build.tcl $(SRC_DIR)/top.xdc $(SRC_DIR)/custom_axi_lite.v
	mkdir -p $(HW_DIR)
	cd $(SCRIPT_DIR) && $(VIVADO) -mode batch -source build.tcl -notrace

.PHONY: program
program: $(BITSTREAM)
	cd $(SCRIPT_DIR) && $(VIVADO) -mode batch -source program.tcl -notrace

.PHONY: export
export: $(XSA)

$(XSA): $(BITSTREAM)
	@echo "XSA already generated during build"

.PHONY: clean
clean:
	rm -rf $(HW_DIR) .Xil

.PHONY: help
help:
	@echo "Usage: make [target]"
	@echo "  all     - Build block design and generate bitstream + XSA"
	@echo "  program - Program the bitstream to the Arty Z7-10"
	@echo "  clean   - Remove generated files"
