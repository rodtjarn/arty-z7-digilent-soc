# Arty Z7-10 Counter build system
VIVADO      := vivado
SCRIPT_DIR  := scripts
SRC_DIR     := src
BITSTREAM   := bin/top.bit

.PHONY: all
all: $(BITSTREAM)

$(BITSTREAM): $(SRC_DIR)/counter.sv $(SRC_DIR)/top.sv $(SRC_DIR)/top.xdc $(SCRIPT_DIR)/build.tcl
	mkdir -p bin
	cd $(SCRIPT_DIR) && $(VIVADO) -mode batch -source build.tcl -notrace

.PHONY: program
program: $(BITSTREAM)
	cd $(SCRIPT_DIR) && $(VIVADO) -mode batch -source program.tcl -notrace

.PHONY: clean
clean:
	rm -rf bin

.PHONY: help
help:
	@echo "Usage: make [target]"
	@echo "  all     - Build the counter bitstream"
	@echo "  program - Program the bitstream to the Arty Z7-10"
	@echo "  clean   - Remove generated files"