/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Thread-Metric Test Suite Runner
 *
 * This file provides the entry point for running the Thread-Metric benchmark
 * suite. It can run individual tests or all tests sequentially.
 *
 * Thread-Metric tests run for TM_TEST_DURATION seconds each (default 30s).
 * Running all 8 tests takes approximately 4 minutes.
 *
 * Build options:
 *   -DENABLE_THREAD_METRIC=ON          Enable Thread-Metric tests
 *   -DTM_TEST_DURATION=30              Set test duration (seconds)
 *   -DTM_TEST=<test_name>              Run specific test only
 *
 * Available test names:
 *   BASIC_PROCESSING
 *   COOPERATIVE_SCHEDULING
 *   PREEMPTIVE_SCHEDULING
 *   INTERRUPT_PROCESSING
 *   INTERRUPT_PREEMPTION
 *   MESSAGE_PROCESSING
 *   SYNCHRONIZATION
 *   MEMORY_ALLOCATION
 */

#include "tm_adapter.h"
#include "bench_api.h"

/*******************************************************************************
 * Test Selection
 *
 * By default, run a subset of tests that are most relevant for comparing
 * RTOS performance. The interrupt tests are disabled by default because
 * they require SVC handler modifications.
 ******************************************************************************/

/* Default: run these tests */
#ifndef TM_SKIP_BASIC_PROCESSING
#define TM_RUN_BASIC_PROCESSING         1
#endif

#ifndef TM_SKIP_COOPERATIVE_SCHEDULING
#define TM_RUN_COOPERATIVE_SCHEDULING   1
#endif

#ifndef TM_SKIP_PREEMPTIVE_SCHEDULING
#define TM_RUN_PREEMPTIVE_SCHEDULING    1
#endif

#ifndef TM_SKIP_MESSAGE_PROCESSING
#define TM_RUN_MESSAGE_PROCESSING       1
#endif

#ifndef TM_SKIP_SYNCHRONIZATION
#define TM_RUN_SYNCHRONIZATION          1
#endif

#ifndef TM_SKIP_MEMORY_ALLOCATION
#define TM_RUN_MEMORY_ALLOCATION        1
#endif

/* Default: skip interrupt tests (require SVC handler modifications) */
#ifndef TM_RUN_INTERRUPT_PROCESSING
#define TM_RUN_INTERRUPT_PROCESSING     0
#endif

#ifndef TM_RUN_INTERRUPT_PREEMPTION
#define TM_RUN_INTERRUPT_PREEMPTION     0
#endif

/*******************************************************************************
 * Test Table
 ******************************************************************************/

typedef struct {
    const char *name;
    void (*init_func)(void);
    int enabled;
} tm_test_entry_t;

static const tm_test_entry_t tm_tests[] = {
#if TM_RUN_BASIC_PROCESSING
    {"Basic Processing",        tm_basic_processing_initialize,              1},
#endif
#if TM_RUN_COOPERATIVE_SCHEDULING
    {"Cooperative Scheduling",  tm_cooperative_scheduling_initialize,        1},
#endif
#if TM_RUN_PREEMPTIVE_SCHEDULING
    {"Preemptive Scheduling",   tm_preemptive_scheduling_initialize,         1},
#endif
#if TM_RUN_INTERRUPT_PROCESSING
    {"Interrupt Processing",    tm_interrupt_processing_initialize,          1},
#endif
#if TM_RUN_INTERRUPT_PREEMPTION
    {"Interrupt Preemption",    tm_interrupt_preemption_processing_initialize, 1},
#endif
#if TM_RUN_MESSAGE_PROCESSING
    {"Message Processing",      tm_message_processing_initialize,            1},
#endif
#if TM_RUN_SYNCHRONIZATION
    {"Synchronization",         tm_synchronization_processing_initialize,    1},
#endif
#if TM_RUN_MEMORY_ALLOCATION
    {"Memory Allocation",       tm_memory_allocation_initialize,             1},
#endif
};

#define NUM_TM_TESTS (sizeof(tm_tests) / sizeof(tm_tests[0]))

/*******************************************************************************
 * Single Test Runners
 *
 * These functions run individual tests and are used when building with
 * a specific -DTM_TEST=<name> option.
 ******************************************************************************/

/*
 * Helper macro to properly end a test runner task.
 * FreeRTOS tasks must not return from their entry function.
 */
#define TM_SUSPEND_INIT_TASK() do { \
    extern void vTaskSuspend(void *); \
    vTaskSuspend(NULL); \
    for (;;) {} \
} while(0)

#ifdef TM_TEST_BASIC_PROCESSING
void tm_run_test(void *arg)
{
    (void)arg;
    PRINTF("\n*** Thread-Metric Basic Processing Test ***\n");
    PRINTF("Test Duration: %d seconds\n\n", TM_TEST_DURATION);
    tm_initialize(tm_basic_processing_initialize);
    TM_SUSPEND_INIT_TASK();
}
#endif

