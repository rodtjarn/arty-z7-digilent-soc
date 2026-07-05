# Program Arty Z7-20 via Vivado hardware manager

set bitfile [file normalize "../bin/top.bit"]

open_hw_manager
connect_hw_server -url localhost:3121
open_hw_target

# Find the Zynq FPGA device (not the ARM DAP)
set device ""
foreach dev [get_hw_devices] {
    if {[string match "*xc7z*" [get_property NAME $dev]]} {
        set device $dev
        break
    }
}
if {$device eq ""} {
    error "No Zynq FPGA device found"
}
current_hw_device $device
refresh_hw_device -update_hw_probes false $device

set_property PROGRAM.FILE $bitfile $device
program_hw_devices $device

puts "Programming complete"
close_hw_manager