/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2024 Microsoft Corporation
 *
 * Thread-Metric Message Processing Test
 *
 * This test measures message queue send/receive throughput. A single thread
 * sends a 16-byte message to a queue and then receives it back.
 *
 * Adapted for rtos-benchmark2 integration.
 */

#include "tm_adapter.h"

/*******************************************************************************
 * Test Data
 ******************************************************************************/

/* Counter for completed message send/receive cycles */
volatile unsigned long tm_message_processing_counter;

/* Message buffers */
/* Message buffers - don't need volatile as queue API handles synchronization */
unsigned long tm_message_sent[4];
unsigned long tm_message_received[4];

/*******************************************************************************
 * Thread Prototypes
 ******************************************************************************/

void tm_message_processing_thread_0_entry(void);
void tm_message_processing_thread_report(void);

/*******************************************************************************
 * Test Initialization
 ******************************************************************************/

void tm_message_processing_initialize(void)
{
    /* Reset counter */
    tm_message_processing_counter = 0;

    /* Create thread 0 at priority 10 */
    tm_thread_create(0, 10, tm_message_processing_thread_0_entry);

    /* Resume thread 0 */
    tm_thread_resume(0);

    /* Create a queue for message passing */
    tm_queue_create(0);

    /* Create the reporting thread at higher priority (priority 2) */
    tm_thread_create(5, 2, tm_message_processing_thread_report);
    tm_thread_resume(5);
}

/*******************************************************************************
 * Message Processing Thread
 ******************************************************************************/

void tm_message_processing_thread_0_entry(void)
{
    /* Initialize the source message with test pattern */
    tm_message_sent[0] = 0x11112222;
    tm_message_sent[1] = 0x33334444;
    tm_message_sent[2] = 0x55556666;
    tm_message_sent[3] = 0x77778888;

    while (1) {
        /* Send a message to the queue */
        tm_queue_send(0, tm_message_sent);

        /* Receive a message from the queue */
        tm_queue_receive(0, tm_message_received);

        /* Validate the received message */
        if (tm_message_received[3] != tm_message_sent[3]) {
            break;  /* Message corruption detected */
        }

        /* Increment the last word of the message for next iteration */
        tm_message_sent[3]++;

        /* Increment the counter */
        tm_message_processing_counter++;
    }
}

/*******************************************************************************
 * Reporting Thread
 ******************************************************************************/

void tm_message_processing_thread_report(void)
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
        PRINTF("**** Thread-Metric Message Processing Test **** Relative Time: %lu\n",
               relative_time);

        /* Check for errors */
        if (tm_message_processing_counter == last_counter) {
            PRINTF("ERROR: Invalid counter value(s). Error sending/receiving messages!\n");
        }

        /* Show the time period total */
        PRINTF("Time Period Total:  %lu\n\n", tm_message_processing_counter - last_counter);

        /* Save the last counter */
        last_counter = tm_message_processing_counter;
    }
}
