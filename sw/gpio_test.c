#define LED_GPIO_BASE 0x41200000
#define BTN_GPIO_BASE 0x41210000
#define GPIO_DATA     0x00
#define GPIO_TRI      0x04
#define SLCR_UNLOCK   0xF8000008
#define SLCR_LOCK     0xF8000004
#define FPGA0_CLK_CTRL 0xF8000170

#define LED_MASK 0xF
#define BTN_MASK 0xF

#define REG32(addr) (*(volatile unsigned int *)(addr))

static void enable_fclk(void) {
    REG32(SLCR_UNLOCK) = 0xDF0D;
    REG32(FPGA0_CLK_CTRL) = 0x00400801;
    REG32(SLCR_LOCK) = 0x767B;
}

void delay(volatile int count) {
    while (count--);
}

int main(void) {
    enable_fclk();
    REG32(LED_GPIO_BASE + GPIO_TRI) = 0;
    REG32(BTN_GPIO_BASE + GPIO_TRI) = BTN_MASK;

    while (1) {
        REG32(LED_GPIO_BASE + GPIO_DATA) = 0xA;
        if ((REG32(LED_GPIO_BASE + GPIO_DATA) & LED_MASK) != 0xA) {
            REG32(LED_GPIO_BASE + GPIO_DATA) = 0x1;
            while (1);
        }
        delay(25000000);

        REG32(LED_GPIO_BASE + GPIO_DATA) = 0x5;
        if ((REG32(LED_GPIO_BASE + GPIO_DATA) & LED_MASK) != 0x5) {
            REG32(LED_GPIO_BASE + GPIO_DATA) = 0x2;
            while (1);
        }
        delay(25000000);

        (void)REG32(BTN_GPIO_BASE + GPIO_DATA);
    }
}
