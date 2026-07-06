# xsdb script: program SoC bitstream and run gpio_test on Arty Z7

set bit_file  [file normalize [file join [file dirname [info script]] \
    "../arty-z7-soc/hw/arty_z7_soc.bit"]]
if {[info exists ::env(TEST_ELF)]} {
    set elf_file [file normalize [file join [file dirname [info script]] $::env(TEST_ELF)]]
} else {
    set elf_file [file normalize [file join [file dirname [info script]] \
        "gpio_test_low.elf"]]
}
set ps7_init  [file normalize [file join [file dirname [info script]] \
    "../arty-z7-soc/hw/ps7_init.tcl"]]
set stamp_file [file normalize [file join [file dirname [info script]] \
    ".program_stamp"]]

puts "Bitstream : $bit_file"
puts "ELF       : $elf_file"
puts "ps7_init  : $ps7_init"

# Bitstream flashing over JTAG takes several seconds; skip it when the FPGA is
# already running the exact bitstream we'd flash. A hash comparison alone can't
# tell (it doesn't know about power cycles or reprogramming by other tools), so
# it's paired with a live "fpga -state" query below for ground truth.
set current_hash [lindex [exec sha256sum $bit_file] 0]
set stamped_hash ""
if {[file exists $stamp_file]} {
    set fp [open $stamp_file r]
    set stamped_hash [string trim [read $fp]]
    close $fp
}

connect -host localhost -port 3121
puts "Connected to hw_server"

puts "\n--- Available targets ---"
targets
puts "-------------------------\n"

# Select ARM core first, before any other operations
targets -set -filter {name =~ "ARM Cortex-A9 MPCore #0"}
puts "Selected ARM Cortex-A9 MPCore #0"

# stop throws instead of no-op'ing if the core is already halted (e.g. left
# stopped by a prior run) -- either way the core ends up halted, so tolerate it.
if {[catch {stop} err]} {
    puts "Core already halted ($err)"
} else {
    puts "Core halted"
}

# Program PL bitstream, unless it's unchanged and the FPGA is already
# configured (live DONE-bit check, not just our stamp file -- catches power
# cycles / external reprogramming since the stamp was written).
targets -set -filter {name =~ "xc7z010"}
set fpga_state [fpga -state]
puts "FPGA state: $fpga_state"
if {$current_hash eq $stamped_hash && ![string match "*not configured*" $fpga_state]} {
    puts "Bitstream unchanged and FPGA already configured -- skipping PL programming"
} else {
    puts "Programming PL bitstream..."
    fpga $bit_file
    puts "Bitstream programmed"
    set fp [open $stamp_file w]
    puts -nonewline $fp $current_hash
    close $fp
}

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

# Set PC to _start, immediately after the 32-byte vector table.
set entry 0x00000020
set sp    0x0000F000
rwr cpsr 0x000001D3
rwr pc $entry
rwr sp $sp
puts [format "CPSR=0x000001D3  PC=0x%08X  SP=0x%08X" $entry $sp]

# RESULT_ADDR lives in OCM outside the downloaded ELF image, so it isn't reset by
# `dow` and still holds whatever a *previous* run left there. Clear it here, with
# the core halted, instead of relying on the target's own early clear in main() --
# otherwise the first poll below can race that clear and read a stale PASS/FAIL
# from the prior run, reporting success in milliseconds with no test actually run.
set result_addr   0x0000FFE0
set stage_addr    0x0000FFE4
set detail_addr   0x0000FFE8
set expected_addr 0x0000FFEC
set actual_addr   0x0000FFF0
mwr $result_addr 0
mwr $stage_addr 0
mwr $detail_addr 0
mwr $expected_addr 0
mwr $actual_addr 0
puts "Cleared stale result sentinel at [format 0x%08X $result_addr]"

puts "Running bare-metal diagnostic suite..."
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
puts "Stopped [string trim [rrd pc]]  [string trim [rrd cpsr]]"

if {$result eq "PASS"} {
    puts "TEST PASSED"
} elseif {$result eq "FAIL"} {
    set stage    [mrd -value $stage_addr]
    set detail   [mrd -value $detail_addr]
    set expected [mrd -value $expected_addr]
    set actual   [mrd -value $actual_addr]
    puts [format "TEST FAILED stage/code=0x%08X detail=0x%08X expected=0x%08X actual=0x%08X" $stage $detail $expected $actual]
} else {
    set stage [mrd -value $stage_addr]
    set detail [mrd -value $detail_addr]
    puts [format "TEST FAILED (timed out waiting for result after ${timeout_ms}ms, stage/code=0x%08X detail=0x%08X)" $stage $detail]
}
