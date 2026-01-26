/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Thread-Metric Adapter Layer for rtos-benchmark2
 *
 * This header provides the Thread-Metric API declarations that map
 * to the rtos-benchmark2 bench_* API functions.
 *
 * Thread-Metric is a throughput-based benchmark that measures how many
 * RTOS operations can be performed in a fixed time period (default 30 seconds).
 *
 * Copyright (c) 2024 Microsoft Corporation (Original Thread-Metric)
 * Adapter layer for rtos-benchmark2
 */

#ifndef TM_ADAPTER_H
#define TM_ADAPTER_H

#include "bench_api.h"
#include "bench_utils.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Configuration
 ******************************************************************************/

/* Test duration in seconds (default: 30 seconds per Thread-Metric spec) */
#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION    30
#endif

/* Maximum number of each resource type */
#define TM_MAX_THREADS          10
#define TM_MAX_QUEUES           2
#define TM_MAX_SEMAPHORES       2
#define TM_MAX_MEMORY_POOLS     2

/* Memory pool configuration */
#define TM_BLOCK_SIZE           128
#define TM_POOL_SIZE            2048
#define TM_BLOCKS_PER_POOL      (TM_POOL_SIZE / TM_BLOCK_SIZE)

/* Message queue configuration (16-byte messages = 4 x unsigned long) */
#define TM_QUEUE_MSG_SIZE       16

/*******************************************************************************
 * Return Codes
 ******************************************************************************/

#define TM_SUCCESS  BENCH_SUCCESS
#define TM_ERROR    BENCH_ERROR

/*******************************************************************************
 * Interrupt Trigger Macro
 *
 * This macro triggers a software interrupt for the interrupt processing tests.
 * The implementation is architecture-specific.
 ******************************************************************************/

#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
/* Cortex-M33, Cortex-M4, Cortex-M7 */
#define TM_CAUSE_INTERRUPT    __asm volatile("SVC #0")
#elif defined(__ARM_ARCH_6M__)
/* Cortex-M0/M0+ */
#define TM_CAUSE_INTERRUPT    __asm volatile("SVC #0")
#else
/* Fallback - may need adjustment for other architectures */
#define TM_CAUSE_INTERRUPT    __asm volatile("SVC #0")
#endif

/*******************************************************************************
 * Thread-Metric API Declarations
 *
 * These functions are implemented in tm_adapter.c and provide the mapping
 * from Thread-Metric API to rtos-benchmark2 bench_* API.
 ******************************************************************************/

/**
 * @brief Initialize the test and start the RTOS
 *
 * This function is called from tm_main() in each test. It performs RTOS
 * initialization, calls the test initialization function, and starts the
 * RTOS scheduler.
 *
 * @param test_initialization_function The test-specific initialization function
 */
void tm_initialize(void (*test_initialization_function)(void));

/**
 * @brief Create a thread
 *
 * Creates a thread with the specified priority. The thread is created in
 * a suspended state and must be resumed with tm_thread_resume().
 *
 * Thread-Metric priority: 1 (highest) to 31 (lowest)
 *
 * @param thread_id    Thread identifier (0 to TM_MAX_THREADS-1)
 * @param priority     Thread priority (1=highest, 31=lowest)
 * @param entry_function Thread entry point
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void));

/**
 * @brief Resume a suspended thread
 *
 * @param thread_id Thread identifier
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_thread_resume(int thread_id);

/**
 * @brief Suspend a thread
 *
 * @param thread_id Thread identifier
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_thread_suspend(int thread_id);

/**
 * @brief Relinquish to other threads at the same priority
 *
 * This is equivalent to a cooperative yield operation.
 */
void tm_thread_relinquish(void);

/**
 * @brief Sleep for the specified number of seconds
 *
 * @param seconds Number of seconds to sleep
 */
void tm_thread_sleep(int seconds);

/**
 * @brief Create a message queue
 *
 * Creates a queue capable of holding at least one 16-byte message.
 *
 * @param queue_id Queue identifier (0 to TM_MAX_QUEUES-1)
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_queue_create(int queue_id);

/**
 * @brief Send a 16-byte message to a queue
 *
 * @param queue_id    Queue identifier
 * @param message_ptr Pointer to 16-byte message (4 unsigned longs)
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_queue_send(int queue_id, unsigned long *message_ptr);

/**
 * @brief Receive a 16-byte message from a queue
 *
 * @param queue_id    Queue identifier
 * @param message_ptr Pointer to buffer for 16-byte message
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_queue_receive(int queue_id, unsigned long *message_ptr);

/**
 * @brief Create a binary semaphore
 *
 * Creates a semaphore with initial count of 1.
 *
 * @param semaphore_id Semaphore identifier (0 to TM_MAX_SEMAPHORES-1)
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_semaphore_create(int semaphore_id);

/**
 * @brief Get (take) a semaphore
 *
 * @param semaphore_id Semaphore identifier
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_semaphore_get(int semaphore_id);

/**
 * @brief Put (give) a semaphore
 *
 * @param semaphore_id Semaphore identifier
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_semaphore_put(int semaphore_id);

/**
 * @brief Create a memory pool
 *
 * Creates a memory pool capable of allocating at least one 128-byte block.
 *
 * @param pool_id Pool identifier (0 to TM_MAX_MEMORY_POOLS-1)
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_memory_pool_create(int pool_id);

/**
 * @brief Allocate a 128-byte block from a memory pool
 *
 * @param pool_id    Pool identifier
 * @param memory_ptr Pointer to store the allocated block address
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr);

/**
 * @brief Deallocate a block back to a memory pool
 *
 * @param pool_id    Pool identifier
 * @param memory_ptr Pointer to the block to deallocate
 * @return TM_SUCCESS on success, TM_ERROR on failure
 */
int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr);

/*******************************************************************************
 * Interrupt Handler Declarations
 *
 * These handlers must be called from the SVC exception handler.
 * They are defined in the respective test files.
 ******************************************************************************/

/**
 * @brief Interrupt handler for tm_interrupt_processing_test
 *
 * This handler increments a counter and posts a semaphore.
 */
void tm_interrupt_handler(void);

/**
 * @brief Interrupt handler for tm_interrupt_preemption_processing_test
 *
 * This handler increments a counter and resumes a higher-priority thread.
 */
void tm_interrupt_preemption_handler(void);

/*******************************************************************************
 * Test Entry Points
 *
 * Each test has its own main entry point (tm_main) and initialization function.
 ******************************************************************************/

/* Test initialization functions - implemented in each test file */
extern void tm_basic_processing_initialize(void);
extern void tm_cooperative_scheduling_initialize(void);
extern void tm_preemptive_scheduling_initialize(void);
extern void tm_interrupt_processing_initialize(void);
extern void tm_interrupt_preemption_processing_initialize(void);
extern void tm_message_processing_initialize(void);
extern void tm_synchronization_processing_initialize(void);
extern void tm_memory_allocation_initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* TM_ADAPTER_H */
