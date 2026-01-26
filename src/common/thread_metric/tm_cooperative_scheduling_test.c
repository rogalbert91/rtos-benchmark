/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Cooperative Scheduling Test
 *
 * This test measures cooperative (voluntary) context switch throughput.
 * It consists of 5 threads at the same priority that yield to each other
 * in a round-robin fashion.
 *
 * Adapted for rtos-benchmark2 integration.
 */

#include "tm_adapter.h"

/*******************************************************************************
 * Test Data
 ******************************************************************************/

/* Counters for each cooperative thread */
volatile unsigned long tm_cooperative_thread_0_counter;
volatile unsigned long tm_cooperative_thread_1_counter;
volatile unsigned long tm_cooperative_thread_2_counter;
volatile unsigned long tm_cooperative_thread_3_counter;
volatile unsigned long tm_cooperative_thread_4_counter;

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_cooperative_thread_0_entry(void);
void tm_cooperative_thread_1_entry(void);
void tm_cooperative_thread_2_entry(void);
void tm_cooperative_thread_3_entry(void);
void tm_cooperative_thread_4_entry(void);
void tm_cooperative_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_cooperative_scheduling_initialize(void)
{
    /* Reset counters */
    tm_cooperative_thread_0_counter = 0;
    tm_cooperative_thread_1_counter = 0;
    tm_cooperative_thread_2_counter = 0;
    tm_cooperative_thread_3_counter = 0;
    tm_cooperative_thread_4_counter = 0;

    /* Create all 5 threads at the same priority (priority 3) */
    tm_thread_create(0, 3, tm_cooperative_thread_0_entry);
    tm_thread_create(1, 3, tm_cooperative_thread_1_entry);
    tm_thread_create(2, 3, tm_cooperative_thread_2_entry);
    tm_thread_create(3, 3, tm_cooperative_thread_3_entry);
    tm_thread_create(4, 3, tm_cooperative_thread_4_entry);

    /* Resume all 5 threads */
    tm_thread_resume(0);
    tm_thread_resume(1);
    tm_thread_resume(2);
    tm_thread_resume(3);
    tm_thread_resume(4);

    /* Create the reporting thread at higher priority (priority 2) */
    tm_thread_create(5, 2, tm_cooperative_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * Cooperative Threads
 ******************************************************************************/

void tm_cooperative_thread_0_entry(void)
{
    while (1) {
        /* Relinquish to other threads at same priority */
        tm_thread_relinquish();

        /* Increment counter after regaining control */
        tm_cooperative_thread_0_counter++;
    }
}

void tm_cooperative_thread_1_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_1_counter++;
    }
}

void tm_cooperative_thread_2_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_2_counter++;
    }
}

void tm_cooperative_thread_3_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_3_counter++;
    }
}

void tm_cooperative_thread_4_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_4_counter++;
    }
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_cooperative_thread_report(void)
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
        PRINTF("**** Thread-Metric Cooperative Scheduling Test **** Relative Time: %lu\n",
               relative_time);

        /* Calculate total and average */
        total = tm_cooperative_thread_0_counter +
                tm_cooperative_thread_1_counter +
                tm_cooperative_thread_2_counter +
                tm_cooperative_thread_3_counter +
                tm_cooperative_thread_4_counter;

        average = total / 5;

        /* Print individual counter values for debugging */
        PRINTF("tm_cooperative_thread_0_counter: %lu\n", tm_cooperative_thread_0_counter);
        PRINTF("tm_cooperative_thread_1_counter: %lu\n", tm_cooperative_thread_1_counter);
        PRINTF("tm_cooperative_thread_2_counter: %lu\n", tm_cooperative_thread_2_counter);
        PRINTF("tm_cooperative_thread_3_counter: %lu\n", tm_cooperative_thread_3_counter);
        PRINTF("tm_cooperative_thread_4_counter: %lu\n", tm_cooperative_thread_4_counter);

        /* Validate: all counters should be within 1 of the average */
        if ((tm_cooperative_thread_0_counter < (average - 1)) ||
            (tm_cooperative_thread_0_counter > (average + 1)) ||
            (tm_cooperative_thread_1_counter < (average - 1)) ||
            (tm_cooperative_thread_1_counter > (average + 1)) ||
            (tm_cooperative_thread_2_counter < (average - 1)) ||
            (tm_cooperative_thread_2_counter > (average + 1)) ||
            (tm_cooperative_thread_3_counter < (average - 1)) ||
            (tm_cooperative_thread_3_counter > (average + 1)) ||
            (tm_cooperative_thread_4_counter < (average - 1)) ||
            (tm_cooperative_thread_4_counter > (average + 1))) {
            PRINTF("ERROR: Invalid counter value(s). Cooperative counters should not be more than 1 different than the average!\n");
        }

        /* Show the time period total */
        PRINTF("Time Period Total:  %lu\n\n", total - last_total);

        /* Save the last total */
        last_total = total;
    }
}