#ifdef TM_TEST_COOPERATIVE_SCHEDULING
void tm_run_test(void *arg)
{
    (void)arg;
    PRINTF("\n*** Thread-Metric Cooperative Scheduling Test ***\n");
    PRINTF("Test Duration: %d seconds\n\n", TM_TEST_DURATION);
    tm_initialize(tm_cooperative_scheduling_initialize);
    TM_SUSPEND_INIT_TASK();
}
#endif

#ifdef TM_TEST_PREEMPTIVE_SCHEDULING
void tm_run_test(void *arg)
{
    (void)arg;
    PRINTF("\n*** Thread-Metric Preemptive Scheduling Test ***\n");
    PRINTF("Test Duration: %d seconds\n\n", TM_TEST_DURATION);
    tm_initialize(tm_preemptive_scheduling_initialize);
    TM_SUSPEND_INIT_TASK();
}
#endif

#ifdef TM_TEST_MESSAGE_PROCESSING
void tm_run_test(void *arg)
{
    (void)arg;
    PRINTF("\n*** Thread-Metric Message Processing Test ***\n");
    PRINTF("Test Duration: %d seconds\n\n", TM_TEST_DURATION);
    tm_initialize(tm_message_processing_initialize);
    TM_SUSPEND_INIT_TASK();
}
#endif

#ifdef TM_TEST_SYNCHRONIZATION
void tm_run_test(void *arg)
{
    (void)arg;
    PRINTF("\n*** Thread-Metric Synchronization Processing Test ***\n");
    PRINTF("Test Duration: %d seconds\n\n", TM_TEST_DURATION);
    tm_initialize(tm_synchronization_processing_initialize);
    TM_SUSPEND_INIT_TASK();
}
#endif

#ifdef TM_TEST_MEMORY_ALLOCATION
void tm_run_test(void *arg)
{
    (void)arg;
    PRINTF("\n*** Thread-Metric Memory Allocation Test ***\n");
    PRINTF("Test Duration: %d seconds\n\n", TM_TEST_DURATION);
    tm_initialize(tm_memory_allocation_initialize);
    TM_SUSPEND_INIT_TASK();
}
#endif

/*******************************************************************************
 * All Tests Runner
 *
 * This runs all enabled tests sequentially. Note that Thread-Metric tests
 * run indefinitely (reporting every TM_TEST_DURATION seconds), so this
 * runner will start the first test and never proceed to the next.
 *
 * For running multiple tests, build and run each test separately.
 ******************************************************************************/

#if defined(TM_RUN_ALL_TESTS) || (!defined(TM_TEST_BASIC_PROCESSING) && \
    !defined(TM_TEST_COOPERATIVE_SCHEDULING) && \
    !defined(TM_TEST_PREEMPTIVE_SCHEDULING) && \
    !defined(TM_TEST_MESSAGE_PROCESSING) && \
    !defined(TM_TEST_SYNCHRONIZATION) && \
    !defined(TM_TEST_MEMORY_ALLOCATION))

void tm_run_test(void *arg)
{
    (void)arg;

    PRINTF("\n");
    PRINTF("==========================================================\n");
    PRINTF("           Thread-Metric Benchmark Suite\n");
    PRINTF("==========================================================\n");
    PRINTF("\n");
    PRINTF("Test Duration: %d seconds per test\n", TM_TEST_DURATION);
    PRINTF("Number of tests: %d\n", (int)NUM_TM_TESTS);
    PRINTF("\n");
    PRINTF("NOTE: Thread-Metric tests run continuously, reporting every\n");
    PRINTF("      %d seconds. Press RESET to stop.\n", TM_TEST_DURATION);
    PRINTF("\n");

    if (NUM_TM_TESTS > 0) {
        PRINTF("Starting: %s\n", tm_tests[0].name);
        PRINTF("----------------------------------------------------------\n\n");
        tm_initialize(tm_tests[0].init_func);
    } else {
        PRINTF("ERROR: No tests enabled!\n");
    }

    /* 
     * Thread-Metric tests create threads that run forever.
     * This init task must not return (FreeRTOS tasks can't return).
     * Suspend ourselves to let the test threads run.
     */
    #ifdef FREERTOS
    extern void vTaskSuspend(void *);
    vTaskSuspend(NULL);  /* Suspend current task forever */
    #endif

    /* Should never reach here */
    for (;;) {
        /* Loop forever as fallback */
    }
}

#endif /* TM_RUN_ALL_TESTS */

/*******************************************************************************
 * Main Entry Point
 ******************************************************************************/

#ifdef RUN_THREAD_METRIC
int main(void)
{
    bench_test_init(tm_run_test);
    return 0;
}
#endif
