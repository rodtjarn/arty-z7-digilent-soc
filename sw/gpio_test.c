#define LED_GPIO_BASE     0x41200000u
#define BTN_GPIO_BASE     0x41210000u
#define GPIO_DATA         0x00u
#define GPIO_TRI          0x04u

#define UART0_BASE        0xE0000000u
#define UART_CR           0x00u
#define UART_SR           0x2Cu
#define UART_FIFO         0x30u
#define UART_CR_TX_EN     0x10u
#define UART_CR_RX_EN     0x04u
#define UART_SR_TXFULL    0x10u

#define SLCR_UNLOCK       0xF8000008u
#define SLCR_LOCK         0xF8000004u
#define FPGA0_CLK_CTRL    0xF8000170u

#define GLOBAL_TIMER_BASE 0xF8F00200u
#define GT_COUNTER_LOW    0x00u
#define GT_COUNTER_HIGH   0x04u
#define GT_CONTROL        0x08u

#define DDR_TEST_BASE     0x00100000u
#define DDR_TEST_WORDS    16384u

#define LED_MASK          0xFu
#define BTN_MASK          0xFu

#define TEST_MODE_FULL    0
#define TEST_MODE_UART    1
#define TEST_MODE_DDR     2
#define TEST_MODE_BUTTONS 3
#define TEST_MODE_TIMER   4
#define TEST_MODE_GPIO    5

#ifndef TEST_MODE
#define TEST_MODE TEST_MODE_FULL
#endif

/*
 * Top 32 bytes of the 64 KiB low OCM test window are reserved for xsdb status.
 * The linker keeps code/data/stack below 0x0000F000.
 */
#define RESULT_ADDR       0x0000FFE0u
#define RESULT_STAGE_ADDR 0x0000FFE4u
#define RESULT_DETAIL_ADDR 0x0000FFE8u
#define RESULT_EXPECT_ADDR 0x0000FFECu
#define RESULT_ACTUAL_ADDR 0x0000FFF0u
#define RESULT_PASS       0x600D9000u
#define RESULT_FAIL       0xBAD09000u

#define STAGE_BOOT        0x00000001u
#define STAGE_AXI_GPIO    0x00000002u
#define STAGE_BUTTONS     0x00000003u
#define STAGE_TIMER       0x00000004u
#define STAGE_DDR         0x00000005u
#define STAGE_DONE        0x00000006u
#define STAGE_UART        0x00000007u

#define FAIL_AXI_DATA     0x00000102u
#define FAIL_TIMER_STUCK  0x00000301u
#define FAIL_DDR_PATTERN  0x00000401u

#define REG32(addr) (*(volatile unsigned int *)(addr))

typedef unsigned int u32;

static u32 led_ready;

static void delay(volatile u32 count) {
    while (count--) {
        __asm__ volatile("nop");
    }
}

static void halt(void) {
    while (1) {
        __asm__ volatile("wfi");
    }
}

static void led_write(u32 value) {
    REG32(LED_GPIO_BASE + GPIO_DATA) = value & LED_MASK;
}

static void uart_enable(void) {
    REG32(UART0_BASE + UART_CR) = UART_CR_TX_EN | UART_CR_RX_EN;
}

static void uart_putc(char c) {
    if (c == '\n') {
        uart_putc('\r');
    }

    for (u32 timeout = 1000000u; timeout > 0u; timeout--) {
        if ((REG32(UART0_BASE + UART_SR) & UART_SR_TXFULL) == 0u) {
            REG32(UART0_BASE + UART_FIFO) = (u32)c;
            return;
        }
    }
}

static void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

static void uart_hex32(u32 value) {
    static const char hex[] = "0123456789ABCDEF";

    uart_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        uart_putc(hex[(value >> (u32)shift) & 0xFu]);
    }
}

static void status_clear(void) {
    REG32(RESULT_ADDR) = 0u;
    REG32(RESULT_STAGE_ADDR) = 0u;
    REG32(RESULT_DETAIL_ADDR) = 0u;
    REG32(RESULT_EXPECT_ADDR) = 0u;
    REG32(RESULT_ACTUAL_ADDR) = 0u;
}

static void stage(u32 code, const char *name) {
    REG32(RESULT_STAGE_ADDR) = code;
    if (led_ready) {
        led_write(code);
    }
    uart_puts("[stage] ");
    uart_puts(name);
    uart_puts("\n");
}

static void pass(void) {
    stage(STAGE_DONE, "done");
    if (led_ready) {
        led_write(0xFu);
    }
    uart_puts("TEST PASSED\n");
    REG32(RESULT_ADDR) = RESULT_PASS;
    halt();
}

static void fail_detail(u32 code, u32 detail, u32 expected, u32 actual) {
    REG32(RESULT_STAGE_ADDR) = code;
    REG32(RESULT_DETAIL_ADDR) = detail;
    REG32(RESULT_EXPECT_ADDR) = expected;
    REG32(RESULT_ACTUAL_ADDR) = actual;
    if (led_ready) {
        led_write(code);
    }

    uart_puts("TEST FAILED code=");
    uart_hex32(code);
    uart_puts(" detail=");
    uart_hex32(detail);
    uart_puts(" expected=");
    uart_hex32(expected);
    uart_puts(" actual=");
    uart_hex32(actual);
    uart_puts("\n");

    REG32(RESULT_ADDR) = RESULT_FAIL;
    halt();
}

static void fail(u32 code, u32 expected, u32 actual) {
    fail_detail(code, 0u, expected, actual);
}

