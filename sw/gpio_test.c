#define LED_GPIO_BASE 0x41200000  // AXI GPIO instance driving the LEDs
#define BTN_GPIO_BASE 0x41210000  // AXI GPIO instance reading the buttons
#define GPIO_DATA     0x00        // data register offset
#define GPIO_TRI      0x04        // tri-state (direction) register offset: 0 = output, 1 = input
#define SLCR_UNLOCK   0xF8000008  // write unlock key here to allow SLCR register writes
#define SLCR_LOCK     0xF8000004  // write lock key here to re-lock SLCR registers
#define FPGA0_CLK_CTRL 0xF8000170 // PL clock 0 (FCLK0) source/divider control

#define LED_MASK 0xF
#define BTN_MASK 0xF

#define REG32(addr) (*(volatile unsigned int *)(addr))

#define TEST_ITERATIONS 10

// Fixed OCM word (well above .text/.data/.bss, well below the stack at 0x0000FFFC)
// where run_gpio_test.tcl polls for the pass/fail sentinel after starting execution.
#define RESULT_ADDR 0x00001000
#define RESULT_PASS 0x600D9000
#define RESULT_FAIL 0xBAD09000

// Unlock SLCR, enable/configure FCLK0 (clock feeding the PL/AXI GPIO), then re-lock SLCR
static void enable_fclk(void) {
    REG32(SLCR_UNLOCK) = 0xDF0D;
    REG32(FPGA0_CLK_CTRL) = 0x00400801;
    REG32(SLCR_LOCK) = 0x767B;
}

// Busy-wait loop; volatile count prevents the compiler from optimizing it away
void delay(volatile int count) {
    while (count--);
}

// Park the core once the result sentinel has been written; wfi (not a busy loop)
// so the CPU is idle rather than spinning while the harness reads the result.
static void halt(void) {
    while (1) {
        __asm__ volatile("wfi");
    }
}

int main(void) {
    enable_fclk();                                    // start the PL clock so AXI GPIO is reachable
    REG32(LED_GPIO_BASE + GPIO_TRI) = 0;               // LEDs: all bits output
    REG32(BTN_GPIO_BASE + GPIO_TRI) = BTN_MASK;        // buttons: all bits input
    REG32(RESULT_ADDR) = 0;                            // clear any stale sentinel before the harness starts polling

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        // drive LED pattern 1010, then read back to confirm the write took effect
        REG32(LED_GPIO_BASE + GPIO_DATA) = 0xA;
        if ((REG32(LED_GPIO_BASE + GPIO_DATA) & LED_MASK) != 0xA) {
            REG32(LED_GPIO_BASE + GPIO_DATA) = 0x1;    // readback mismatch: signal failure
            REG32(RESULT_ADDR) = RESULT_FAIL;
            halt();
        }
        delay(25000000);

        // drive LED pattern 0101, then read back to confirm the write took effect
        REG32(LED_GPIO_BASE + GPIO_DATA) = 0x5;
        if ((REG32(LED_GPIO_BASE + GPIO_DATA) & LED_MASK) != 0x5) {
            REG32(LED_GPIO_BASE + GPIO_DATA) = 0x2;    // readback mismatch: signal failure
            REG32(RESULT_ADDR) = RESULT_FAIL;
            halt();
        }
        delay(25000000);

        (void)REG32(BTN_GPIO_BASE + GPIO_DATA);        // touch the button register (result unused)
    }

    REG32(RESULT_ADDR) = RESULT_PASS;                  // completed all iterations without a mismatch
    halt();
}
