/*
 * SPDX-License-Identifier: Apache-2.0
 * Board initialization for Raspberry Pi Pico 2 (RP2350)
 * Standalone implementation - no Pico SDK runtime dependencies
 */

#include "board.h"
#include <stdint.h>

/* RP2350 Register Base Addresses */
#define RESETS_BASE         0x40020000
#define IO_BANK0_BASE       0x40028000
#define PADS_BANK0_BASE     0x40038000
#define UART0_BASE          0x40070000
#define CLOCKS_BASE         0x40010000
#define XOSC_BASE           0x40048000
#define PLL_SYS_BASE        0x40050000
#define WATCHDOG_BASE       0x40058000

/* Register access macros */
#define REG32(addr)         (*(volatile uint32_t *)(addr))

/* RESETS registers */
#define RESETS_RESET        REG32(RESETS_BASE + 0x0)
#define RESETS_RESET_DONE   REG32(RESETS_BASE + 0x8)

/* Reset bits */
#define RESET_UART0         (1 << 26)
#define RESET_IO_BANK0      (1 << 6)
#define RESET_PADS_BANK0    (1 << 9)
#define RESET_PLL_SYS       (1 << 14)

/* XOSC registers */
#define XOSC_CTRL           REG32(XOSC_BASE + 0x00)
#define XOSC_STATUS         REG32(XOSC_BASE + 0x04)
#define XOSC_STARTUP        REG32(XOSC_BASE + 0x0C)

/* PLL registers */
#define PLL_SYS_CS          REG32(PLL_SYS_BASE + 0x00)
#define PLL_SYS_PWR         REG32(PLL_SYS_BASE + 0x04)
#define PLL_SYS_FBDIV       REG32(PLL_SYS_BASE + 0x08)
#define PLL_SYS_PRIM        REG32(PLL_SYS_BASE + 0x0C)

/* Clock registers */
#define CLK_REF_CTRL        REG32(CLOCKS_BASE + 0x30)
#define CLK_REF_DIV         REG32(CLOCKS_BASE + 0x34)
#define CLK_SYS_CTRL        REG32(CLOCKS_BASE + 0x3C)
#define CLK_SYS_DIV         REG32(CLOCKS_BASE + 0x40)
#define CLK_PERI_CTRL       REG32(CLOCKS_BASE + 0x48)

/* GPIO function select */
#define GPIO_FUNC_UART      2

/* UART registers */
#define UART0_DR            REG32(UART0_BASE + 0x00)
#define UART0_FR            REG32(UART0_BASE + 0x18)
#define UART0_IBRD          REG32(UART0_BASE + 0x24)
#define UART0_FBRD          REG32(UART0_BASE + 0x28)
#define UART0_LCR_H         REG32(UART0_BASE + 0x2C)
#define UART0_CR            REG32(UART0_BASE + 0x30)

/* UART flags */
#define UART_FR_TXFF        (1 << 5)  /* TX FIFO full */

/* System clock - will be updated after clock init */
uint32_t SystemCoreClock = 12000000;  /* Start with 12 MHz assumption */

static void busy_wait_cycles(uint32_t cycles)
{
    volatile uint32_t count = cycles;
    while (count--) {
        __asm volatile ("nop");
    }
}

static void reset_release(uint32_t bits)
{
    /* Clear reset bits */
    RESETS_RESET &= ~bits;
    
    /* Wait for reset done */
    while ((RESETS_RESET_DONE & bits) != bits) {
        /* spin */
    }
}

static void gpio_set_function(uint32_t gpio, uint32_t fn)
{
    /* Set GPIO function in IO_BANK0 */
    volatile uint32_t *ctrl = (volatile uint32_t *)(IO_BANK0_BASE + 0x04 + (gpio * 8));
    *ctrl = fn;
}

/* SIO for GPIO direct control */
#define SIO_BASE            0xD0000000
#define SIO_GPIO_OUT_SET    REG32(SIO_BASE + 0x018)
#define SIO_GPIO_OUT_CLR    REG32(SIO_BASE + 0x020)
#define SIO_GPIO_OE_SET     REG32(SIO_BASE + 0x038)
#define SIO_GPIO_OE_CLR     REG32(SIO_BASE + 0x040)

/* LED GPIO */
#define LED_PIN             25
#define GPIO_FUNC_SIO       5

void led_init(void)
{
    /* Set LED GPIO to SIO function */
    gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
    /* Enable output */
    SIO_GPIO_OE_SET = (1 << LED_PIN);
    /* Turn off initially */
    SIO_GPIO_OUT_CLR = (1 << LED_PIN);
}

void led_on(void)
{
    SIO_GPIO_OUT_SET = (1 << LED_PIN);
}

void led_off(void)
{
    SIO_GPIO_OUT_CLR = (1 << LED_PIN);
}

void led_toggle(void)
{
    static int state = 0;
    if (state) {
        led_off();
    } else {
        led_on();
    }
    state = !state;
}

