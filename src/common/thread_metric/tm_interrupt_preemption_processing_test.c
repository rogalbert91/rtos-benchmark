/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Interrupt Preemption Processing Test
 *
 * This test measures interrupt-to-thread-preemption throughput. A thread
 * triggers a software interrupt, the ISR resumes a higher priority thread
 * which causes preemption.
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
volatile unsigned long tm_interrupt_preemption_thread_0_counter;
volatile unsigned long tm_interrupt_preemption_thread_1_counter;
volatile unsigned long tm_interrupt_preemption_handler_counter;

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_interrupt_preemption_thread_0_entry(void);
void tm_interrupt_preemption_thread_1_entry(void);
void tm_interrupt_preemption_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_interrupt_preemption_processing_initialize(void)
{
    /* Reset counters */
    tm_interrupt_preemption_thread_0_counter = 0;
    tm_interrupt_preemption_thread_1_counter = 0;
    tm_interrupt_preemption_handler_counter = 0;

    /* Create high-priority thread that will be resumed from ISR (priority 3) */
    tm_thread_create(0, 3, tm_interrupt_preemption_thread_0_entry);

    /* Create thread that generates the interrupt (priority 10 - lower) */
    tm_thread_create(1, 10, tm_interrupt_preemption_thread_1_entry);

    /* Only resume thread 1 - thread 0 will be resumed from ISR */
    tm_thread_resume(1);

    /* Create the reporting thread at highest priority (priority 2) */
    tm_thread_create(5, 2, tm_interrupt_preemption_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * High-Priority Thread (resumed from ISR)
 ******************************************************************************/

void tm_interrupt_preemption_thread_0_entry(void)
{
    while (1) {
        /* Increment counter */
        tm_interrupt_preemption_thread_0_counter++;

        /* Suspend self - returns control to the interrupt-generating thread */
        tm_thread_suspend(0);
    }
}

/*******************************************************************************
 * Interrupt-Generating Thread
 *
 * NOTE: The TM_CAUSE_INTERRUPT macro triggers a software interrupt (SVC).
 * This requires proper SVC handler integration that may conflict with FreeRTOS.
 ******************************************************************************/

void tm_interrupt_preemption_thread_1_entry(void)
{
    while (1) {
#ifdef TM_ENABLE_INTERRUPT_TESTS
        /* Trigger software interrupt
         * NOTE: This requires the SVC handler to call tm_interrupt_preemption_handler()
         */
        TM_CAUSE_INTERRUPT
#else
        /* Fallback: simulate by directly calling handler */
        tm_interrupt_preemption_handler_counter++;
        tm_thread_resume(0);  /* Resume high-priority thread, causing preemption */
#endif

        /* We return here after interrupt processing and thread 0 self-suspends */
        tm_interrupt_preemption_thread_1_counter++;
    }
}

/*******************************************************************************
 * Interrupt Handler
 *
 * This function must be called from the SVC exception handler:
 *
 * void SVC_Handler(void) {
 *     __asm volatile("PUSH {lr}");
 *     tm_interrupt_preemption_handler();
 *     __asm volatile("POP {lr}");
 *     __asm volatile("BX lr");
 * }
 ******************************************************************************/

void tm_interrupt_preemption_handler(void)
{
    /* Increment the interrupt count */
    tm_interrupt_preemption_handler_counter++;

    /* Resume the higher priority thread - this causes preemption */
    tm_thread_resume(0);
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_interrupt_preemption_thread_report(void)
{
    unsigned long total;
    unsigned long relative_time;
    unsigned long last_total;
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
        PRINTF("**** Thread-Metric Interrupt Preemption Processing Test **** Relative Time: %lu\n",
               relative_time);

#ifndef TM_ENABLE_INTERRUPT_TESTS
        PRINTF("NOTE: Running in simulation mode (no actual interrupts)\n");
#endif

        /* Calculate total and average */
        total = tm_interrupt_preemption_thread_0_counter +
                tm_interrupt_preemption_thread_1_counter +
                tm_interrupt_preemption_handler_counter;

        average = total / 3;

        /* Validate: all counters should be within 1 of the average */
        if ((tm_interrupt_preemption_thread_0_counter < (average - 1)) ||
            (tm_interrupt_preemption_thread_0_counter > (average + 1)) ||
            (tm_interrupt_preemption_thread_1_counter < (average - 1)) ||
            (tm_interrupt_preemption_thread_1_counter > (average + 1)) ||
            (tm_interrupt_preemption_handler_counter < (average - 1)) ||
            (tm_interrupt_preemption_handler_counter > (average + 1))) {
            PRINTF("ERROR: Invalid counter value(s). Interrupt preemption test has failed!\n");
        }

        /* Show the total interrupts for the time period */
        PRINTF("Time Period Total:  %lu\n\n", tm_interrupt_preemption_handler_counter - last_total);

        /* Save the last total */
        last_total = tm_interrupt_preemption_handler_counter;
    }
}
