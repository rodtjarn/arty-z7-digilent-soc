# xsdb script: program SoC bitstream and run gpio_test on Arty Z7

set bit_file  [file normalize [file join [file dirname [info script]] \
    "../arty-z7-soc/hw/arty_z7_soc.bit"]]
set elf_file  [file normalize [file join [file dirname [info script]] \
    "gpio_test_low.elf"]]
set ps7_init  "/tmp/ps7_init_data/ps7_init.tcl"

puts "Bitstream : $bit_file"
puts "ELF       : $elf_file"
puts "ps7_init  : $ps7_init"

connect -host localhost -port 3121
puts "Connected to hw_server"

puts "\n--- Available targets ---"
targets
puts "-------------------------\n"

# Select ARM core first, before any other operations
targets -set -filter {name =~ "ARM Cortex-A9 MPCore #0"}
puts "Selected ARM Cortex-A9 MPCore #0"

stop
puts "Core halted"

# Program PL bitstream
targets -set -filter {name =~ "xc7z010"}
puts "Programming PL bitstream..."
fpga $bit_file
puts "Bitstream programmed"

# Back to ARM core
targets -set -filter {name =~ "ARM Cortex-A9 MPCore #0"}
puts "Back to ARM core"

# Download ELF BEFORE ps7_init while DAP is still clean
# (0x00000000 = OCM low alias in JTAG boot mode)
puts "Downloading ELF before ps7_init (OCM low alias at 0x00000000)..."
dow $elf_file
puts "ELF downloaded"

# Initialize PS
source $ps7_init
puts "Calling ps7_init (initializes MIO, PLLs, clocks, DDR)..."
ps7_init
puts "ps7_init done"
ps7_post_config
puts "ps7_post_config done (PS-PL level shifters enabled)"

# Set PC to entry point and SP to top of OCM
set entry 0x00000020
set sp    0x0000FFFC
rwr pc $entry
rwr sp $sp
puts [format "PC=0x%08X  SP=0x%08X" $entry $sp]

puts "Running... (LEDs should blink 0xA<->0x5 if PASS)"
con
