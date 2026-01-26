/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Preemptive Scheduling Test
 *
 * This test measures preemptive context switch throughput. It consists of
 * 5 threads at different priorities that form a resume/suspend chain.
 *
 * The lowest priority thread resumes the next higher priority thread,
 * which immediately preempts it. This continues up the priority chain
 * until the highest priority thread runs and suspends itself.
 *
 * Adapted for rtos-benchmark2 integration.
 */

#include "tm_adapter.h"

/*******************************************************************************
 * Test Data
 ******************************************************************************/

/* Counters for each preemptive thread */
volatile unsigned long tm_preemptive_thread_0_counter;
volatile unsigned long tm_preemptive_thread_1_counter;
volatile unsigned long tm_preemptive_thread_2_counter;
volatile unsigned long tm_preemptive_thread_3_counter;
volatile unsigned long tm_preemptive_thread_4_counter;

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_preemptive_thread_0_entry(void);
void tm_preemptive_thread_1_entry(void);
void tm_preemptive_thread_2_entry(void);
void tm_preemptive_thread_3_entry(void);
void tm_preemptive_thread_4_entry(void);
void tm_preemptive_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_preemptive_scheduling_initialize(void)
{
    /* Reset counters */
    tm_preemptive_thread_0_counter = 0;
    tm_preemptive_thread_1_counter = 0;
    tm_preemptive_thread_2_counter = 0;
    tm_preemptive_thread_3_counter = 0;
    tm_preemptive_thread_4_counter = 0;

    /* Create threads at different priorities
     * Thread 0: priority 10 (lowest of the 5)
     * Thread 1: priority 9
     * Thread 2: priority 8
     * Thread 3: priority 7
     * Thread 4: priority 6 (highest of the 5)
     */
    tm_thread_create(0, 10, tm_preemptive_thread_0_entry);
    tm_thread_create(1, 9, tm_preemptive_thread_1_entry);
    tm_thread_create(2, 8, tm_preemptive_thread_2_entry);
    tm_thread_create(3, 7, tm_preemptive_thread_3_entry);
    tm_thread_create(4, 6, tm_preemptive_thread_4_entry);

    /* Only resume thread 0 - it will resume the others */
    tm_thread_resume(0);

    /* Create the reporting thread at highest priority (priority 2) */
    tm_thread_create(5, 2, tm_preemptive_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * Preemptive Threads
 *
 * Execution flow:
 * 1. Thread 0 (P10) runs, resumes Thread 1 (P9) -> preempted
 * 2. Thread 1 (P9) runs, resumes Thread 2 (P8) -> preempted
 * 3. Thread 2 (P8) runs, resumes Thread 3 (P7) -> preempted
 * 4. Thread 3 (P7) runs, resumes Thread 4 (P6) -> preempted
 * 5. Thread 4 (P6) runs, increments counter, suspends self -> returns to Thread 3
 * 6. Thread 3 increments counter, suspends self -> returns to Thread 2
 * 7. Thread 2 increments counter, suspends self -> returns to Thread 1
 * 8. Thread 1 increments counter, suspends self -> returns to Thread 0
 * 9. Thread 0 increments counter, starts over
 ******************************************************************************/

void tm_preemptive_thread_0_entry(void)
{
    while (1) {
        /* Resume thread 1 - this will cause preemption */
        tm_thread_resume(1);

        /* We return here after threads 1, 2, 3, and 4 have all
         * executed and self-suspended.
         */

        /* Increment counter */
        tm_preemptive_thread_0_counter++;
    }
}

void tm_preemptive_thread_1_entry(void)
{
    while (1) {
        /* Resume thread 2 - causes preemption */
        tm_thread_resume(2);

        /* Return here after threads 2, 3, and 4 self-suspend */
        tm_preemptive_thread_1_counter++;

        /* Suspend self */
        tm_thread_suspend(1);
    }
}

void tm_preemptive_thread_2_entry(void)
{
    while (1) {
        /* Resume thread 3 - causes preemption */
        tm_thread_resume(3);

        /* Return here after threads 3 and 4 self-suspend */
        tm_preemptive_thread_2_counter++;

        /* Suspend self */
        tm_thread_suspend(2);
    }
}

void tm_preemptive_thread_3_entry(void)
{
    while (1) {
        /* Resume thread 4 - causes preemption */
        tm_thread_resume(4);

        /* Return here after thread 4 self-suspends */
        tm_preemptive_thread_3_counter++;

        /* Suspend self */
        tm_thread_suspend(3);
    }
}

void tm_preemptive_thread_4_entry(void)
{
    while (1) {
        /* Highest priority in the chain - just increment and suspend */
        tm_preemptive_thread_4_counter++;

        /* Suspend self - returns control to thread 3 */
        tm_thread_suspend(4);
    }
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_preemptive_thread_report(void)
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

        /* Print results header */
        PRINTF("**** Thread-Metric Preemptive Scheduling Test **** Relative Time: %lu\n",
               relative_time);

        /* Calculate total and average */
        total = tm_preemptive_thread_0_counter +
                tm_preemptive_thread_1_counter +
                tm_preemptive_thread_2_counter +
                tm_preemptive_thread_3_counter +
                tm_preemptive_thread_4_counter;

        average = total / 5;

        /* Validate: all counters should be within 1 of the average */
        if ((tm_preemptive_thread_0_counter < (average - 1)) ||
            (tm_preemptive_thread_0_counter > (average + 1)) ||
            (tm_preemptive_thread_1_counter < (average - 1)) ||
            (tm_preemptive_thread_1_counter > (average + 1)) ||
            (tm_preemptive_thread_2_counter < (average - 1)) ||
            (tm_preemptive_thread_2_counter > (average + 1)) ||
            (tm_preemptive_thread_3_counter < (average - 1)) ||
            (tm_preemptive_thread_3_counter > (average + 1)) ||
            (tm_preemptive_thread_4_counter < (average - 1)) ||
            (tm_preemptive_thread_4_counter > (average + 1))) {
            PRINTF("ERROR: Invalid counter value(s). Preemptive counters should not be more than 1 different than the average!\n");
        }

        /* Show the time period total */
        PRINTF("Time Period Total:  %lu\n\n", total - last_total);

        /* Save the last total */
        last_total = total;
    }
}
