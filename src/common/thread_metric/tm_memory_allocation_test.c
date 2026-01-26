/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Memory Allocation Test
 *
 * This test measures memory pool allocate/deallocate throughput. A single
 * thread allocates and releases 128-byte blocks repeatedly.
 *
 * Adapted for rtos-benchmark2 integration.
 */

#include "tm_adapter.h"

/*******************************************************************************
 * Test Data
 ******************************************************************************/

/* Counter for completed allocation/deallocation cycles */
volatile unsigned long tm_memory_allocation_counter;

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_memory_allocation_thread_0_entry(void);
void tm_memory_allocation_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_memory_allocation_initialize(void)
{
    /* Reset counter */
    tm_memory_allocation_counter = 0;

    /* Create thread 0 at priority 10 */
    tm_thread_create(0, 10, tm_memory_allocation_thread_0_entry);

    /* Resume thread 0 */
    tm_thread_resume(0);

    /* Create a memory pool */
    tm_memory_pool_create(0);

    /* Create the reporting thread at higher priority (priority 2) */
    tm_thread_create(5, 2, tm_memory_allocation_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * Memory Allocation Thread
 ******************************************************************************/

void tm_memory_allocation_thread_0_entry(void)
{
    int status;
    unsigned char *memory_ptr;

    while (1) {
        /* Allocate a 128-byte block from the pool */
        tm_memory_pool_allocate(0, &memory_ptr);

        /* Release the block back to the pool */
        status = tm_memory_pool_deallocate(0, memory_ptr);

        /* Check for error */
        if (status != TM_SUCCESS) {
            break;
        }

        /* Increment the counter */
        tm_memory_allocation_counter++;
    }
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_memory_allocation_thread_report(void)
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
        PRINTF("**** Thread-Metric Memory Allocation Test **** Relative Time: %lu\n",
               relative_time);

        /* Check for errors */
        if (tm_memory_allocation_counter == last_counter) {
            PRINTF("ERROR: Invalid counter value(s). Error allocating/deallocating memory!\n");
        }

        /* Show the time period total */
        PRINTF("Time Period Total:  %lu\n\n", tm_memory_allocation_counter - last_counter);

        /* Save the last counter */
        last_counter = tm_memory_allocation_counter;
    }
}
