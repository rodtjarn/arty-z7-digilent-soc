# Build script for Arty Z7-10 SoC with PS + AXI GPIO + AXI Timer

set project_dir [file normalize "../hw"]
set src_dir     [file normalize "../src"]

# Create project targeting Arty Z7-10
create_project -force arty_z7_soc $project_dir -part xc7z010clg400-1
set_property board_part digilentinc.com:arty-z7-10:part0:1.1 [current_project]
set_property target_language Verilog [current_project]

# Enable automatic source management to pick up generated IP sources
set_property source_mgmt_mode All [current_project]

# Create block design
create_bd_design design_1

# Add Zynq Processing System
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ps7
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {
    make_external "FIXED_IO, DDR"
    apply_board_preset "1"
} [get_bd_cells ps7]

# Configure PS FCLK to 50 MHz for PL peripherals
set_property -dict [list CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50}] [get_bd_cells ps7]


# Add AXI GPIO for LEDs (4-bit output)
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_led
set_property -dict [list \
    CONFIG.C_GPIO_WIDTH {4} \
    CONFIG.C_ALL_OUTPUTS {1} \
    CONFIG.C_IS_DUAL {0} \
] [get_bd_cells axi_gpio_led]

# Add AXI GPIO for buttons (4-bit input)
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_btn
set_property -dict [list \
    CONFIG.C_GPIO_WIDTH {4} \
    CONFIG.C_ALL_INPUTS {1} \
    CONFIG.C_IS_DUAL {0} \
] [get_bd_cells axi_gpio_btn]

# Add AXI Timer for bare-metal PL peripheral testing
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_0

# Create external ports for PL I/O
create_bd_port -dir O -from 3 -to 0 led
create_bd_port -dir I -from 3 -to 0 btn

connect_bd_net [get_bd_ports led] [get_bd_pins axi_gpio_led/gpio_io_o]
connect_bd_net [get_bd_ports btn] [get_bd_pins axi_gpio_btn/gpio_io_i]

# Connect AXI GPIO slaves to PS via automation (creates interconnect)
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
    Master /ps7/M_AXI_GP0
    Clk /ps7/FCLK_CLK0
} [get_bd_intf_pins axi_gpio_led/S_AXI]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
    Master /ps7/M_AXI_GP0
    Clk /ps7/FCLK_CLK0
} [get_bd_intf_pins axi_gpio_btn/S_AXI]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {
    Master /ps7/M_AXI_GP0
    Clk /ps7/FCLK_CLK0
} [get_bd_intf_pins axi_timer_0/S_AXI]

# Keep the PS-to-PL MMIO map stable for bare-metal software.
assign_bd_address -offset 0x41200000 -range 0x10000 -target_address_space [get_bd_addr_spaces ps7/Data] [get_bd_addr_segs axi_gpio_led/S_AXI/Reg] -force
assign_bd_address -offset 0x41210000 -range 0x10000 -target_address_space [get_bd_addr_spaces ps7/Data] [get_bd_addr_segs axi_gpio_btn/S_AXI/Reg] -force
assign_bd_address -offset 0x42800000 -range 0x10000 -target_address_space [get_bd_addr_spaces ps7/Data] [get_bd_addr_segs axi_timer_0/S_AXI/Reg] -force

# Validate block design
validate_bd_design

# Generate all IP output products
generate_target all [get_files *.bd]

# Create HDL wrapper and add to sources_1 explicitly
set wrapper_path "$project_dir/arty_z7_soc.gen/sources_1/bd/design_1/hdl/design_1_wrapper.v"
if {[file exists $wrapper_path]} {
    add_files -norecurse $wrapper_path
    set_property top design_1_wrapper [current_fileset]
} else {
    error "Wrapper file not found at $wrapper_path"
}

# NOTE: IP-generated VHDL netlist files (ip/*/synth/*.vhd, ipshared/*/hdl/*.vhd)
# are already registered in the project by generate_target as part of each IP's
# sub-design scope (with correct VHDL library assignments). Re-adding them
# explicitly via add_files corrupts that scope/library association and can
# cause synthesis to fail to resolve IP modules (Vivado warns about this).

# Update compile order to resolve all IP dependencies
update_compile_order -fileset sources_1

# Add pin constraints
add_files -fileset constrs_1 [file join $src_dir top.xdc]

# Synthesize and implement via the standard project-mode run flow. This is
# required (rather than calling synth_design directly on the wrapper) so that
# IP cores generated with out-of-context synthesis checkpoints (the default
# for axi_gpio/interconnect/etc.) get their own OOC synthesis run producing a
# .dcp that is automatically linked into the top-level synthesis run; calling
# synth_design directly skips that step and fails to resolve the IP modules.
launch_runs synth_1 -jobs [get_param general.maxThreads]
wait_on_run synth_1
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    error "Synthesis run synth_1 did not complete successfully"
}

launch_runs impl_1 -to_step write_bitstream -jobs [get_param general.maxThreads]
wait_on_run impl_1
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    error "Implementation run impl_1 did not complete successfully"
}

# Copy the bitstream out of the run directory to the top-level hw output dir
set run_bit [get_property DIRECTORY [get_runs impl_1]]/design_1_wrapper.bit
file copy -force $run_bit [file join $project_dir arty_z7_soc.bit]

# Export hardware (XSA) for Vitis/PetaLinux
open_run impl_1
write_hw_platform -fixed -include_bit -force -file [file join $project_dir arty_z7_soc.xsa]

# Copy the PS7 init files (ps7_init.tcl/.c/.h + GPL variants) generated
# alongside the ps7 IP core out to hw/, flat, so xsdb scripts (e.g.
# sw/run_gpio_test.tcl) have a stable path to source. These live in the
# .gen tree per IP core and are not otherwise exposed outside the XSA zip.
set ps7_gen_dir "$project_dir/arty_z7_soc.gen/sources_1/bd/design_1/ip/design_1_ps7_0"
foreach f {ps7_init.tcl ps7_init.c ps7_init.h ps7_init_gpl.c ps7_init_gpl.h} {
    set src [file join $ps7_gen_dir $f]
    if {[file exists $src]} {
        file copy -force $src [file join $project_dir $f]
    }
}

puts "Build complete: [file join $project_dir arty_z7_soc.bit]"
close_project
