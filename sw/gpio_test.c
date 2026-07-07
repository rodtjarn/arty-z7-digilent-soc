#define LED_GPIO_BASE     0x41200000u
#define BTN_GPIO_BASE     0x41210000u
#define GPIO_DATA         0x00u
#define GPIO_TRI          0x04u

#define AXI_TIMER_BASE    0x42800000u
#define AXI_TIMER_TCSR0   0x00u
#define AXI_TIMER_TLR0    0x04u
#define AXI_TIMER_TCR0    0x08u
#define AXI_TIMER_CSR_LOAD 0x20u
#define AXI_TIMER_CSR_ENT 0x80u

#define CUSTOM_AXI_BASE   0x43C00000u
#define CUSTOM_AXI_ID     0x00u
#define CUSTOM_AXI_SCRATCH 0x04u
#define CUSTOM_AXI_COUNTER 0x08u
#define CUSTOM_AXI_STATUS 0x0Cu
#define CUSTOM_AXI_ID_VALUE 0xA7100007u
#define CUSTOM_AXI_STATUS_XOR 0x5A5AA5A5u

#define AXI_BRAM_BASE     0x42000000u
#define AXI_BRAM_WORDS    4096u

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

#define GIC_CPU_BASE      0xF8F00100u
#define GIC_CPU_CTRL      0x00u
#define GIC_CPU_PRIMASK   0x04u
#define GIC_CPU_IAR       0x0Cu
#define GIC_CPU_EOIR      0x10u

#define GIC_DIST_BASE     0xF8F01000u
#define GIC_DIST_CTRL     0x000u
#define GIC_DIST_SET_EN   0x100u
#define GIC_DIST_CLR_EN   0x180u
#define GIC_DIST_PRIORITY 0x400u

#define PRIV_TIMER_BASE   0xF8F00600u
#define PRIV_TIMER_LOAD   0x00u
#define PRIV_TIMER_COUNT  0x04u
#define PRIV_TIMER_CTRL   0x08u
#define PRIV_TIMER_ISR    0x0Cu

#define PRIV_TIMER_IRQ_ID 29u
#define PRIV_TIMER_CTRL_ENABLE 0x1u
#define PRIV_TIMER_CTRL_AUTO_RELOAD 0x2u
#define PRIV_TIMER_CTRL_IRQ_ENABLE 0x4u

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
#define TEST_MODE_GIC     6
#define TEST_MODE_AXI_TIMER 7
#define TEST_MODE_CUSTOM_AXI 8
#define TEST_MODE_AXI_BRAM 9

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
#define STAGE_GIC         0x00000008u
#define STAGE_AXI_TIMER   0x00000009u
#define STAGE_CUSTOM_AXI  0x0000000Au
#define STAGE_AXI_BRAM    0x0000000Bu

#define FAIL_AXI_DATA     0x00000102u
#define FAIL_AXI_TIMER_STUCK 0x00000202u
#define FAIL_CUSTOM_AXI_ID 0x00000210u
#define FAIL_CUSTOM_AXI_SCRATCH 0x00000211u
#define FAIL_CUSTOM_AXI_COUNTER 0x00000212u
#define FAIL_CUSTOM_AXI_STATUS 0x00000213u
#define FAIL_AXI_BRAM_PATTERN 0x00000220u
#define FAIL_TIMER_STUCK  0x00000301u
#define FAIL_DDR_PATTERN  0x00000401u
#define FAIL_GIC_NO_IRQ   0x00000501u
#define FAIL_GIC_BAD_IRQ  0x00000502u

#define REG32(addr) (*(volatile unsigned int *)(addr))
#define REG8(addr)  (*(volatile unsigned char *)(addr))

typedef unsigned int u32;

static u32 led_ready;
static volatile u32 gic_irq_count;
static volatile u32 gic_last_irq_id;
static volatile u32 gic_unexpected_irq_id;

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

static void enable_irq(void) {
    __asm__ volatile("cpsie i" ::: "memory");
}