void BOARD_InitBootPins(void)
{
    /* Release IO_BANK0 and PADS_BANK0 from reset */
    reset_release(RESET_IO_BANK0 | RESET_PADS_BANK0);
    
    /* Initialize LED first - visual feedback that we're running */
    led_init();
    led_on();  /* Turn LED on immediately to show we're alive */
    
    /* Set GPIO 0 and 1 to UART function */
    gpio_set_function(BOARD_DEBUG_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(BOARD_DEBUG_UART_RX_PIN, GPIO_FUNC_UART);
}

void BOARD_InitBootClocks(void)
{
    /*
     * Initialize clocks for 150 MHz operation.
     * 
     * RP2350 boot sequence:
     * 1. Enable XOSC (12 MHz crystal)
     * 2. Configure PLL_SYS: 12 MHz * 125 / 5 / 2 = 150 MHz
     * 3. Switch clk_sys to PLL_SYS
     * 4. Configure clk_peri from clk_sys
     */
    
    /* Enable XOSC */
    XOSC_CTRL = 0xAA0;  /* ENABLE = ENABLE, FREQ_RANGE = 1_15MHZ */
    XOSC_STARTUP = 47;  /* ~1ms startup delay at ring osc frequency */
    
    /* Wait for XOSC to stabilize */
    while (!(XOSC_STATUS & (1 << 31))) {  /* STABLE bit */
        /* spin */
    }
    
    /* Switch clk_ref to XOSC (source = 2) */
    CLK_REF_CTRL = 2;  /* XOSC */
    CLK_REF_DIV = (1 << 8);  /* Divide by 1 */
    
    /* Configure PLL_SYS for 150 MHz */
    /* First put PLL into reset */
    RESETS_RESET |= RESET_PLL_SYS;
    busy_wait_cycles(100);
    RESETS_RESET &= ~RESET_PLL_SYS;
    while (!(RESETS_RESET_DONE & RESET_PLL_SYS)) {
        /* spin */
    }
    
    /* Configure PLL: VCO = 12 MHz * 125 = 1500 MHz */
    PLL_SYS_FBDIV = 125;
    
    /* Power up PLL VCO and wait for lock */
    PLL_SYS_PWR &= ~((1 << 5) | (1 << 0));  /* Clear VCOPD, PD */
    while (!(PLL_SYS_CS & (1 << 31))) {  /* Wait for LOCK */
        /* spin */
    }
    
    /* Configure post dividers: /5 /2 = 150 MHz */
    PLL_SYS_PRIM = (5 << 16) | (2 << 12);  /* POSTDIV1=5, POSTDIV2=2 */
    
    /* Power up post dividers */
    PLL_SYS_PWR &= ~(1 << 3);  /* Clear POSTDIVPD */
    
    /* Switch clk_sys to PLL_SYS */
    /* First set AUX source to PLL_SYS (0) */
    CLK_SYS_CTRL = (CLK_SYS_CTRL & ~(0x7 << 5)) | (0 << 5);
    /* Then switch to AUX source */
    CLK_SYS_CTRL = (CLK_SYS_CTRL & ~0x3) | 0x1;
    CLK_SYS_DIV = (1 << 8);  /* Divide by 1 */
    
    /* Configure clk_peri to run from clk_sys */
    CLK_PERI_CTRL = (1 << 11) | (0 << 5);  /* ENABLE, SRC=clk_sys */
    
    /* Update SystemCoreClock */
    SystemCoreClock = 150000000;
}

void BOARD_InitDebugConsole(void)
{
    /* Release UART0 from reset */
    reset_release(RESET_UART0);
    
    /*
     * Configure UART0 for 115200 baud
     * 
     * Baud rate = UARTCLK / (16 * divisor)
     * At 150 MHz: divisor = 150000000 / (16 * 115200) = 81.380
     * At 12 MHz:  divisor = 12000000 / (16 * 115200) = 6.510
     * 
     * IBRD = integer part, FBRD = fractional part * 64
     */
    uint32_t baud_rate = 115200;
    uint32_t baud_div = (8 * SystemCoreClock / baud_rate);
    uint32_t ibrd = baud_div >> 7;
    uint32_t fbrd = ((baud_div & 0x7F) + 1) / 2;
    
    if (ibrd == 0) {
        ibrd = 1;
        fbrd = 0;
    } else if (ibrd >= 65535) {
        ibrd = 65535;
        fbrd = 0;
    }
    
    UART0_IBRD = ibrd;
    UART0_FBRD = fbrd;
    
    /* 8 data bits, no parity, 1 stop bit, enable FIFOs */
    UART0_LCR_H = (3 << 5) | (1 << 4);  /* WLEN=8, FEN=1 */
    
    /* Enable UART, TX, and RX */
    UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);  /* UARTEN, TXE, RXE */
}

/* Low-level character output for printf */
void uart_putc(char c)
{
    /* Wait while TX FIFO is full */
    while (UART0_FR & UART_FR_TXFF) {
        /* spin */
    }
    UART0_DR = c;
}

/* Override _write for newlib printf support */
int _write(int file, char *ptr, int len)
{
    (void)file;
    
    for (int i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            uart_putc('\r');
        }
        uart_putc(ptr[i]);
    }
    return len;
}

/* Stub for _read (not used) */
int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/* Stub for _close */
int _close(int file)
{
    (void)file;
    return -1;
}

/* Stub for _lseek */
int _lseek(int file, int offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    return 0;
}

/* Stub for _fstat */
int _fstat(int file, void *st)
{
    (void)file;
    (void)st;
    return 0;
}

/* Stub for _isatty */
int _isatty(int file)
{
    (void)file;
    return 1;
}

/* Stub for _sbrk (heap allocation) */
void *_sbrk(int incr)
{
    extern char __end__;
    extern char __HeapLimit;
    static char *heap_end = 0;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &__end__;
    }
    prev_heap_end = heap_end;

    if (heap_end + incr > &__HeapLimit) {
        return (void *)-1;
    }

    heap_end += incr;
    return prev_heap_end;
}
