/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Timer ISR manipulation for RP2350 interrupt latency tests
 *
 * Uses SysTick for the system tick and interrupt latency measurements.
 * The RP2350 has VTOR, so we can redirect the vector table.
 */

#include "bench_api.h"
#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"

/* CMSIS-style definitions for Cortex-M33 */
#define SCB_VTOR_TBLOFF_Msk  (0x1FFFFFFFUL << 7)

/*
 * For the Cortex-M, the system tick ISR is at index 15 (offset 0x3c) in
 * the vector table. We create a copy of the vector table and switch to
 * using that one via VTOR.
 */
#define TIMER_ISR_VECTOR_TABLE_INDEX  15

static bench_isr_handler_t my_vector_table[48] __attribute__((aligned(256)));

bench_isr_handler_t bench_timer_isr_get(void)
{
    bench_isr_handler_t *table = (bench_isr_handler_t *)scb_hw->vtor;
    return table[TIMER_ISR_VECTOR_TABLE_INDEX];
}

uint32_t bench_timer_cycles_per_second(void)
{
    return SYS_CLOCK_HW_CYCLES_PER_SEC;
}

uint32_t bench_timer_cycles_per_tick(void)
{
    return (bench_timer_cycles_per_second() / configTICK_RATE_HZ);
}

void bench_timer_isr_set(bench_isr_handler_t isr)
{
    int i;
    bench_isr_handler_t *src = (bench_isr_handler_t *)scb_hw->vtor;

    /* Copy the current vector table */
    for (i = 0; i < 48; i++) {
        my_vector_table[i] = src[i];
    }

    /* Install our handler */
    my_vector_table[TIMER_ISR_VECTOR_TABLE_INDEX] = isr;
    
    /* Switch to our vector table */
    uint32_t save = save_and_disable_interrupts();
    scb_hw->vtor = (uint32_t)my_vector_table;
    __dsb();
    __isb();
    restore_interrupts(save);
}

bench_time_t bench_timer_cycles_diff(bench_time_t trigger_point,
                                     bench_time_t sample_point)
{
    /* SysTick counts down, so trigger > sample */
    return (trigger_point - sample_point + 1);
}

bench_time_t bench_timer_cycles_get(void)
{
    return (bench_time_t)systick_hw->cvr;
}

bench_time_t bench_timer_isr_expiry_set(uint32_t usec)
{
    uint32_t cycles_per_usec;
    uint32_t cycles;

    cycles_per_usec = (bench_timer_cycles_per_second() + 999999) / 1000000;
    cycles = cycles_per_usec * usec;

    systick_hw->rvr = cycles;
    systick_hw->cvr = cycles - 1;
    systick_hw->csr = 0x7;  /* Enable, interrupt, processor clock */

    return (bench_time_t)cycles;
}

void bench_timer_isr_restore(bench_isr_handler_t handler)
{
    uint32_t cycles;

    cycles = bench_timer_cycles_per_tick() - 1;
    systick_hw->rvr = cycles;
    systick_hw->cvr = 0;  /* Resets timer to cycles */
    systick_hw->csr = 0x7;  /* Enable, interrupt, processor clock */

    bench_timer_isr_set(handler);
}