static void enable_fclk(void) {
    REG32(SLCR_UNLOCK) = 0xDF0Du;
    REG32(FPGA0_CLK_CTRL) = 0x00400801u;
    REG32(SLCR_LOCK) = 0x767Bu;
}

static void init_pl_gpio(void) {
    enable_fclk();
    REG32(LED_GPIO_BASE + GPIO_TRI) = 0u;
    REG32(BTN_GPIO_BASE + GPIO_TRI) = BTN_MASK;
    led_ready = 1u;
}

static void test_axi_gpio(void) {
    static const u32 patterns[] = {
        0x0u, 0xFu, 0xAu, 0x5u, 0x3u, 0xCu, 0x9u, 0x6u
    };

    stage(STAGE_AXI_GPIO, "axi gpio led readback");

    REG32(LED_GPIO_BASE + GPIO_TRI) = 0u;
    for (u32 i = 0u; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        led_write(patterns[i]);
        delay(3000000u);
        if ((REG32(LED_GPIO_BASE + GPIO_DATA) & LED_MASK) != patterns[i]) {
            fail(FAIL_AXI_DATA, patterns[i],
                 REG32(LED_GPIO_BASE + GPIO_DATA) & LED_MASK);
        }
    }
}

static void test_buttons(void) {
    u32 observed_high = 0u;
    u32 observed_low = 0u;

    stage(STAGE_BUTTONS, "button input sampling");

    REG32(BTN_GPIO_BASE + GPIO_TRI) = BTN_MASK;
    uart_puts("Press buttons now to include them in observed mask.\n");
    for (u32 i = 0u; i < 120u; i++) {
        u32 sample = REG32(BTN_GPIO_BASE + GPIO_DATA) & BTN_MASK;
        observed_high |= sample;
        observed_low |= (~sample) & BTN_MASK;
        led_write(sample);
        delay(1500000u);
    }

    uart_puts("button observed_high=");
    uart_hex32(observed_high);
    uart_puts(" observed_low=");
    uart_hex32(observed_low);
    uart_puts("\n");
}

static void test_global_timer(void) {
    u32 start;
    u32 mid;
    u32 end;

    stage(STAGE_TIMER, "arm global timer");

    REG32(GLOBAL_TIMER_BASE + GT_CONTROL) = 0u;
    REG32(GLOBAL_TIMER_BASE + GT_COUNTER_LOW) = 0u;
    REG32(GLOBAL_TIMER_BASE + GT_COUNTER_HIGH) = 0u;
    REG32(GLOBAL_TIMER_BASE + GT_CONTROL) = 1u;

    start = REG32(GLOBAL_TIMER_BASE + GT_COUNTER_LOW);
    delay(10000u);
    mid = REG32(GLOBAL_TIMER_BASE + GT_COUNTER_LOW);
    delay(1000000u);
    end = REG32(GLOBAL_TIMER_BASE + GT_COUNTER_LOW);

    if (mid == start || end == mid || ((end - start) < 100u)) {
        fail(FAIL_TIMER_STUCK, start, end);
    }

    uart_puts("timer start=");
    uart_hex32(start);
    uart_puts(" end=");
    uart_hex32(end);
    uart_puts("\n");
}

static u32 ddr_pattern(u32 index, u32 pass_index) {
    switch (pass_index) {
    case 0u:
        return 0x00000000u;
    case 1u:
        return 0xFFFFFFFFu;
    case 2u:
        return 0xA5A50000u ^ (index * 0x00010001u);
    default:
        return ~(0x5A5A0000u ^ (index * 0x00010001u));
    }
}

static void test_ddr(void) {
    volatile u32 *mem = (volatile u32 *)DDR_TEST_BASE;

    stage(STAGE_DDR, "ddr memory patterns");

    for (u32 pass_index = 0u; pass_index < 4u; pass_index++) {
        for (u32 i = 0u; i < DDR_TEST_WORDS; i++) {
            mem[i] = ddr_pattern(i, pass_index);
        }

        for (u32 i = 0u; i < DDR_TEST_WORDS; i++) {
            u32 expected = ddr_pattern(i, pass_index);
            u32 actual = mem[i];
            if (actual != expected) {
                fail_detail(FAIL_DDR_PATTERN, DDR_TEST_BASE + (i * 4u),
                            expected, actual);
            }
        }

        uart_puts("ddr pass ");
        uart_hex32(pass_index);
        uart_puts(" ok\n");
    }
}

int main(void) {
    status_clear();
    REG32(RESULT_STAGE_ADDR) = STAGE_BOOT;
    uart_enable();

    uart_puts("\nArty Z7 bare-metal diagnostic\n");

#if TEST_MODE == TEST_MODE_UART
    stage(STAGE_UART, "uart smoke");
    uart_puts("UART0 transmit path is readable at 115200 baud.\n");
    pass();
#elif TEST_MODE == TEST_MODE_DDR
    stage(STAGE_DDR, "ddr-only mode");
    test_ddr();
    pass();
#elif TEST_MODE == TEST_MODE_GPIO
    init_pl_gpio();
    stage(STAGE_AXI_GPIO, "gpio-only mode");
    test_axi_gpio();
    pass();
#elif TEST_MODE == TEST_MODE_BUTTONS
    init_pl_gpio();
    stage(STAGE_BUTTONS, "buttons-only mode");
    test_buttons();
    pass();
#elif TEST_MODE == TEST_MODE_TIMER
    stage(STAGE_TIMER, "timer-only mode");
    test_global_timer();
    pass();
#else
    init_pl_gpio();
    stage(STAGE_BOOT, "boot");
    test_axi_gpio();
    test_buttons();
    test_global_timer();
    test_ddr();
    pass();
#endif

    return 0;
}
