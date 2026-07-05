# Build script for Arty Z7-10 counter

set project_dir [file normalize "../bin"]
set src_dir    [file normalize "../src"]

# Create project targeting Arty Z7-10
create_project -force counter $project_dir -part xc7z010clg400-1
set_property board_part digilentinc.com:arty-z7-10:part0:1.1 [current_project]

# Add SystemVerilog sources
add_files -fileset sources_1 [glob -nocomplain [file join $src_dir *.sv]]

# Add constraints
add_files -fileset constrs_1 [file join $src_dir top.xdc]

# Set top module
set_property top top [current_fileset]

# Synthesize
synth_design -top top -part xc7z010clg400-1
write_checkpoint -force [file join $project_dir post_synth.dcp]

# Optimize, place, route
opt_design
place_design
write_checkpoint -force [file join $project_dir post_place.dcp]

route_design
write_checkpoint -force [file join $project_dir post_route.dcp]

# Generate bitstream
write_bitstream -force [file join $project_dir top.bit]

# Reports
report_timing -file [file join $project_dir timing.rpt]
report_utilization -file [file join $project_dir utilization.rpt]

puts "Build complete: [file join $project_dir top.bit]"
close_project