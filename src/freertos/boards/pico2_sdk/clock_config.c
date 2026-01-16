/*
 * SPDX-License-Identifier: Apache-2.0
 * Clock configuration implementation for RP2350
 */

#include "clock_config.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"

/* System clock frequency variable (required by some code) */
uint32_t SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;

void SystemCoreClockUpdate(void)
{
    /* Update SystemCoreClock based on actual clock configuration */
    SystemCoreClock = clock_get_hz(clk_sys);
}
