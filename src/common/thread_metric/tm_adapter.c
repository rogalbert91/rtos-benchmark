/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Thread-Metric Adapter Implementation for rtos-benchmark2
 *
 * This file implements the Thread-Metric API by mapping to the
 * rtos-benchmark2 bench_* API functions.
 *
 * The adapter handles:
 * - Thread entry function signature conversion (void(void) to void(void*))
 * - Priority mapping (Thread-Metric uses 1=highest, bench_api varies by RTOS)
 * - Memory pool implementation (Thread-Metric expects fixed-block pools)
 * - Sleep implementation using FreeRTOS vTaskDelay
 */

#include "tm_adapter.h"
#include "bench_api.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* FreeRTOS includes for sleep implementation */
#ifdef FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

/*******************************************************************************
 * Private Data
 ******************************************************************************/

/* Thread entry function storage (void(void) signature) */
static void (*tm_entry_functions[TM_MAX_THREADS])(void);

/* Thread creation tracking */
static int tm_thread_created[TM_MAX_THREADS];

/* Memory pool storage - simple fixed-block allocator */
static unsigned char tm_pool_memory[TM_MAX_MEMORY_POOLS][TM_POOL_SIZE];
static unsigned char *tm_pool_free_list[TM_MAX_MEMORY_POOLS][TM_BLOCKS_PER_POOL];
static int tm_pool_free_count[TM_MAX_MEMORY_POOLS];
static int tm_pool_initialized[TM_MAX_MEMORY_POOLS];

/* Queue tracking */
static int tm_queue_created[TM_MAX_QUEUES];

/* Semaphore tracking */
static int tm_semaphore_created[TM_MAX_SEMAPHORES];

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Thread wrapper to adapt void(void) to void(void*)
 *
 * Thread-Metric uses void(void) entry functions, but bench_api uses void(void*).
 * This wrapper retrieves the original entry function and calls it.
 */
static void tm_thread_wrapper(void *arg)
{
    int thread_id = (int)(uintptr_t)arg;

    if (thread_id >= 0 && thread_id < TM_MAX_THREADS &&
        tm_entry_functions[thread_id] != NULL) {
        /* Call the original Thread-Metric entry function */
        tm_entry_functions[thread_id]();
    }

    /* Thread should not return, but if it does, exit cleanly */
    bench_thread_exit();
}

/**
 * @brief Map Thread-Metric priority to bench_api priority
 *
 * Thread-Metric: 1 (highest) to 31 (lowest)
 * bench_api/FreeRTOS: The porting layer maps as:
 *   FreeRTOS priority = configMAX_PRIORITIES - bench_priority
 *
 * With configMAX_PRIORITIES=16, valid bench priorities are 1-15.
 *
 * Thread-Metric tests typically use priorities in the range 1-15,
 * so we can use a direct 1:1 mapping for these values. For priorities
 * above 15, we clamp to 15 (lowest usable priority).
 *
 * This preserves the relative ordering needed for preemptive tests
 * where adjacent priorities (like 6,7,8,9,10) must all be distinct.
 */
static int tm_map_priority(int tm_priority)
{
    /* Clamp Thread-Metric priority to valid range */
    if (tm_priority < 1) {
        return 1;
    } else if (tm_priority > 15) {
        return 15;
    }

    /* Direct 1:1 mapping for priorities 1-15 */
    return tm_priority;
}

/*******************************************************************************
 * Thread-Metric API Implementation
 ******************************************************************************/

void tm_initialize(void (*test_initialization_function)(void))
{
    /* Clear entry function array */
    memset(tm_entry_functions, 0, sizeof(tm_entry_functions));
    memset(tm_thread_created, 0, sizeof(tm_thread_created));
    memset(tm_queue_created, 0, sizeof(tm_queue_created));
    memset(tm_semaphore_created, 0, sizeof(tm_semaphore_created));
    memset(tm_pool_initialized, 0, sizeof(tm_pool_initialized));

    /* Call the test initialization function */
    if (test_initialization_function != NULL) {
        test_initialization_function();
    }
}

int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    char name[16];
    int result;

    if (thread_id < 0 || thread_id >= TM_MAX_THREADS) {
        return TM_ERROR;
    }

    if (entry_function == NULL) {
        return TM_ERROR;
    }

    /* Store the entry function for the wrapper */
    tm_entry_functions[thread_id] = entry_function;

    /* Create a unique thread name */
    snprintf(name, sizeof(name), "tm_thd_%d", thread_id);

    /* Create the thread using bench_api
     * Note: FreeRTOS starts threads immediately on creation, but the
     * Thread-Metric tests expect threads to start suspended and be
     * explicitly resumed.
     */
    result = bench_thread_create(thread_id, name, tm_map_priority(priority),
                                  tm_thread_wrapper, (void *)(uintptr_t)thread_id);

    if (result == BENCH_SUCCESS) {
        tm_thread_created[thread_id] = 1;

        /* Thread-Metric expects threads to be created in suspended state.
         * Suspend the thread immediately after creation so tm_thread_resume()
         * is required to start it.
         */
        bench_thread_suspend(thread_id);

        return TM_SUCCESS;
    }

    return TM_ERROR;
}

int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_MAX_THREADS) {
        return TM_ERROR;
    }

    if (!tm_thread_created[thread_id]) {
        return TM_ERROR;
    }

    bench_thread_resume(thread_id);
    return TM_SUCCESS;
}

