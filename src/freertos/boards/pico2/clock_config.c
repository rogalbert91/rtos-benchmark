/*
 * SPDX-License-Identifier: Apache-2.0
 * Clock configuration implementation for RP2350
 */

#include "clock_config.h"

/* System clock frequency variable - defined in board.c */
extern uint32_t SystemCoreClock;

void SystemCoreClockUpdate(void)
{
    /* 
     * For now, assume default 150 MHz.
     * A full implementation would read the actual clock configuration.
     */
    SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;
}
