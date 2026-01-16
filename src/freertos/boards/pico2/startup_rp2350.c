/*
 * SPDX-License-Identifier: Apache-2.0
 * Startup code for RP2350 (Cortex-M33)
 */

#include <stdint.h>

/* Linker symbols */
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

/* External main function */
extern int main(void);

/* Default handler for interrupts */
void Default_Handler(void)
{
    while (1) {
        /* Infinite loop */
    }
}

/* Weak aliases for fault handlers */
void NMI_Handler(void)         __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void SecureFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* FreeRTOS interrupt handlers - defined by FreeRTOS CM33 port */
extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);

/* Early debug - direct register access before any init */
#define RESETS_BASE_EARLY    0x40020000
#define IO_BANK0_BASE_EARLY  0x40028000
#define UART0_BASE_EARLY     0x40070000
#define SIO_BASE_EARLY       0xD0000000

static volatile uint32_t *sio_out_xor;

static void early_delay(volatile uint32_t count)
{
    while (count--) {
        __asm volatile ("nop");
    }
}

static void early_led_init(void)
{
    volatile uint32_t *resets = (volatile uint32_t *)(RESETS_BASE_EARLY);
    volatile uint32_t *resets_done = (volatile uint32_t *)(RESETS_BASE_EARLY + 0x8);
    volatile uint32_t *sio_oe_set = (volatile uint32_t *)(SIO_BASE_EARLY + 0x038);
    volatile uint32_t *sio_out_set = (volatile uint32_t *)(SIO_BASE_EARLY + 0x018);
    sio_out_xor = (volatile uint32_t *)(SIO_BASE_EARLY + 0x028);
    
    /* Release IO_BANK0 and PADS from reset */
    *resets &= ~((1 << 6) | (1 << 9));
    while ((*resets_done & ((1 << 6) | (1 << 9))) != ((1 << 6) | (1 << 9)));
    
    /* GPIO 26 = LCD backlight on Pimoroni Explorer */
    volatile uint32_t *gpio26_ctrl = (volatile uint32_t *)(IO_BANK0_BASE_EARLY + 0x04 + (26 * 8));
    
    /* Set to SIO function (5) */
    *gpio26_ctrl = 5;
    
    /* Enable output and turn on (LCD backlight ON) */
    *sio_oe_set = (1 << 26);
    *sio_out_set = (1 << 26);
}

static void early_led_toggle(void)
{
    if (sio_out_xor) {
        *sio_out_xor = (1 << 26);  /* Toggle LCD backlight */
    }
}

static void early_uart_init(void)
{
    volatile uint32_t *resets = (volatile uint32_t *)(RESETS_BASE_EARLY);
    volatile uint32_t *resets_done = (volatile uint32_t *)(RESETS_BASE_EARLY + 0x8);
    volatile uint32_t *gpio0_ctrl = (volatile uint32_t *)(IO_BANK0_BASE_EARLY + 0x04 + (0 * 8));
    volatile uint32_t *gpio1_ctrl = (volatile uint32_t *)(IO_BANK0_BASE_EARLY + 0x04 + (1 * 8));
    volatile uint32_t *uart_ibrd = (volatile uint32_t *)(UART0_BASE_EARLY + 0x24);
    volatile uint32_t *uart_fbrd = (volatile uint32_t *)(UART0_BASE_EARLY + 0x28);
    volatile uint32_t *uart_lcrh = (volatile uint32_t *)(UART0_BASE_EARLY + 0x2C);
    volatile uint32_t *uart_cr = (volatile uint32_t *)(UART0_BASE_EARLY + 0x30);
    
    /* Release UART0 from reset */
    *resets &= ~(1 << 26);
    while (!(*resets_done & (1 << 26)));
    
    /* Set GPIO0 (TX) and GPIO1 (RX) to UART function (2) */
    *gpio0_ctrl = 2;
    *gpio1_ctrl = 2;
    
    /* Configure UART at 12MHz/115200 baud (boot clock is ~12MHz ring osc) */
    /* divisor = 12000000 / (16 * 115200) = 6.51 -> IBRD=6, FBRD=33 */
    *uart_ibrd = 6;
    *uart_fbrd = 33;
    *uart_lcrh = (3 << 5);  /* 8 bits, no FIFO */
    *uart_cr = (1 << 0) | (1 << 8) | (1 << 9);  /* UART enable, TX enable, RX enable */
}

static void early_uart_putc(char c)
{
    volatile uint32_t *uart_dr = (volatile uint32_t *)(UART0_BASE_EARLY + 0x00);
    volatile uint32_t *uart_fr = (volatile uint32_t *)(UART0_BASE_EARLY + 0x18);
    
    while (*uart_fr & (1 << 5));  /* Wait if TX FIFO full */
    *uart_dr = c;
}

static void early_uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') early_uart_putc('\r');
        early_uart_putc(*s++);
    }
}

static void early_boot_test(void)
{
    int i;
    
    early_led_init();
    early_uart_init();
    
    /* Send startup message */
    early_uart_puts("\n\n*** RP2350 BOOT ***\n");
    early_uart_puts("Pimoroni/Pico 2 FreeRTOS Benchmark\n");
    
    /* Blink and send dots to show we're alive */
    for (i = 0; i < 10; i++) {
        early_uart_putc('.');
        early_led_toggle();
        early_delay(500000);
    }
    early_uart_puts("\nStarting main()...\n");
}

/* Reset handler - entry point */
void Reset_Handler(void)
{
    uint32_t *src, *dst;
    
    /* EARLY DEBUG - runs before .data/.bss init */
    early_boot_test();
    
    /* Copy .data section from FLASH to RAM */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    
    /* Zero fill the .bss section */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    
    /* Call the application's entry point */
    main();
    
    /* Should never reach here */
    while (1);
}

/* Vector table */
__attribute__((section(".vectors")))
void (* const vector_table[])(void) = {
    (void (*)(void))(&_estack),  /* Initial Stack Pointer */
    Reset_Handler,               /* Reset Handler */
    NMI_Handler,                 /* NMI Handler */
    HardFault_Handler,           /* Hard Fault Handler */
    MemManage_Handler,           /* MPU Fault Handler */
    BusFault_Handler,            /* Bus Fault Handler */
    UsageFault_Handler,          /* Usage Fault Handler */
    SecureFault_Handler,         /* Secure Fault Handler */
    0,                           /* Reserved */
    0,                           /* Reserved */
    0,                           /* Reserved */
    SVC_Handler,                 /* SVCall Handler (FreeRTOS) */
    DebugMon_Handler,            /* Debug Monitor Handler */
    0,                           /* Reserved */
    PendSV_Handler,              /* PendSV Handler (FreeRTOS) */
    SysTick_Handler,             /* SysTick Handler (FreeRTOS) */
    /* External Interrupts - add more as needed */
};

/* FreeRTOS Tick Hook - called from FreeRTOS SysTick handler */
void vApplicationTickHook(void)
{
    /* Hook for timing overflow tracking */
    extern void arch_timing_systick_overflow(void);
    arch_timing_systick_overflow();
}

/* Provide stubs for newlib */
int _getpid(void)
{
    return 1;
}

void _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
}
