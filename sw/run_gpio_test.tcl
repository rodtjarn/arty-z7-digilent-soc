# xsdb script: program SoC bitstream and run gpio_test on Arty Z7

set bit_file  [file normalize [file join [file dirname [info script]] \
    "../arty-z7-soc/hw/arty_z7_soc.bit"]]
set elf_file  [file normalize [file join [file dirname [info script]] \
    "gpio_test_low.elf"]]
set ps7_init  [file normalize [file join [file dirname [info script]] \
    "../arty-z7-soc/hw/ps7_init.tcl"]]

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

# RESULT_ADDR lives in OCM outside the downloaded ELF image, so it isn't reset by
# `dow` and still holds whatever a *previous* run left there. Clear it here, with
# the core halted, instead of relying on the target's own early clear in main() --
# otherwise the first poll below can race that clear and read a stale PASS/FAIL
# from the prior run, reporting success in milliseconds with no test actually run.
set result_addr 0x00001000
mwr $result_addr 0
puts "Cleared stale result sentinel at [format 0x%08X $result_addr]"

puts "Running... (LEDs blink 0xA<->0x5 for 10 iterations)"
con

# gpio_test.c writes a sentinel to RESULT_ADDR once it finishes: RESULT_PASS after
# 10 clean iterations, RESULT_FAIL immediately on any readback mismatch. Poll for it
# instead of blocking on con forever, so a wedged core times out instead of stalling.
set result_pass 0x600d9000
set result_fail 0xbad09000
set timeout_ms  30000
set poll_ms     200
set elapsed_ms  0
set result      ""

while {$elapsed_ms < $timeout_ms} {
    set value [mrd -value $result_addr]
    if {$value == $result_pass} {
        set result "PASS"
        break
    } elseif {$value == $result_fail} {
        set result "FAIL"
        break
    }
    after $poll_ms
    incr elapsed_ms $poll_ms
}

stop
puts "Core halted"

if {$result eq "PASS"} {
    puts "TEST PASSED"
} elseif {$result eq "FAIL"} {
    puts "TEST FAILED"
} else {
    puts "TEST FAILED (timed out waiting for result after ${timeout_ms}ms)"
}
