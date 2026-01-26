/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Interrupt Processing Test (No Preemption)
 *
 * This test measures interrupt-to-semaphore throughput without thread preemption.
 * A thread triggers a software interrupt, the ISR posts a semaphore, and the
 * thread waits for the semaphore.
 *
 * NOTE: This test requires custom SVC handler integration which may conflict
 * with FreeRTOS. Currently disabled by default.
 *
 * Adapted for rtos-benchmark2 integration.
 */

#include "tm_adapter.h"

/*******************************************************************************
 * Test Data
 ******************************************************************************/

/* Counters */
volatile unsigned long tm_interrupt_thread_0_counter;
volatile unsigned long tm_interrupt_handler_counter;

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_interrupt_thread_0_entry(void);
void tm_interrupt_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_interrupt_processing_initialize(void)
{
    /* Reset counters */
    tm_interrupt_thread_0_counter = 0;
    tm_interrupt_handler_counter = 0;

    /* Create thread that generates the interrupt at priority 10 */
    tm_thread_create(0, 10, tm_interrupt_thread_0_entry);

    /* Create a semaphore for interrupt-to-thread signaling */
    tm_semaphore_create(0);

    /* Resume thread 0 */
    tm_thread_resume(0);

    /* Create the reporting thread at higher priority (priority 2) */
    tm_thread_create(5, 2, tm_interrupt_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * Interrupt Thread
 *
 * NOTE: The TM_CAUSE_INTERRUPT macro triggers a software interrupt (SVC).
 * This requires proper SVC handler integration that may conflict with FreeRTOS.
 * Currently this test will skip the interrupt trigger and just measure
 * semaphore operations.
 ******************************************************************************/

void tm_interrupt_thread_0_entry(void)
{
    int status;

    /* Get the semaphore initially (it starts with count 1) */
    status = tm_semaphore_get(0);
    if (status != TM_SUCCESS) {
        return;
    }

    while (1) {
#ifdef TM_ENABLE_INTERRUPT_TESTS
        /* Trigger software interrupt
         * NOTE: This requires the SVC handler to call tm_interrupt_handler()
         */
        TM_CAUSE_INTERRUPT
#else
        /* Fallback: simulate by directly calling handler and posting semaphore */
        tm_interrupt_handler_counter++;
        tm_semaphore_put(0);
#endif

        /* Wait for semaphore from interrupt handler */
        status = tm_semaphore_get(0);
        if (status != TM_SUCCESS) {
            return;
        }

        /* Increment counter */
        tm_interrupt_thread_0_counter++;
    }
}

/*******************************************************************************
 * Interrupt Handler
 *
 * This function must be called from the SVC exception handler:
 *
 * void SVC_Handler(void) {
 *     __asm volatile("PUSH {lr}");
 *     tm_interrupt_handler();
 *     __asm volatile("POP {lr}");
 *     __asm volatile("BX lr");
 * }
 ******************************************************************************/

void tm_interrupt_handler(void)
{
    /* Increment the interrupt count */
    tm_interrupt_handler_counter++;

    /* Post the semaphore to signal the waiting thread */
    tm_semaphore_put(0);
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_interrupt_thread_report(void)
{
    unsigned long total;
    unsigned long last_total;
    unsigned long relative_time;
    unsigned long average;

    /* Initialize tracking variables */
    last_total = 0;
    relative_time = 0;

    while (1) {
        /* Sleep for the test duration */
        tm_thread_sleep(TM_TEST_DURATION);

        /* Update relative time */
        relative_time = relative_time + TM_TEST_DURATION;

        /* Print results */
        PRINTF("**** Thread-Metric Interrupt Processing Test **** Relative Time: %lu\n",
               relative_time);

#ifndef TM_ENABLE_INTERRUPT_TESTS
        PRINTF("NOTE: Running in simulation mode (no actual interrupts)\n");
#endif

        /* Calculate total and average */
        total = tm_interrupt_thread_0_counter + tm_interrupt_handler_counter;
        average = total / 2;

        /* Validate: both counters should be within 1 of the average */
        if ((tm_interrupt_thread_0_counter < (average - 1)) ||
            (tm_interrupt_thread_0_counter > (average + 1)) ||
            (tm_interrupt_handler_counter < (average - 1)) ||
            (tm_interrupt_handler_counter > (average + 1))) {
            PRINTF("ERROR: Invalid counter value(s). Interrupt processing test has failed!\n");
        }

        /* Show the total interrupts for the time period */
        PRINTF("Time Period Total:  %lu\n\n", tm_interrupt_handler_counter - last_total);

        /* Save the last total */
        last_total = tm_interrupt_handler_counter;
    }
}
