/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Basic Processing Test
 *
 * This test establishes a baseline for processing capability independent
 * of RTOS overhead. It consists of a single thread performing array
 * manipulation operations.
 *
 * Adapted for rtos-benchmark2 integration.
 */

#include "tm_adapter.h"

/*******************************************************************************
 * Test Data
 ******************************************************************************/

/* Counter for completed processing loops - must be volatile for cross-thread visibility */
volatile unsigned long tm_basic_processing_counter;

/* Test array - used to consume processing bandwidth */
volatile unsigned long tm_basic_processing_array[1024];

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_basic_processing_thread_0_entry(void);
void tm_basic_processing_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_basic_processing_initialize(void)
{
    /* Reset counter */
    tm_basic_processing_counter = 0;

    /* Create thread 0 at priority 10 (worker thread) */
    tm_thread_create(0, 10, tm_basic_processing_thread_0_entry);

    /* Resume thread 0 */
    tm_thread_resume(0);

    /* Create the reporting thread at priority 2 (higher priority)
     * It will preempt the worker thread to print results.
     */
    tm_thread_create(5, 2, tm_basic_processing_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * Worker Thread
 ******************************************************************************/

void tm_basic_processing_thread_0_entry(void)
{
    int i;

    /* Initialize the test array */
    for (i = 0; i < 1024; i++) {
        tm_basic_processing_array[i] = 0;
    }

    while (1) {
        /* Loop through the array, performing calculations to consume
         * processing bandwidth. This should be identical on all RTOSes
         * assuming the same processor and clock speed.
         */
        for (i = 0; i < 1024; i++) {
            tm_basic_processing_array[i] =
                (tm_basic_processing_array[i] + tm_basic_processing_counter) ^
                tm_basic_processing_array[i];
        }

        /* Increment the processing counter */
        tm_basic_processing_counter++;
    }
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_basic_processing_thread_report(void)
{
    unsigned long last_counter;
    unsigned long relative_time;


    /* Initialize tracking variables */
    last_counter = 0;
    relative_time = 0;

    while (1) {
        /* Sleep for the test duration */
        tm_thread_sleep(TM_TEST_DURATION);

        /* Update relative time */
        relative_time = relative_time + TM_TEST_DURATION;

        /* Print results */
        PRINTF("**** Thread-Metric Basic Single Thread Processing Test **** Relative Time: %lu\n",
               relative_time);

        /* Check for errors */
        if (tm_basic_processing_counter == last_counter) {
            PRINTF("ERROR: Invalid counter value(s). Basic processing thread died!\n");
        }

        /* Show the time period total */
        PRINTF("Time Period Total:  %lu\n\n", tm_basic_processing_counter - last_counter);

        /* Save the last counter for next iteration */
        last_counter = tm_basic_processing_counter;
    }
}
