/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Synchronization Processing Test
 *
 * This test measures semaphore get/put throughput. A single thread
 * acquires and releases a semaphore repeatedly.
 *
 * Adapted for rtos-benchmark2 integration.
 */

#include "tm_adapter.h"

/*******************************************************************************
 * Test Data
 ******************************************************************************/

/* Counter for completed semaphore get/put cycles */
volatile unsigned long tm_synchronization_processing_counter;

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_synchronization_processing_thread_0_entry(void);
void tm_synchronization_processing_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_synchronization_processing_initialize(void)
{
    /* Reset counter */
    tm_synchronization_processing_counter = 0;

    /* Create thread 0 at priority 10 */
    tm_thread_create(0, 10, tm_synchronization_processing_thread_0_entry);

    /* Resume thread 0 */
    tm_thread_resume(0);

    /* Create a semaphore for the test */
    tm_semaphore_create(0);

    /* Create the reporting thread at higher priority (priority 2) */
    tm_thread_create(5, 2, tm_synchronization_processing_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * Synchronization Processing Thread
 ******************************************************************************/

void tm_synchronization_processing_thread_0_entry(void)
{
    int status;

    while (1) {
        /* Get the semaphore */
        tm_semaphore_get(0);

        /* Release the semaphore */
        status = tm_semaphore_put(0);

        /* Check for error */
        if (status != TM_SUCCESS) {
            break;
        }

        /* Increment the counter */
        tm_synchronization_processing_counter++;
    }
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_synchronization_processing_thread_report(void)
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
        PRINTF("**** Thread-Metric Synchronization Processing Test **** Relative Time: %lu\n",
               relative_time);

        /* Check for errors */
        if (tm_synchronization_processing_counter == last_counter) {
            PRINTF("ERROR: Invalid counter value(s). Error getting/putting semaphore!\n");
        }

        /* Show the time period total */
        PRINTF("Time Period Total:  %lu\n\n", tm_synchronization_processing_counter - last_counter);

        /* Save the last counter */
        last_counter = tm_synchronization_processing_counter;
    }
}
