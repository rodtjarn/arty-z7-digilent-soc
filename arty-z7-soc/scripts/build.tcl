# Build script for Arty Z7-10 SoC with PS + AXI GPIO

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

# Add all IP VHDL source files generated for this design
set gen_dir "$project_dir/arty_z7_soc.gen/sources_1/bd/design_1"
set vhd_files [glob -nocomplain $gen_dir/ip/*/synth/*.vhd]
set shared_vhd [glob -nocomplain $gen_dir/ipshared/*/hdl/*.vhd]
set all_vhd [concat $vhd_files $shared_vhd]
if {[llength $all_vhd] > 0} {
    eval add_files -norecurse $all_vhd
}

# Update compile order to resolve all IP dependencies
update_compile_order -fileset sources_1

# Add pin constraints
add_files -fileset constrs_1 [file join $src_dir top.xdc]

# Synthesize
synth_design -top design_1_wrapper -part xc7z010clg400-1
write_checkpoint -force [file join $project_dir post_synth.dcp]

# Implement
opt_design
place_design
write_checkpoint -force [file join $project_dir post_place.dcp]
route_design
write_checkpoint -force [file join $project_dir post_route.dcp]

# Generate bitstream
write_bitstream -force [file join $project_dir arty_z7_soc.bit]

# Export hardware (XSA) for Vitis/PetaLinux
write_hw_platform -fixed -include_bit -force -file [file join $project_dir arty_z7_soc.xsa]

puts "Build complete: [file join $project_dir arty_z7_soc.bit]"
close_project