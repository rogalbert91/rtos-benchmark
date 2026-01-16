/*
 * Board support for RP2350 (Pico 2 / Pimoroni Explorer) with Pico SDK
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"

/* Board initialization */
void BOARD_InitBootPins(void)
{
    /* Initialize activity indicator pin (LCD backlight on Pimoroni Explorer) */
    gpio_init(LCD_BACKLIGHT_PIN);
    gpio_set_dir(LCD_BACKLIGHT_PIN, GPIO_OUT);
    gpio_put(LCD_BACKLIGHT_PIN, 1);  /* Turn on initially */
}

void BOARD_InitBootClocks(void)
{
    /* Pico SDK handles clock initialization via stdio_init_all() */
}

void BOARD_InitDebugConsole(void)
{
    stdio_init_all();
    printf("\n*** RP2350 FreeRTOS Benchmark ***\n");
    printf("CPU: %lu MHz\n", (unsigned long)(clock_get_hz(clk_sys) / 1000000));
}

/* Activity indicator toggle */
void board_led_toggle(void)
{
    static bool led_state = false;
    led_state = !led_state;
    gpio_put(LCD_BACKLIGHT_PIN, led_state);
}
