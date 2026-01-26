# ThreadX Thread-Metric Benchmark Suite Integration Guide

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Thread-Metric Overview](#thread-metric-overview)
3. [Test Suite Specifications](#test-suite-specifications)
4. [Architecture Analysis](#architecture-analysis)
5. [API Mapping: Thread-Metric to rtos-benchmark2](#api-mapping-thread-metric-to-rtos-benchmark2)
6. [Integration Strategy](#integration-strategy)
7. [Step-by-Step Integration Guide](#step-by-step-integration-guide)
8. [Implementation Details](#implementation-details)
9. [Testing and Validation](#testing-and-validation)
10. [Appendix](#appendix)

---

## Executive Summary

This document provides a comprehensive guide for integrating the **ThreadX Thread-Metric** benchmark suite into the **rtos-benchmark2** project. Thread-Metric is a widely-used RTOS benchmarking standard that measures throughput-based metrics over fixed time intervals, complementing the existing cycle-accurate timing measurements in rtos-benchmark2.

### Key Differences Between Suites

| Aspect | Thread-Metric | rtos-benchmark2 |
|--------|---------------|-----------------|
| **Measurement Type** | Throughput (operations/time) | Latency (cycles/operation) |
| **Time Window** | 30-second intervals | Per-operation cycle count |
| **Primary Metric** | Total operations count | avg/min/max nanoseconds |
| **Test Focus** | Sustained performance | Individual operation timing |
| **Porting Layer** | `tm_*` API functions | `bench_*` API functions |

### Integration Value

Adding Thread-Metric to rtos-benchmark2 provides:
- Industry-standard benchmark compatibility
- Throughput metrics complementing latency measurements
- Ability to compare results with published ThreadX benchmarks
- Additional test scenarios (preemptive scheduling chain, cooperative round-robin)

---

## Thread-Metric Overview

### Source Location

```
threadx/utility/benchmarks/thread_metric/
├── tm_api.h                                    # API definitions
├── tm_porting_layer.h                          # Platform-specific configuration
├── tm_porting_layer_template.c                 # Porting template
├── tm_basic_processing_test.c                  # Baseline processing test
├── tm_cooperative_scheduling_test.c            # Cooperative scheduling test
├── tm_preemptive_scheduling_test.c             # Preemptive scheduling test
├── tm_interrupt_processing_test.c              # Interrupt processing test
├── tm_interrupt_preemption_processing_test.c   # Interrupt with preemption test
├── tm_message_processing_test.c                # Message queue test
├── tm_synchronization_processing_test.c        # Semaphore test
├── tm_memory_allocation_test.c                 # Memory allocation test
└── threadx_example/
    ├── tm_porting_layer_threadx.c              # ThreadX reference implementation
    └── threadx_tm_*_example.c                  # ThreadX-specific examples
```

### License

```
MIT License
Copyright (c) 2024 Microsoft Corporation
```

The MIT license is compatible with rtos-benchmark2's Apache 2.0 license.

### Design Philosophy

Thread-Metric measures **how many RTOS operations can be performed in a fixed time period** (default 30 seconds). This is the inverse of rtos-benchmark2's approach which measures **how long each operation takes**.

```
Thread-Metric:    operations / time  →  throughput score
rtos-benchmark2:  time / operation   →  latency measurement
```

---

## Test Suite Specifications

### Test 1: Basic Processing Test

**Purpose:** Establish baseline processing capability independent of RTOS overhead.

**Implementation:**
- Single thread at priority 10
- Performs array manipulation operations
- Reference point for normalizing other test results

**Mechanism:**
```c
while(1) {
    for (i = 0; i < 1024; i++) {
        tm_basic_processing_array[i] = 
            (tm_basic_processing_array[i] + counter) ^ tm_basic_processing_array[i];
    }
    counter++;
}
```

**rtos-benchmark2 Equivalent:** No direct equivalent (new test type)

---

### Test 2: Cooperative Scheduling Test

**Purpose:** Measure voluntary context switch throughput with round-robin scheduling.

**Implementation:**
- 5 threads at the same priority (priority 3)
- Each thread yields to the next via `tm_thread_relinquish()`
- Validates balanced execution (all counters within ±1 of average)

**Thread Structure:**
```
Thread 0 → relinquish → Thread 1 → relinquish → Thread 2 → ... → Thread 0
```

**rtos-benchmark2 Equivalent:** `bench_thread_switch_yield_test.c` (measures latency, not throughput)

---

### Test 3: Preemptive Scheduling Test

**Purpose:** Measure preemption chain throughput with priority-based scheduling.

**Implementation:**
- 5 threads at different priorities (6, 7, 8, 9, 10)
- Lowest priority thread (10) starts the chain by resuming thread 9
- Each thread resumes the next higher priority, causing immediate preemption
- After highest priority executes, threads unwind via self-suspend

**Execution Flow:**
```
Thread 0 (P10): resume(1) → preempted
  Thread 1 (P9): resume(2) → preempted
    Thread 2 (P8): resume(3) → preempted
      Thread 3 (P7): resume(4) → preempted
        Thread 4 (P6): increment, suspend(4) → returns to Thread 3
      Thread 3: increment, suspend(3) → returns to Thread 2
    Thread 2: increment, suspend(2) → returns to Thread 1
  Thread 1: increment, suspend(1) → returns to Thread 0
Thread 0: increment, repeat
```

**rtos-benchmark2 Equivalent:** Partial coverage in thread and semaphore tests

---

### Test 4: Interrupt Processing Test (No Preemption)

**Purpose:** Measure interrupt-to-semaphore throughput without thread preemption.

**Implementation:**
- Single thread triggers software interrupt via `TM_CAUSE_INTERRUPT`
- ISR increments counter and posts semaphore
- Thread waits for semaphore, increments counter, triggers next interrupt

**Interrupt Trigger:**
```c
// Defined in tm_porting_layer.h for Cortex-M
#define TM_CAUSE_INTERRUPT    asm("SVC #0");
```

**ISR Requirement:**
```c
void SVC_Handler(void) {
    PUSH    {lr}
    BL      tm_interrupt_handler
    POP     {lr}
    BX      LR
}
```

**rtos-benchmark2 Equivalent:** `bench_interrupt_latency_test.c` (different measurement approach)

---

### Test 5: Interrupt Preemption Processing Test

**Purpose:** Measure interrupt-to-thread-preemption throughput.

**Implementation:**
- Thread 1 (priority 10) triggers software interrupt
- ISR resumes Thread 0 (priority 3), causing preemption
- Thread 0 runs, increments counter, suspends itself
- Control returns to Thread 1

**rtos-benchmark2 Equivalent:** `bench_sem_context_switch_test.c` (similar pattern)

---

### Test 6: Message Processing Test

**Purpose:** Measure message queue send/receive throughput.

**Implementation:**
- Single thread sends and receives 16-byte messages
- Messages contain incrementing pattern for validation

**Message Structure:**
```c
unsigned long tm_message_sent[4] = {
    0x11112222, 0x33334444, 0x55556666, 0x77778888
};
```

**rtos-benchmark2 Equivalent:** `bench_message_queue_test.c`

---

### Test 7: Synchronization Processing Test

**Purpose:** Measure semaphore get/put throughput.

**Implementation:**
- Single thread acquires and releases semaphore repeatedly

**rtos-benchmark2 Equivalent:** `bench_sem_signal_release_test.c`

---

### Test 8: Memory Allocation Test

**Purpose:** Measure memory pool allocate/deallocate throughput.

**Implementation:**
- Single thread allocates and frees 128-byte blocks

**rtos-benchmark2 Equivalent:** `bench_malloc_free_test.c`

---

## Architecture Analysis

### Thread-Metric Porting Layer API

```c
// System initialization
void  tm_initialize(void (*test_initialization_function)(void));

// Thread management (priorities: 1=highest, 31=lowest)
int   tm_thread_create(int thread_id, int priority, void (*entry_function)(void));
int   tm_thread_resume(int thread_id);
int   tm_thread_suspend(int thread_id);
void  tm_thread_relinquish(void);
void  tm_thread_sleep(int seconds);

// Message queues (16-byte messages)
int   tm_queue_create(int queue_id);
int   tm_queue_send(int queue_id, unsigned long *message_ptr);
int   tm_queue_receive(int queue_id, unsigned long *message_ptr);

// Semaphores (binary)
int   tm_semaphore_create(int semaphore_id);
int   tm_semaphore_get(int semaphore_id);
int   tm_semaphore_put(int semaphore_id);

// Memory pools (128-byte blocks)
int   tm_memory_pool_create(int pool_id);
int   tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr);
int   tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr);
```

### rtos-benchmark2 API (subset relevant for mapping)

```c
// System initialization
void bench_test_init(void (*test_init_function)(void *));

// Thread management
int  bench_thread_create(int thread_id, const char *name, int priority,
                         void (*entry_function)(void *), void *args);
int  bench_thread_spawn(int thread_id, const char *name, int priority,
                        void (*entry_function)(void *), void *args);
void bench_thread_start(int thread_id);
void bench_thread_resume(int thread_id);
void bench_thread_suspend(int thread_id);
void bench_yield(void);

// Semaphores
int  bench_sem_create(int sem_id, int initial_count, int maximum_count);
void bench_sem_give(int sem_id);
int  bench_sem_take(int sem_id);

// Message queues
int  bench_message_queue_create(int mq_id, const char *name,
                                size_t msg_max_num, size_t msg_max_len);
int  bench_message_queue_send(int mq_id, char *msg_ptr, size_t msg_len);
int  bench_message_queue_receive(int mq_id, char *msg_ptr, size_t msg_len);

// Memory (heap-based, not pool-based)
void *bench_malloc(size_t size);
void  bench_free(void *ptr);
```

---

## API Mapping: Thread-Metric to rtos-benchmark2

### Direct Mappings

| Thread-Metric API | rtos-benchmark2 API | Notes |
|-------------------|---------------------|-------|
| `tm_initialize()` | `bench_test_init()` | Entry signature differs |
| `tm_thread_create()` | `bench_thread_create()` | Add name, args params |
| `tm_thread_resume()` | `bench_thread_resume()` | Direct mapping |
| `tm_thread_suspend()` | `bench_thread_suspend()` | Direct mapping |
| `tm_thread_relinquish()` | `bench_yield()` | Direct mapping |
| `tm_semaphore_create()` | `bench_sem_create()` | Add count parameters |
| `tm_semaphore_get()` | `bench_sem_take()` | Direct mapping |
| `tm_semaphore_put()` | `bench_sem_give()` | Direct mapping |
| `tm_queue_create()` | `bench_message_queue_create()` | Add size parameters |
| `tm_queue_send()` | `bench_message_queue_send()` | Message format differs |
| `tm_queue_receive()` | `bench_message_queue_receive()` | Message format differs |

### Missing APIs (Need Implementation)

| Thread-Metric API | Required Addition |
|-------------------|-------------------|
| `tm_thread_sleep()` | Blocking sleep for N seconds |
| `tm_memory_pool_create()` | Fixed-block memory pool |
| `tm_memory_pool_allocate()` | Block allocation (128 bytes) |
| `tm_memory_pool_deallocate()` | Block deallocation |

### Priority Mapping

Thread-Metric uses priorities 1-31 where 1 is highest. The mapping to FreeRTOS must preserve uniqueness for adjacent priorities (critical for preemptive scheduling tests).

```c
// Thread-Metric: 1 (highest) to 31 (lowest)
// bench_api: 1 (highest) to 15 (lowest) - porting layer inverts for FreeRTOS
// FreeRTOS native: 0 (lowest) to configMAX_PRIORITIES-1 (highest)

// CORRECT: Direct 1:1 mapping for priorities 1-15
int tm_map_priority(int tm_priority) {
    if (tm_priority < 1) return 1;
    if (tm_priority > 15) return 15;
    return tm_priority;  // Direct mapping
}

// WRONG: This causes priority collisions!
// int tm_map_priority(int tm_priority) {
//     return (tm_priority + 1) / 2;  // TM 9,10 both → bench 5!
// }
```

**Priority Flow Example (Preemptive Test):**
```
Thread-Metric    →    bench_api    →    FreeRTOS (configMAX_PRIORITIES=16)
TM priority 2   →    bench 2      →    FreeRTOS 14 (highest)
TM priority 6   →    bench 6      →    FreeRTOS 10
TM priority 7   →    bench 7      →    FreeRTOS 9
TM priority 8   →    bench 8      →    FreeRTOS 8
TM priority 9   →    bench 9      →    FreeRTOS 7
TM priority 10  →    bench 10     →    FreeRTOS 6 (lowest in chain)
```

---

## Integration Strategy

### Option A: Adapter Layer (Recommended)

Create a thin adapter that translates Thread-Metric API calls to rtos-benchmark2 API calls.

**Advantages:**
- Minimal changes to existing code
- Tests remain portable
- Easy to maintain

**Directory Structure:**
```
rtos-benchmark2/
├── src/
│   ├── common/
│   │   └── thread_metric/           # NEW: Thread-Metric tests
│   │       ├── tm_adapter.c         # API adapter layer
│   │       ├── tm_adapter.h         # Adapter declarations
│   │       ├── tm_basic_processing_test.c
│   │       ├── tm_cooperative_scheduling_test.c
│   │       ├── tm_preemptive_scheduling_test.c
│   │       ├── tm_interrupt_processing_test.c
│   │       ├── tm_interrupt_preemption_processing_test.c
│   │       ├── tm_message_processing_test.c
│   │       ├── tm_synchronization_processing_test.c
│   │       └── tm_memory_allocation_test.c
│   └── <rtos>/
│       └── tm_porting_layer_<rtos>.c  # RTOS-specific extensions
```

### Option B: Native Rewrite

Rewrite Thread-Metric tests using rtos-benchmark2 API directly.

**Advantages:**
- Consistent with existing tests
- No adapter overhead

**Disadvantages:**
- More work
- Results may not be directly comparable to published Thread-Metric results

---

## Step-by-Step Integration Guide

### Step 1: Create Directory Structure

```bash
cd /home/alberto_rodriguez/Code/rtos-benchmark2
mkdir -p src/common/thread_metric
```

### Step 2: Copy Thread-Metric Source Files

```bash
# Copy test files (modify includes)
cp /home/alberto_rodriguez/Code/threadx/utility/benchmarks/thread_metric/tm_*.c \
   src/common/thread_metric/

# Copy header files
cp /home/alberto_rodriguez/Code/threadx/utility/benchmarks/thread_metric/tm_api.h \
   src/common/thread_metric/
```

### Step 3: Create Adapter Header (`tm_adapter.h`)

```c
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef TM_ADAPTER_H
#define TM_ADAPTER_H

#include "bench_api.h"

/* Thread-Metric configuration */
#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION    30  /* Test duration in seconds */
#endif

/* Return codes */
#define TM_SUCCESS  BENCH_SUCCESS
#define TM_ERROR    BENCH_ERROR

/* Interrupt trigger macro - architecture specific */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
#define TM_CAUSE_INTERRUPT    __asm volatile("SVC #0")
#else
#error "TM_CAUSE_INTERRUPT not defined for this architecture"
#endif

/* Thread-Metric API declarations - implemented in tm_adapter.c */
void  tm_initialize(void (*test_initialization_function)(void));
int   tm_thread_create(int thread_id, int priority, void (*entry_function)(void));
int   tm_thread_resume(int thread_id);
int   tm_thread_suspend(int thread_id);
void  tm_thread_relinquish(void);
void  tm_thread_sleep(int seconds);
int   tm_queue_create(int queue_id);
int   tm_queue_send(int queue_id, unsigned long *message_ptr);
int   tm_queue_receive(int queue_id, unsigned long *message_ptr);
int   tm_semaphore_create(int semaphore_id);
int   tm_semaphore_get(int semaphore_id);
int   tm_semaphore_put(int semaphore_id);
int   tm_memory_pool_create(int pool_id);
int   tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr);
int   tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr);

/* Interrupt handlers - must be implemented per RTOS */
void  tm_interrupt_handler(void);
void  tm_interrupt_preemption_handler(void);

#endif /* TM_ADAPTER_H */
```

### Step 4: Create Adapter Implementation (`tm_adapter.c`)

```c
/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Thread-Metric Adapter Implementation for rtos-benchmark2
 *
 * CRITICAL IMPLEMENTATION NOTES:
 * 1. All counter variables in tests MUST be declared 'volatile'
 * 2. FreeRTOS tasks must NEVER return - use vTaskSuspend(NULL)
 * 3. Priority mapping must preserve uniqueness for adjacent priorities
 * 4. Threads must be suspended immediately after creation
 */

#include "tm_adapter.h"
#include "bench_api.h"
#include <string.h>
#include <stdint.h>

/* FreeRTOS includes for sleep implementation */
#ifdef FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

/* Configuration constants */
#define TM_MAX_THREADS      10
#define TM_MAX_QUEUES       2
#define TM_MAX_SEMAPHORES   5
#define TM_MAX_POOLS        1
#define TM_BLOCK_SIZE       128
#define TM_POOL_SIZE        2048
#define TM_QUEUE_MSG_SIZE   16  /* 4 x unsigned long */

/* Thread entry function storage */
static void (*tm_entry_functions[TM_MAX_THREADS])(void);
static int tm_thread_created[TM_MAX_THREADS];

/* Memory pool storage (simple implementation) */
static unsigned char tm_pool_memory[TM_MAX_POOLS][TM_POOL_SIZE];
static unsigned char *tm_pool_free_list[TM_MAX_POOLS][TM_POOL_SIZE / TM_BLOCK_SIZE];
static int tm_pool_free_count[TM_MAX_POOLS];
static int tm_pool_initialized[TM_MAX_POOLS];

/* Tracking arrays */
static int tm_queue_created[TM_MAX_QUEUES];
static int tm_semaphore_created[TM_MAX_SEMAPHORES];

/*
 * Thread wrapper to adapt void(void) to void(void*)
 * Thread-Metric uses void(void) entry functions, but bench_api uses void(void*).
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

/*
 * Map Thread-Metric priority (1=highest, 31=lowest) to bench_api priority
 *
 * CRITICAL: Use direct 1:1 mapping for priorities 1-15 to preserve
 * the relative ordering needed for preemptive scheduling tests.
 * The old mapping ((priority + 1) / 2) caused collisions that broke
 * the preemptive chain.
 */
static int tm_map_priority(int tm_priority)
{
    if (tm_priority < 1) {
        return 1;
    } else if (tm_priority > 15) {
        return 15;
    }
    /* Direct 1:1 mapping for priorities 1-15 */
    return tm_priority;
}

void tm_initialize(void (*test_initialization_function)(void))
{
    /* Clear all tracking arrays */
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
     * Note: FreeRTOS starts threads immediately on creation, but
     * Thread-Metric expects threads to start suspended and be
     * explicitly resumed.
     */
    result = bench_thread_create(thread_id, name, tm_map_priority(priority),
                                  tm_thread_wrapper, (void *)(uintptr_t)thread_id);

    if (result == BENCH_SUCCESS) {
        tm_thread_created[thread_id] = 1;

        /* CRITICAL: Thread-Metric expects threads to be created in suspended state.
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
    vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
    #elif defined(ZEPHYR)
    k_sleep(K_SECONDS(seconds));
    #else
    /* Fallback: busy wait (not recommended) */
    volatile uint32_t i;
    for (i = 0; i < seconds * 10000000; i++);
    #endif
}

int tm_queue_create(int queue_id)
{
    int result;
    if (queue_id < 0 || queue_id >= TM_MAX_QUEUES) {
        return TM_ERROR;
    }
    result = bench_message_queue_create(queue_id, "tm_queue", 
                                        1, TM_QUEUE_MSG_SIZE);
    if (result == BENCH_SUCCESS) {
        tm_queue_created[queue_id] = 1;
    }
    return result;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    return bench_message_queue_send(queue_id, (char *)message_ptr, 
                                    TM_QUEUE_MSG_SIZE);
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    return bench_message_queue_receive(queue_id, (char *)message_ptr,
                                       TM_QUEUE_MSG_SIZE);
}

int tm_semaphore_create(int semaphore_id)
{
    int result;
    if (semaphore_id < 0 || semaphore_id >= TM_MAX_SEMAPHORES) {
        return TM_ERROR;
    }
    /* Thread-Metric expects binary semaphore, initial count = 1 */
    result = bench_sem_create(semaphore_id, 1, 1);
    if (result == BENCH_SUCCESS) {
        tm_semaphore_created[semaphore_id] = 1;
    }
    return result;
}

int tm_semaphore_get(int semaphore_id)
{
    return bench_sem_take(semaphore_id);
}

int tm_semaphore_put(int semaphore_id)
{
    bench_sem_give(semaphore_id);
    return TM_SUCCESS;
}

/* Simple fixed-block memory pool implementation */
int tm_memory_pool_create(int pool_id)
{
    int i;
    int num_blocks;
    
    if (pool_id < 0 || pool_id >= TM_MAX_POOLS) {
        return TM_ERROR;
    }
    
    num_blocks = TM_POOL_SIZE / TM_BLOCK_SIZE;
    
    /* Initialize free list with pointers to each block */
    for (i = 0; i < num_blocks; i++) {
        tm_pool_free_list[pool_id][i] = &tm_pool_memory[pool_id][i * TM_BLOCK_SIZE];
    }
    
    tm_pool_free_count[pool_id] = num_blocks;
    tm_pool_initialized[pool_id] = 1;
    
    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_MAX_POOLS || !tm_pool_initialized[pool_id]) {
        return TM_ERROR;
    }
    
    if (tm_pool_free_count[pool_id] > 0) {
        tm_pool_free_count[pool_id]--;
        *memory_ptr = tm_pool_free_list[pool_id][tm_pool_free_count[pool_id]];
        return TM_SUCCESS;
    }
    
    return TM_ERROR;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    int max_blocks;
    
    if (pool_id < 0 || pool_id >= TM_MAX_POOLS || !tm_pool_initialized[pool_id]) {
        return TM_ERROR;
    }
    
    max_blocks = TM_POOL_SIZE / TM_BLOCK_SIZE;
    
    if (tm_pool_free_count[pool_id] < max_blocks) {
        tm_pool_free_list[pool_id][tm_pool_free_count[pool_id]] = memory_ptr;
        tm_pool_free_count[pool_id]++;
        return TM_SUCCESS;
    }
    
    return TM_ERROR;
}
```

### Step 5: Modify Thread-Metric Test Files

Update the includes in each test file:

```c
/* Old include */
// #include "tm_api.h"

/* New include */
#include "tm_adapter.h"
```

Remove the `tm_main()` function from each test or wrap it:

```c
#ifdef RUN_TM_BASIC_PROCESSING
int main(void)
{
    PRINTF("\n\r *** Thread-Metric Basic Processing Test ***\n\n\r");
    bench_test_init((void (*)(void *))tm_basic_processing_initialize);
    return 0;
}
#endif
```

### Step 6: Update CMakeLists.txt

Add to `src/common/CMakeLists.txt`:

```cmake
# Thread-Metric test sources
set(THREAD_METRIC_SOURCES
    thread_metric/tm_adapter.c
    thread_metric/tm_basic_processing_test.c
    thread_metric/tm_cooperative_scheduling_test.c
    thread_metric/tm_preemptive_scheduling_test.c
    thread_metric/tm_interrupt_processing_test.c
    thread_metric/tm_interrupt_preemption_processing_test.c
    thread_metric/tm_message_processing_test.c
    thread_metric/tm_synchronization_processing_test.c
    thread_metric/tm_memory_allocation_test.c
)

# Add Thread-Metric test option
option(ENABLE_THREAD_METRIC "Enable Thread-Metric benchmark tests" OFF)

if(ENABLE_THREAD_METRIC)
    target_sources(app PRIVATE ${THREAD_METRIC_SOURCES})
    target_compile_definitions(app PRIVATE THREAD_METRIC_ENABLED)
    
    # Individual test selection
    if(TM_TEST)
        target_compile_definitions(app PRIVATE RUN_TM_${TM_TEST})
    endif()
endif()
```

### Step 7: Implement Interrupt Handlers (RTOS-specific)

For FreeRTOS on Cortex-M, add to `bench_porting_layer_freertos.c`:

```c
/* Thread-Metric interrupt handlers */
#ifdef THREAD_METRIC_ENABLED

extern void tm_interrupt_handler(void);
extern void tm_interrupt_preemption_handler(void);

/* SVC Handler for Thread-Metric interrupt tests */
void vApplicationSVCHandler(void)
{
    /* Determine which test is active and call appropriate handler */
    #if defined(RUN_TM_INTERRUPT_PROCESSING)
    tm_interrupt_handler();
    #elif defined(RUN_TM_INTERRUPT_PREEMPTION)
    tm_interrupt_preemption_handler();
    #endif
}

#endif /* THREAD_METRIC_ENABLED */
```

### Step 8: Add bench_thread_sleep API

Add to `bench_api.h`:

```c
/**
 * @brief Sleep for specified number of seconds
 *
 * This routine suspends the calling thread for the specified
 * number of seconds.
 *
 * @param seconds Number of seconds to sleep
 */
void bench_thread_sleep(int seconds);
```

Implement in each RTOS porting layer:

```c
/* FreeRTOS implementation */
void bench_thread_sleep(int seconds)
{
    vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
}

/* Zephyr implementation */
void bench_thread_sleep(int seconds)
{
    k_sleep(K_SECONDS(seconds));
}
```

### Step 9: Create Test Runner

Create `src/common/thread_metric/tm_all.c`:

```c
/* SPDX-License-Identifier: Apache-2.0 */

#include "tm_adapter.h"
#include "bench_utils.h"

/* External test initialization functions */
extern void tm_basic_processing_initialize(void);
extern void tm_cooperative_scheduling_initialize(void);
extern void tm_preemptive_scheduling_initialize(void);
extern void tm_interrupt_processing_initialize(void);
extern void tm_interrupt_preemption_processing_initialize(void);
extern void tm_message_processing_initialize(void);
extern void tm_synchronization_processing_initialize(void);
extern void tm_memory_allocation_initialize(void);

/* Test function pointers and names */
typedef struct {
    const char *name;
    void (*init_func)(void);
} tm_test_entry_t;

static const tm_test_entry_t tm_tests[] = {
    {"Basic Processing", tm_basic_processing_initialize},
    {"Cooperative Scheduling", tm_cooperative_scheduling_initialize},
    {"Preemptive Scheduling", tm_preemptive_scheduling_initialize},
    {"Interrupt Processing", tm_interrupt_processing_initialize},
    {"Interrupt Preemption", tm_interrupt_preemption_processing_initialize},
    {"Message Processing", tm_message_processing_initialize},
    {"Synchronization Processing", tm_synchronization_processing_initialize},
    {"Memory Allocation", tm_memory_allocation_initialize},
};

#define NUM_TM_TESTS (sizeof(tm_tests) / sizeof(tm_tests[0]))

void tm_run_all_tests(void *arg)
{
    int i;
    
    PRINTF("\n*** Thread-Metric Benchmark Suite ***\n");
    PRINTF("Test Duration: %d seconds per test\n\n", TM_TEST_DURATION);
    
    for (i = 0; i < NUM_TM_TESTS; i++) {
        PRINTF("=== Running: %s ===\n", tm_tests[i].name);
        tm_initialize(tm_tests[i].init_func);
        /* Note: Each test runs for TM_TEST_DURATION seconds internally */
    }
    
    PRINTF("\n*** Thread-Metric Suite Complete ***\n");
}

#ifdef RUN_THREAD_METRIC_ALL
int main(void)
{
    PRINTF("\n\r *** Starting Thread-Metric Suite ***\n\n\r");
    bench_test_init(tm_run_all_tests);
    PRINTF("\n\r *** Done! ***\n\r");
    return 0;
}
#endif
```

### Step 10: Build and Test

```bash
# Build with Thread-Metric enabled
cmake -GNinja -DRTOS=freertos -DBOARD=frdm_k64f \
      -DMCUX_SDK_PATH=/path/to/SDK \
      -DENABLE_THREAD_METRIC=ON \
      -S . -B build

ninja -C build

# Run specific Thread-Metric test
cmake -GNinja -DRTOS=freertos -DBOARD=frdm_k64f \
      -DMCUX_SDK_PATH=/path/to/SDK \
      -DENABLE_THREAD_METRIC=ON \
      -DTM_TEST=COOPERATIVE_SCHEDULING \
      -S . -B build
```

---

## Implementation Details

### Priority Mapping Considerations

Thread-Metric uses priorities 1-31 (1 = highest), while different RTOSes have varying priority schemes.

**CRITICAL:** The mapping must preserve uniqueness for adjacent priorities. Tests like Preemptive Scheduling use threads at priorities 6, 7, 8, 9, 10 - these MUST map to 5 distinct RTOS priorities.

| RTOS | Priority Range | Highest | Recommended Mapping |
|------|----------------|---------|---------------------|
| FreeRTOS | 0 to configMAX_PRIORITIES-1 | Highest number | Direct 1:1 for TM 1-15, via bench_api inversion |
| Zephyr | Depends on config | Lowest number | Direct 1:1 for TM 1-15 |
| ThreadX | 0-31 | 0 | `tm_priority - 1` |

**FreeRTOS Note:** With `configMAX_PRIORITIES=16`, the bench_api porting layer performs: `FreeRTOS_priority = configMAX_PRIORITIES - bench_priority`. So bench priority 1 becomes FreeRTOS 15 (highest), and bench priority 15 becomes FreeRTOS 1 (lowest usable).

### Memory Pool Implementation

Thread-Metric expects fixed-block memory pools (128-byte blocks). Options:

1. **Simple Static Pool** (shown in adapter): Single block, simple free list
2. **FreeRTOS Heap**: Use `pvPortMalloc()`/`vPortFree()`
3. **Custom Block Pool**: Implement proper block pool allocator

For accurate benchmarking, consider implementing a proper block pool:

```c
#define TM_BLOCK_COUNT      16
#define TM_BLOCK_SIZE       128

typedef struct {
    unsigned char blocks[TM_BLOCK_COUNT][TM_BLOCK_SIZE];
    unsigned char free_flags[TM_BLOCK_COUNT];
} tm_block_pool_t;

static tm_block_pool_t tm_pools[TM_MAX_POOLS];
```

### Reporting Thread Implementation

Thread-Metric uses a high-priority reporting thread that wakes every `TM_TEST_DURATION` seconds. This thread:
1. Sleeps for 30 seconds
2. Prints current counter values
3. Calculates throughput
4. Validates counter integrity

The reporting thread must have higher priority (priority 2) than test threads.

### Interrupt Test Considerations

The interrupt tests require:

1. **Software Interrupt Mechanism**: SVC on Cortex-M
2. **Custom ISR Installation**: May conflict with RTOS exception handlers
3. **Context Save/Restore**: ISR must save/restore full context

For FreeRTOS, the SVC handler is often used by the kernel. Consider:
- Using a different software interrupt (PendSV alternative)
- Using a hardware timer interrupt
- Modifying FreeRTOS port to chain handlers

---

## Testing and Validation

### Validation Criteria

Each Thread-Metric test includes built-in validation:

1. **Counter Consistency**: All thread counters should be within ±1 of average
2. **Counter Progress**: Counters should increment each period
3. **Error Detection**: Tests report errors via `PRINTF` macro to make assure compatibility in different hardware

### Expected Output

```
**** Thread-Metric Cooperative Scheduling Test **** Relative Time: 30
tm_cooperative_thread_0_counter: 245123
tm_cooperative_thread_1_counter: 245122
tm_cooperative_thread_2_counter: 245123
tm_cooperative_thread_3_counter: 245122
tm_cooperative_thread_4_counter: 245123
Time Period Total:  1225613

**** Thread-Metric Preemptive Scheduling Test **** Relative Time: 30
Time Period Total:  892456
```

### Comparison with Published Results

Microsoft publishes Thread-Metric results for ThreadX. After integration, compare results to verify correct implementation:

| Test | ThreadX Reference | Your Result | Variance |
|------|-------------------|-------------|----------|
| Basic Processing | ~X,XXX,XXX | | |
| Cooperative | ~X,XXX,XXX | | |
| Preemptive | ~XXX,XXX | | |

---

## RP2350/FreeRTOS Implementation Notes

This section documents critical implementation details and fixes discovered during integration on the RP2350 (Pimoroni Explorer) platform with FreeRTOS.

### Critical: Volatile Counter Variables

**Problem:** Without the `volatile` keyword, the compiler optimizes away counter reads/writes between threads, resulting in counters showing 0 even when threads are running.

**Solution:** All counter variables shared between threads MUST be declared `volatile`:

```c
/* CORRECT - counters visible across threads */
volatile unsigned long tm_basic_processing_counter;
volatile unsigned long tm_cooperative_thread_0_counter;

/* WRONG - compiler may optimize away */
unsigned long tm_basic_processing_counter;  /* DON'T DO THIS */
```

**Note:** Message buffers do NOT need `volatile` because the queue API handles synchronization internally.

### Critical: FreeRTOS Task Termination

**Problem:** FreeRTOS tasks must NEVER return from their entry function. Returning causes undefined behavior and typically results in a `*** PANIC ***` error.

**Solution:** All test runner functions must suspend themselves at the end:

```c
void tm_run_test(void *arg)
{
    (void)arg;
    PRINTF("\n*** Thread-Metric Test ***\n");
    tm_initialize(test_init_function);
    
    /* CRITICAL: Must not return - suspend forever */
    #ifdef FREERTOS
    extern void vTaskSuspend(void *);
    vTaskSuspend(NULL);
    #endif
    
    for (;;) {}  /* Fallback */
}
```

### Critical: Priority Mapping

**Problem:** The initial priority mapping `(tm_priority + 1) / 2` caused priority collisions:
- TM priorities 9 and 10 both mapped to bench priority 5
- TM priorities 7 and 8 both mapped to bench priority 4

This broke the preemptive scheduling test because threads at the same priority don't preempt each other.

**Solution:** Use direct 1:1 mapping for priorities 1-15 (which covers all Thread-Metric tests):

```c
static int tm_map_priority(int tm_priority)
{
    /* Clamp to valid range */
    if (tm_priority < 1) {
        return 1;
    } else if (tm_priority > 15) {
        return 15;
    }
    /* Direct 1:1 mapping preserves relative ordering */
    return tm_priority;
}
```

### FreeRTOS Thread Creation Behavior

**Problem:** FreeRTOS starts tasks immediately upon creation via `xTaskCreate()`, but Thread-Metric expects threads to be created in a suspended state.

**Solution:** The adapter suspends threads immediately after creation:

```c
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    /* ... setup ... */
    
    result = bench_thread_create(thread_id, name, tm_map_priority(priority),
                                  tm_thread_wrapper, (void *)(uintptr_t)thread_id);

    if (result == BENCH_SUCCESS) {
        tm_thread_created[thread_id] = 1;
        
        /* Thread-Metric expects suspended-on-creation */
        bench_thread_suspend(thread_id);
        
        return TM_SUCCESS;
    }
    return TM_ERROR;
}
```

### FreeRTOS Configuration Requirements

Update `FreeRTOSConfig.h` for Thread-Metric compatibility:

```c
/* Increase priority levels for proper Thread-Metric priority mapping */
#define configMAX_PRIORITIES                    16

/* Enable stack overflow detection (helpful for debugging) */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* Ensure these are enabled */
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelay                      1
```

### Stack Size Requirements

Thread-Metric tests require more stack space than typical benchmarks. Increase the stack size in the porting layer:

```c
/* Increased stack size for Thread-Metric tests */
#define STACK_SIZE (configMINIMAL_STACK_SIZE + 512)
```

### Stack Overflow Hook

Add a stack overflow hook for better debugging:

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    PRINTF("\n*** STACK OVERFLOW in task: %s ***\n", pcTaskName);
    for (;;) { /* Halt here for debugging */ }
}
```

### DWT Timing (RP2350)

For high-resolution timing on Cortex-M33, enable the DWT cycle counter:

```cmake
# In CMakeLists.txt - DWT enabled by default for Thread-Metric
if(ENABLE_THREAD_METRIC AND NOT DEFINED USE_DWT_TIMING)
    option(USE_DWT_TIMING "Use DWT cycle counter for timing" ON)
endif()
```

### Build Commands for RP2350

```bash
# Create build directory
mkdir build_tm && cd build_tm

# Configure with Thread-Metric enabled
cmake -DRTOS=freertos -DBOARD=pico2_sdk -DENABLE_THREAD_METRIC=ON \
      -DPICO_SDK_PATH=$PICO_SDK_PATH \
      -DFREERTOS_KERNEL_PATH=$FREERTOS_KERNEL_PATH ..

# Build
make -j4

# Flash to device
make flash

# Run specific test (rebuild required)
cmake -DRTOS=freertos -DBOARD=pico2_sdk -DENABLE_THREAD_METRIC=ON \
      -DTM_TEST=COOPERATIVE_SCHEDULING \
      -DPICO_SDK_PATH=$PICO_SDK_PATH \
      -DFREERTOS_KERNEL_PATH=$FREERTOS_KERNEL_PATH ..
make -j4 && make flash
```

### RP2350 FreeRTOS Results Reference

Results from Pimoroni Explorer (RP2350 @ 150 MHz) with FreeRTOS:

| Test | Operations/30s | Notes |
|------|----------------|-------|
| Basic Processing | ~399,000 | CPU baseline, RTOS-independent |
| Cooperative Scheduling | TBD | 5 threads yielding |
| Preemptive Scheduling | ~55,800,000 | 5-thread priority chain |
| Message Processing | TBD | Queue send/receive |
| Synchronization | TBD | Semaphore get/put |
| Memory Allocation | TBD | Block alloc/free |

**Note:** Basic Processing measures raw CPU throughput and should be consistent across RTOSes on the same hardware. Other tests measure RTOS-specific overhead - higher numbers indicate better performance.

---

## Appendix

### A. Complete File Listing

```
src/common/thread_metric/
├── CMakeLists.txt
├── tm_adapter.h
├── tm_adapter.c
├── tm_all.c
├── tm_basic_processing_test.c
├── tm_cooperative_scheduling_test.c
├── tm_preemptive_scheduling_test.c
├── tm_interrupt_processing_test.c
├── tm_interrupt_preemption_processing_test.c
├── tm_message_processing_test.c
├── tm_synchronization_processing_test.c
└── tm_memory_allocation_test.c
```

### B. Build Configuration Options

| CMake Variable | Values | Description |
|----------------|--------|-------------|
| `ENABLE_THREAD_METRIC` | ON/OFF | Enable Thread-Metric tests |
| `TM_TEST` | Test name | Run specific test only |
| `TM_TEST_DURATION` | Integer | Test duration in seconds (default: 30) |

### C. Test Name Mapping

| CMake TM_TEST Value | Test File |
|--------------------|-----------|
| `BASIC_PROCESSING` | tm_basic_processing_test.c |
| `COOPERATIVE_SCHEDULING` | tm_cooperative_scheduling_test.c |
| `PREEMPTIVE_SCHEDULING` | tm_preemptive_scheduling_test.c |
| `INTERRUPT_PROCESSING` | tm_interrupt_processing_test.c |
| `INTERRUPT_PREEMPTION` | tm_interrupt_preemption_processing_test.c |
| `MESSAGE_PROCESSING` | tm_message_processing_test.c |
| `SYNCHRONIZATION` | tm_synchronization_processing_test.c |
| `MEMORY_ALLOCATION` | tm_memory_allocation_test.c |

### D. References

1. [ThreadX GitHub Repository](https://github.com/eclipse-threadx/threadx)
2. [Thread-Metric Source](https://github.com/eclipse-threadx/threadx/tree/master/utility/benchmarks/thread_metric)
3. [Thread-Metric Documentation](thread_metric_readme.txt)

---

*Document Version: 1.1*
*Created: January 2026*
*Updated: January 2026 - Added RP2350/FreeRTOS implementation notes, priority mapping fixes, volatile requirements*
*Author: Generated for rtos-benchmark2 project*
