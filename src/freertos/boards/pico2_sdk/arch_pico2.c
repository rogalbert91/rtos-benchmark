/*
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Timing implementation for RP2350
 * 
 * Two timing modes available (controlled by USE_DWT_TIMING):
 *   0 = time_us_64() - Reliable, ~1µs resolution (default)
 *   1 = DWT cycle counter - Cycle-accurate (~6.67ns at 150MHz)
 *
 * To enable DWT, add to cmake: -DUSE_DWT_TIMING=1
 * Or define USE_DWT_TIMING=1 before including this file.
 *
 * NOTE: DWT on RP2350 may have issues - use with caution.
 */

#include "arch_api.h"
#include "bench_api.h"
#include "bench_utils.h"
#include "FreeRTOS.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/structs/systick.h"
#include "hardware/exception.h"

#include <stdint.h>

#define NSEC_PER_SEC  1000000000ULL
#define USEC_PER_SEC  1000000ULL

/*
 * Configuration: Set USE_DWT_TIMING to switch timing source
 *   0 = time_us_64() (default, reliable)
 *   1 = DWT cycle counter (experimental)
 */
#ifndef USE_DWT_TIMING
#define USE_DWT_TIMING 0
#endif

/* ========================================================================== */
#if USE_DWT_TIMING
/* ========================================================================== */
/*
 * DWT-based timing implementation
 * 
 * Uses the Cortex-M33 DWT cycle counter for cycle-accurate timing.
 * Resolution: 1 cycle (~6.67ns at 150MHz)
 * 
 * WARNING: DWT on RP2350 may have issues with FreeRTOS. Use for testing only.
 */

/* DWT (Data Watchpoint and Trace) registers */
#define DWT_BASE        0xE0001000UL
#define DWT_CTRL        (*(volatile uint32_t *)(DWT_BASE + 0x000))
#define DWT_CYCCNT      (*(volatile uint32_t *)(DWT_BASE + 0x004))

/* CoreDebug registers */
#define CoreDebug_BASE  0xE000EDF0UL
#define CoreDebug_DEMCR (*(volatile uint32_t *)(CoreDebug_BASE + 0x00C))

/* Bit definitions */
#define CoreDebug_DEMCR_TRCENA_Msk  (1UL << 24)  /* Enable DWT and ITM */
#define DWT_CTRL_CYCCNTENA_Msk      (1UL << 0)   /* Enable cycle counter */

static uint32_t timing_start_cycles = 0;

void arch_timing_init(void)
{
    /* Enable DWT and ITM blocks */
    CoreDebug_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    /* Enable the cycle counter (do NOT reset it) */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void arch_timing_start(void)
{
    /* Record current cycle count as start point */
    timing_start_cycles = DWT_CYCCNT;
}

void arch_timing_stop(void)
{
    /* Nothing to do - counter keeps running */
}

bench_time_t arch_timing_counter_get(void)
{
    /* Return elapsed cycles since timing_start */
    uint32_t now = DWT_CYCCNT;
    return (bench_time_t)(now - timing_start_cycles);
}

/* ========================================================================== */
#else /* !USE_DWT_TIMING */
/* ========================================================================== */
/*
 * time_us_64() based timing implementation (DEFAULT)
 * 
 * Uses the Pico SDK's high-resolution timer which provides microsecond
 * resolution. We convert to/from "cycles" by multiplying/dividing by
 * the clock frequency.
 * 
 * Resolution: ~1µs (150 cycles at 150MHz)
 */

static uint64_t timing_start_us = 0;

void arch_timing_init(void)
{
    /* Pico SDK timer is already initialized by stdio_init_all() */
}

void arch_timing_start(void)
{
    timing_start_us = time_us_64();
}

void arch_timing_stop(void)
{
    /* Nothing to do - timer runs continuously */
}

bench_time_t arch_timing_counter_get(void)
{
    /* 
     * Return elapsed time in "cycles" 
     * cycles = microseconds * (clock_hz / 1000000)
     * For 150MHz: 1us = 150 cycles
     */
    uint64_t now_us = time_us_64();
    uint64_t elapsed_us = now_us - timing_start_us;
    
    /* Convert microseconds to cycles */
    return (bench_time_t)(elapsed_us * (SYS_CLOCK_HW_CYCLES_PER_SEC / USEC_PER_SEC));
}

#endif /* USE_DWT_TIMING */
/* ========================================================================== */

/*
 * Common functions for both timing implementations
 */

bench_time_t arch_timing_cycles_get(volatile bench_time_t *const start,
                                    volatile bench_time_t *const end)
{
    return (*end - *start);
}

bench_time_t arch_timing_cycles_to_ns(bench_time_t cycles)
{
    return (cycles * NSEC_PER_SEC) / SYS_CLOCK_HW_CYCLES_PER_SEC;
}