int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_MAX_THREADS) {
        return TM_ERROR;
    }

    if (!tm_thread_created[thread_id]) {
        return TM_ERROR;
    }

    bench_thread_suspend(thread_id);
    return TM_SUCCESS;
}

void tm_thread_relinquish(void)
{
    bench_yield();
}

void tm_thread_sleep(int seconds)
{
#ifdef FREERTOS
    /* FreeRTOS: Use vTaskDelay with tick rate conversion */
    vTaskDelay(pdMS_TO_TICKS((uint32_t)seconds * 1000));
#elif defined(ZEPHYR)
    /* Zephyr: Use k_sleep */
    extern void k_sleep(int32_t ms);
    k_sleep(seconds * 1000);
#else
    /* Fallback: Use a busy-wait loop (not recommended for production)
     * This is only for platforms without a proper sleep mechanism.
     */
    volatile uint32_t i;
    uint32_t loops = (uint32_t)seconds * 10000000;
    for (i = 0; i < loops; i++) {
        __asm volatile("nop");
    }
#endif
}

int tm_queue_create(int queue_id)
{
    int result;

    if (queue_id < 0 || queue_id >= TM_MAX_QUEUES) {
        return TM_ERROR;
    }

    /* Create a queue for 16-byte messages (4 x unsigned long)
     * Capacity of 10 messages should be sufficient for Thread-Metric tests
     */
    result = bench_message_queue_create(queue_id, "tm_queue",
                                         10,  /* max number of messages */
                                         TM_QUEUE_MSG_SIZE);

    if (result == BENCH_SUCCESS) {
        tm_queue_created[queue_id] = 1;
        return TM_SUCCESS;
    }

    return TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_MAX_QUEUES) {
        return TM_ERROR;
    }

    if (!tm_queue_created[queue_id]) {
        return TM_ERROR;
    }

    return bench_message_queue_send(queue_id, (char *)message_ptr,
                                     TM_QUEUE_MSG_SIZE);
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_MAX_QUEUES) {
        return TM_ERROR;
    }

    if (!tm_queue_created[queue_id]) {
        return TM_ERROR;
    }

    return bench_message_queue_receive(queue_id, (char *)message_ptr,
                                        TM_QUEUE_MSG_SIZE);
}

int tm_semaphore_create(int semaphore_id)
{
    int result;

    if (semaphore_id < 0 || semaphore_id >= TM_MAX_SEMAPHORES) {
        return TM_ERROR;
    }

    /* Thread-Metric expects binary semaphore with initial count = 1 */
    result = bench_sem_create(semaphore_id, 1, 1);

    if (result == BENCH_SUCCESS) {
        tm_semaphore_created[semaphore_id] = 1;
        return TM_SUCCESS;
    }

    return TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_MAX_SEMAPHORES) {
        return TM_ERROR;
    }

    if (!tm_semaphore_created[semaphore_id]) {
        return TM_ERROR;
    }

    return bench_sem_take(semaphore_id);
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_MAX_SEMAPHORES) {
        return TM_ERROR;
    }

    if (!tm_semaphore_created[semaphore_id]) {
        return TM_ERROR;
    }

    bench_sem_give(semaphore_id);
    return TM_SUCCESS;
}

int tm_memory_pool_create(int pool_id)
{
    int i;
    unsigned char *block_ptr;

    if (pool_id < 0 || pool_id >= TM_MAX_MEMORY_POOLS) {
        return TM_ERROR;
    }

    /* Initialize the free list with all blocks */
    block_ptr = tm_pool_memory[pool_id];
    for (i = 0; i < TM_BLOCKS_PER_POOL; i++) {
        tm_pool_free_list[pool_id][i] = block_ptr;
        block_ptr += TM_BLOCK_SIZE;
    }

    tm_pool_free_count[pool_id] = TM_BLOCKS_PER_POOL;
    tm_pool_initialized[pool_id] = 1;

    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_MAX_MEMORY_POOLS) {
        return TM_ERROR;
    }

    if (!tm_pool_initialized[pool_id]) {
        return TM_ERROR;
    }

    if (memory_ptr == NULL) {
        return TM_ERROR;
    }

    if (tm_pool_free_count[pool_id] <= 0) {
        return TM_ERROR;
    }

    /* Get a block from the free list */
    tm_pool_free_count[pool_id]--;
    *memory_ptr = tm_pool_free_list[pool_id][tm_pool_free_count[pool_id]];

    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_MAX_MEMORY_POOLS) {
        return TM_ERROR;
    }

    if (!tm_pool_initialized[pool_id]) {
        return TM_ERROR;
    }

    if (memory_ptr == NULL) {
        return TM_ERROR;
    }

    if (tm_pool_free_count[pool_id] >= TM_BLOCKS_PER_POOL) {
        return TM_ERROR;  /* Pool is already full */
    }

    /* Return block to the free list */
    tm_pool_free_list[pool_id][tm_pool_free_count[pool_id]] = memory_ptr;
    tm_pool_free_count[pool_id]++;

    return TM_SUCCESS;
}

/*******************************************************************************
 * Default Interrupt Handlers (weak symbols)
 *
 * These are provided as weak symbols so they can be overridden by the
 * actual test implementations when those tests are compiled in.
 ******************************************************************************/

__attribute__((weak))
void tm_interrupt_handler(void)
{
    /* Default: do nothing */
}

__attribute__((weak))
void tm_interrupt_preemption_handler(void)
{
    /* Default: do nothing */
}
