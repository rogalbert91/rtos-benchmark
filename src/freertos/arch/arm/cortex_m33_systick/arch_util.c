/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Timing implementation for Cortex-M33 WITHOUT DWT.
 * Uses SysTick for cycle counting.
 */

#include "arch_api.h"
#include "bench_api.h"
#include "bench_utils.h"
#include "FreeRTOS.h"
#include "core_cm33.h"

#include <stdint.h>

#define NSEC_PER_SEC 1000000000ULL

/*
 * Since RP2350 doesn't have DWT, we implement a software-based
 * cycle counter using SysTick. This tracks total elapsed cycles
 * by counting SysTick overflows and the current SysTick value.
 */

static volatile uint32_t systick_overflow_count = 0;
static volatile uint32_t systick_reload_value = 0;
static volatile int timing_active = 0;

/*
 * Hook to be called from SysTick handler to track overflows.
 * This should be called from the FreeRTOS tick hook or
 * a wrapped SysTick handler.
 */
void arch_timing_systick_overflow(void)
{
    if (timing_active) {
        systick_overflow_count++;
    }
}

void arch_timing_init(void)
{
    /*
     * Configure SysTick for maximum count (24-bit counter).
     * SysTick counts DOWN from LOAD to 0.
     * We use the processor clock as the source.
     */
    systick_overflow_count = 0;
    systick_reload_value = SysTick_LOAD_RELOAD_Msk;  /* Maximum 24-bit value */
    
    /* Stop SysTick first */
    SysTick->CTRL = 0;
    
    /* Set reload value to maximum */
    SysTick->LOAD = systick_reload_value;
    
    /* Clear current value */
    SysTick->VAL = 0;
    
    timing_active = 0;
}

void arch_timing_start(void)
{
    /* Reset counters */
    systick_overflow_count = 0;
    
    /* Clear current value */
    SysTick->VAL = 0;
    
    /* Enable SysTick with processor clock, enable interrupt */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_CLKSOURCE_Msk;
    
    timing_active = 1;
}

void arch_timing_stop(void)
{
    timing_active = 0;
    /* Don't disable SysTick as FreeRTOS needs it */
}

bench_time_t arch_timing_counter_get(void)
{
    uint32_t overflow;
    uint32_t current_val;
    uint32_t reload;
    
    /*
     * Read overflow count and current value atomically.
     * SysTick counts DOWN, so we need to invert the logic.
     * Total cycles = (overflow_count * reload_value) + (reload_value - current_val)
     */
    do {
        overflow = systick_overflow_count;
        current_val = SysTick->VAL;
        reload = systick_reload_value;
    } while (overflow != systick_overflow_count);  /* Retry if overflow during read */
    
    /* Calculate total cycles (SysTick counts down) */
    bench_time_t total = ((bench_time_t)overflow * (reload + 1)) + 
                         (reload - current_val);
    
    return total;
}

bench_time_t arch_timing_cycles_get(volatile bench_time_t *const start,
                                    volatile bench_time_t *const end)
{
    /* Simple subtraction - both are up-counting values now */
    return (*end - *start);
}

bench_time_t arch_timing_cycles_to_ns(bench_time_t cycles)
{
    return (cycles * NSEC_PER_SEC) / SYS_CLOCK_HW_CYCLES_PER_SEC;
}