static void disable_irq(void) {
    __asm__ volatile("cpsid i" ::: "memory");
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

void irq_handler(void) {
    u32 iar = REG32(GIC_CPU_BASE + GIC_CPU_IAR);
    u32 irq_id = iar & 0x3FFu;

    gic_last_irq_id = irq_id;
    if (irq_id == PRIV_TIMER_IRQ_ID) {
        REG32(PRIV_TIMER_BASE + PRIV_TIMER_ISR) = 1u;
        gic_irq_count++;
    } else if (irq_id < 1020u) {
        gic_unexpected_irq_id = irq_id;
    }

    REG32(GIC_CPU_BASE + GIC_CPU_EOIR) = iar;
}

static void test_gic_private_timer(void) {
    const u32 irq_bit = 1u << PRIV_TIMER_IRQ_ID;

    stage(STAGE_GIC, "gic private timer irq");
    disable_irq();

    gic_irq_count = 0u;
    gic_last_irq_id = 0xFFFFFFFFu;
    gic_unexpected_irq_id = 0xFFFFFFFFu;

    REG32(PRIV_TIMER_BASE + PRIV_TIMER_CTRL) = 0u;
    REG32(PRIV_TIMER_BASE + PRIV_TIMER_ISR) = 1u;

    REG32(GIC_DIST_BASE + GIC_DIST_CTRL) = 0u;
    REG32(GIC_DIST_BASE + GIC_DIST_CLR_EN) = irq_bit;
    REG8(GIC_DIST_BASE + GIC_DIST_PRIORITY + PRIV_TIMER_IRQ_ID) = 0xA0u;
    REG32(GIC_DIST_BASE + GIC_DIST_SET_EN) = irq_bit;
    REG32(GIC_CPU_BASE + GIC_CPU_PRIMASK) = 0xFFu;
    REG32(GIC_CPU_BASE + GIC_CPU_CTRL) = 1u;
    REG32(GIC_DIST_BASE + GIC_DIST_CTRL) = 1u;

    REG32(PRIV_TIMER_BASE + PRIV_TIMER_LOAD) = 100000u;
    REG32(PRIV_TIMER_BASE + PRIV_TIMER_CTRL) =
        PRIV_TIMER_CTRL_ENABLE | PRIV_TIMER_CTRL_AUTO_RELOAD |
        PRIV_TIMER_CTRL_IRQ_ENABLE;

    enable_irq();
    for (u32 timeout = 0u; timeout < 20000000u && gic_irq_count < 3u; timeout++) {
        __asm__ volatile("nop");
    }
    disable_irq();

    REG32(PRIV_TIMER_BASE + PRIV_TIMER_CTRL) = 0u;
    REG32(PRIV_TIMER_BASE + PRIV_TIMER_ISR) = 1u;
    REG32(GIC_DIST_BASE + GIC_DIST_CLR_EN) = irq_bit;

    uart_puts("gic irq_count=");
    uart_hex32(gic_irq_count);
    uart_puts(" last_irq=");
    uart_hex32(gic_last_irq_id);
    uart_puts("\n");

    if (gic_unexpected_irq_id != 0xFFFFFFFFu) {
        fail(FAIL_GIC_BAD_IRQ, PRIV_TIMER_IRQ_ID, gic_unexpected_irq_id);
    }
    if (gic_irq_count < 3u) {
        fail(FAIL_GIC_NO_IRQ, 3u, gic_irq_count);
    }
}

static void test_axi_timer(void) {
    u32 start;
    u32 end;

    stage(STAGE_AXI_TIMER, "axi timer polled count");

    REG32(AXI_TIMER_BASE + AXI_TIMER_TCSR0) = 0u;
    REG32(AXI_TIMER_BASE + AXI_TIMER_TLR0) = 0u;
    REG32(AXI_TIMER_BASE + AXI_TIMER_TCSR0) = AXI_TIMER_CSR_LOAD;
    REG32(AXI_TIMER_BASE + AXI_TIMER_TCSR0) = 0u;

    start = REG32(AXI_TIMER_BASE + AXI_TIMER_TCR0);
    REG32(AXI_TIMER_BASE + AXI_TIMER_TCSR0) = AXI_TIMER_CSR_ENT;
    delay(1000000u);
    end = REG32(AXI_TIMER_BASE + AXI_TIMER_TCR0);
    REG32(AXI_TIMER_BASE + AXI_TIMER_TCSR0) = 0u;

    uart_puts("axi_timer start=");
    uart_hex32(start);
    uart_puts(" end=");
    uart_hex32(end);
    uart_puts("\n");

    if (end <= start || (end - start) < 100u) {
        fail(FAIL_AXI_TIMER_STUCK, start, end);
    }
}

static void test_custom_axi(void) {
    static const u32 patterns[] = {
        0x00000000u, 0xFFFFFFFFu, 0x12345678u, 0xA5A55A5Au
    };
    u32 id;
    u32 start;
    u32 end;
    u32 status;

    stage(STAGE_CUSTOM_AXI, "custom axi-lite registers");

    id = REG32(CUSTOM_AXI_BASE + CUSTOM_AXI_ID);
    uart_puts("custom_axi id=");
    uart_hex32(id);
    uart_puts("\n");
    if (id != CUSTOM_AXI_ID_VALUE) {
        fail(FAIL_CUSTOM_AXI_ID, CUSTOM_AXI_ID_VALUE, id);
    }

    for (u32 i = 0u; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        REG32(CUSTOM_AXI_BASE + CUSTOM_AXI_SCRATCH) = patterns[i];
        if (REG32(CUSTOM_AXI_BASE + CUSTOM_AXI_SCRATCH) != patterns[i]) {
            fail(FAIL_CUSTOM_AXI_SCRATCH, patterns[i],
                 REG32(CUSTOM_AXI_BASE + CUSTOM_AXI_SCRATCH));
        }
    }

    start = REG32(CUSTOM_AXI_BASE + CUSTOM_AXI_COUNTER);
    delay(1000000u);
    end = REG32(CUSTOM_AXI_BASE + CUSTOM_AXI_COUNTER);
    uart_puts("custom_axi counter_start=");
    uart_hex32(start);
    uart_puts(" counter_end=");
    uart_hex32(end);
    uart_puts("\n");
    if (end == start || (end - start) < 100u) {
        fail(FAIL_CUSTOM_AXI_COUNTER, start, end);
    }

    status = REG32(CUSTOM_AXI_BASE + CUSTOM_AXI_STATUS);
    uart_puts("custom_axi scratch=");
    uart_hex32(patterns[3]);
    uart_puts(" status=");
    uart_hex32(status);
    uart_puts("\n");
    if (status != (patterns[3] ^ CUSTOM_AXI_STATUS_XOR)) {
        fail(FAIL_CUSTOM_AXI_STATUS, patterns[3] ^ CUSTOM_AXI_STATUS_XOR,
             status);
    }
}

static u32 axi_bram_pattern(u32 index, u32 pass_index) {
    switch (pass_index) {
    case 0u:
        return 0x00000000u;
    case 1u:
        return 0xFFFFFFFFu;
    case 2u:
        return 0x13570000u ^ (index * 0x00010001u);
    default:
        return ~(0x24680000u ^ (index * 0x00010001u));
    }
}

static void test_axi_bram(void) {
    volatile u32 *mem = (volatile u32 *)AXI_BRAM_BASE;

    stage(STAGE_AXI_BRAM, "axi bram memory patterns");

    for (u32 pass_index = 0u; pass_index < 4u; pass_index++) {
        for (u32 i = 0u; i < AXI_BRAM_WORDS; i++) {
            mem[i] = axi_bram_pattern(i, pass_index);
        }

        for (u32 i = 0u; i < AXI_BRAM_WORDS; i++) {
            u32 expected = axi_bram_pattern(i, pass_index);
            u32 actual = mem[i];
            if (actual != expected) {
                fail_detail(FAIL_AXI_BRAM_PATTERN,
                            AXI_BRAM_BASE + (i * 4u), expected, actual);
            }
        }

        uart_puts("axi_bram pass ");
        uart_hex32(pass_index);
        uart_puts(" ok\n");
    }
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
#elif TEST_MODE == TEST_MODE_GIC
    stage(STAGE_GIC, "gic-only mode");
    test_gic_private_timer();
    pass();
#elif TEST_MODE == TEST_MODE_AXI_TIMER
    stage(STAGE_AXI_TIMER, "axi-timer-only mode");
    test_axi_timer();
    pass();
#elif TEST_MODE == TEST_MODE_CUSTOM_AXI
    stage(STAGE_CUSTOM_AXI, "custom-axi-only mode");
    test_custom_axi();
    pass();
#elif TEST_MODE == TEST_MODE_AXI_BRAM
    stage(STAGE_AXI_BRAM, "axi-bram-only mode");
    test_axi_bram();
    pass();
#else
    init_pl_gpio();
    stage(STAGE_BOOT, "boot");
    test_axi_gpio();
    test_buttons();
    test_global_timer();
    test_gic_private_timer();
    test_axi_timer();
    test_custom_axi();
    test_axi_bram();
    test_ddr();
    pass();
#endif

    return 0;
}
