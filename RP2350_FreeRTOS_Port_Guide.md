# Porting RTOS-Benchmark to RP2350 (Cortex-M33) with FreeRTOS

## Overview

This document provides a **complete** guide for running the full FreeRTOS benchmarks on the **Raspberry Pi RP2350** (Pico 2, Pimoroni Explorer, etc.).

### Key Discovery

The standard FreeRTOS `ARM_CM33_NTZ` port does **NOT** work directly with RP2350 because of conflicts with the Pico SDK's interrupt handling. Instead, use the **FreeRTOS RP2040 port** with compatibility defines - this works perfectly!

### Tested Configuration

| Component | Details |
|-----------|---------|
| **Board** | Pimoroni Pico Explorer (PIM720) with RP2350 |
| **CPU** | ARM Cortex-M33 @ 150 MHz |
| **FreeRTOS Port** | `portable/ThirdParty/GCC/RP2040` (adapted for RP2350) |
| **Timing** | Pico SDK's `time_us_64()` or DWT cycle counter |
| **UART** | GPIO 0 (TX), GPIO 1 (RX) @ 115200 baud |
| **LED/Indicator** | GPIO 26 (LCD backlight on Pimoroni Explorer) |

### Known Limitations

| Feature | Status | Notes |
|---------|--------|-------|
| **Thread Create/Start** | ❌ N/A | FreeRTOS combines these operations |

### Benchmark Results

```
*** RP2350 FreeRTOS Benchmark ***
CPU: 150 MHz

 *** Starting! ***

** Thread stats [avg, min, max] in nanoseconds **
 Spawn (no context switch)               :   2373,   2360,  14120
 Create (no context switch)              :    n/a,    n/a,    n/a
 Start (no context switch)               :    n/a,    n/a,    n/a
 Suspend (no context switch)             :   1120,   1106,  21073
 Resume (no context switch)              :    886,    880,  12106
 Spawn (context switch)                  :   3100,   3093,  11566
 Start  (context switch)                 :    n/a,    n/a,    n/a
 Suspend (context switch)                :   2266,   2260,  10053
 Resume (context switch)                 :   1820,   1820,   3986
 Terminate (context switch)              :   4346,   4340,   8166
** Mutex Stats [avg, min, max] in nanoseconds **
 Lock (no owner)                         :    900,    893,  10826
 Unlock (no waiters)                     :   1086,   1086,   6233
 Recursive lock                          :    353,    353,   2226
 Recursive unlock                        :    353,    353,   1006
 Unlock with unpend (no context switch)  :   1113,   1086,  28953
 Unlock with unpend (context switch)     :   3240,   3233,  10933
 Pend (no priority inheritance)          :   4866,   4860,  10000
 Pend (priority inheritance)             :   5286,   5280,   7546
** Semaphore stats [avg, min, max] in nanoseconds **
 Take (context switch)                   :   4440,   4426,  16413
 Give (context switch)                   :   2446,   2433,  13380
** Semaphore stats [avg, min, max] in nanoseconds **
 Give (no context switch)                :    653,    653,   1306
 Take (no context switch)                :    626,    626,   2540
** Yield stats [avg, min, max] in nanoseconds **
 Yield (no context switch)               :   1020,   1020,   2846
 Yield (context switch)                  :   1066,   1040,  10960
** Allocation stats [avg, min, max] in nanoseconds **
 Malloc                                  :    153,    153,   1140
 Free                                    :    153,    153,    806
** Message queue stats [avg, min, max] in nanoseconds **
 Create                                  :    966,    960,  10580
 Send (no context switch)                :    960,    953,  12733
 Receive (no context switch)             :    873,    866,  11726
 Send (context switch)                   :   3040,   2900,  17893
 Receive (context switch)                :   4066,   3633,   6420
** Interrupt Stats [avg, min, max] in nanoseconds **
 Latency                                 :    140,    140,   2280

 *** Done! ***
```

---

## Prerequisites

### 1. Install Pico SDK

```bash
cd ~/Code  # or your preferred location
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=$(pwd)
```

### 2. Clone FreeRTOS-Kernel

```bash
cd ~/Code
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git
export FREERTOS_KERNEL_PATH=$(pwd)/FreeRTOS-Kernel
```

### 3. Install Toolchain

```bash
sudo apt install cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi
```

### 4. Install picotool

```bash
sudo apt install picotool
# Or build from source: https://github.com/raspberrypi/picotool
```

---

## Project Structure

The rtos-benchmark integration for RP2350 requires the following files:

```
rtos-benchmark/
├── src/freertos/
│   ├── freertos_pico2_sdk.cmake          # Main CMake configuration
│   ├── bench_porting_layer_freertos.c    # Modified for PICO_RP2350
│   ├── bench_porting_layer_freertos.h    # Modified for PICO_RP2350
│   └── boards/
│       └── pico2_sdk/
│           ├── FreeRTOSConfig.h          # FreeRTOS configuration
│           ├── board.h                   # Board definitions
│           ├── board_pico2.c             # Board initialization
│           ├── clock_config.h            # Clock definitions
│           ├── clock_config.c            # Clock implementation
│           ├── pin_mux.h                 # Pin mux header
│           ├── pin_mux.c                 # Pin mux (stub)
│           ├── arch_pico2.c              # Timing implementation
│           └── timer_pico2.c             # Timer/ISR implementation
└── h/
    └── bench_api.h                       # Modified (added stdint.h)
```

---

## Build Instructions

### Build the Benchmark

**Default build (recommended - uses `time_us_64()` for timing):**
```bash
cd rtos-benchmark
mkdir build_pico2 && cd build_pico2
cmake -DRTOS=freertos -DBOARD=pico2_sdk \
      -DPICO_SDK_PATH=$PICO_SDK_PATH \
      -DFREERTOS_KERNEL_PATH=$FREERTOS_KERNEL_PATH ..
make -j4
```

**With DWT cycle counter (experimental - cycle-accurate timing):**
```bash
cd rtos-benchmark
mkdir build_pico2 && cd build_pico2
cmake -DRTOS=freertos -DBOARD=pico2_sdk \
      -DUSE_DWT_TIMING=ON \
      -DPICO_SDK_PATH=$PICO_SDK_PATH \
      -DFREERTOS_KERNEL_PATH=$FREERTOS_KERNEL_PATH ..
make -j4
```

### Flash to Board

**Option 1: Via Debug Probe (SWD)** - Recommended
```bash
picotool load -f app.uf2 && picotool reboot
# Or use the make target:
make flash
```

**Option 2: Via BOOTSEL Mode**
1. Hold BOOTSEL button on board
2. Press and release RESET (or power cycle)
3. Release BOOTSEL
4. Copy `app.uf2` to the mounted USB drive

### Monitor Output

```bash
# Using minicom
minicom -D /dev/ttyACM0 -b 115200

# Or using screen
screen /dev/ttyACM0 115200

# Or using picocom
picocom -b 115200 /dev/ttyACM0
```

---

## File Contents

### 1. `src/freertos/freertos_pico2_sdk.cmake`

This is the main CMake configuration file:

```cmake
# FreeRTOS benchmark for RP2350 (Pico 2) using Pico SDK
#
# Build:
#   mkdir build && cd build
#   cmake -DRTOS=freertos -DBOARD=pico2_sdk ..
#   make -j4
#
# Flash:
#   make flash
#   # or: picotool load -f app.uf2 && picotool reboot

cmake_minimum_required(VERSION 3.13)

# Mark that we're doing a Pico SDK build - skips subdirectory processing
set(PICO_SDK_BUILD ON CACHE BOOL "Pico SDK based build" FORCE)

# Pico SDK and FreeRTOS paths
set(PICO_BOARD pico2)
set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to the Pico SDK")
set(FREERTOS_KERNEL_PATH "$ENV{FREERTOS_KERNEL_PATH}" CACHE PATH "Path to FreeRTOS-Kernel")

# Fallback paths if env vars not set (adjust to your setup)
if(NOT PICO_SDK_PATH OR PICO_SDK_PATH STREQUAL "")
    set(PICO_SDK_PATH "/path/to/pico-sdk")
endif()
if(NOT FREERTOS_KERNEL_PATH OR FREERTOS_KERNEL_PATH STREQUAL "")
    set(FREERTOS_KERNEL_PATH "/path/to/FreeRTOS-Kernel")
endif()

message(STATUS "PICO_SDK_PATH: ${PICO_SDK_PATH}")
message(STATUS "FREERTOS_KERNEL_PATH: ${FREERTOS_KERNEL_PATH}")

# Initialize Pico SDK BEFORE project() - this is required
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Project must be called AFTER pico_sdk_init.cmake but BEFORE pico_sdk_init()
project(bench C CXX ASM)

# Now initialize Pico SDK
pico_sdk_init()

# FreeRTOS Kernel library
add_library(freertos_kernel STATIC
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_KERNEL_PATH}/timers.c
    ${FREERTOS_KERNEL_PATH}/event_groups.c
    ${FREERTOS_KERNEL_PATH}/stream_buffer.c
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/port.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
)

target_include_directories(freertos_kernel PUBLIC
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include
    ${CMAKE_SOURCE_DIR}/src/freertos/boards/pico2_sdk
)

# RP2350 compatibility defines (RP2040 port adapted for RP2350)
target_compile_definitions(freertos_kernel PUBLIC
    SIO_IRQ_PROC0=SIO_IRQ_FIFO
    SIO_IRQ_PROC1=SIO_IRQ_FIFO
    configNUMBER_OF_CORES=1
    configUSE_CORE_AFFINITY=0
    portSUPPORT_SMP=0
    LIB_FREERTOS_KERNEL=1
    PICO_RP2350=1
    FREERTOS=1
    SYS_CLOCK_HW_CYCLES_PER_SEC=150000000
    CONFIG_CPU_CORTEX_M_HAS_VTOR=1
    # Disable SW spin locks (FreeRTOS RP2040 port uses HW spin locks)
    PICO_USE_SW_SPIN_LOCKS=0
)

target_link_libraries(freertos_kernel PUBLIC
    pico_stdlib
    pico_multicore
    hardware_exception
    hardware_clocks
    hardware_sync
)

# Create the app executable
add_executable(app)

# Board sources
set(BOARD_DIR ${CMAKE_SOURCE_DIR}/src/freertos/boards/pico2_sdk)

# Main benchmark application sources
set(BENCH_SOURCES
    ${CMAKE_SOURCE_DIR}/src/freertos/bench_porting_layer_freertos.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_all.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_thread_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_interrupt_latency_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_malloc_free_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_message_queue_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_mutex_lock_unlock_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_sem_context_switch_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_sem_signal_release_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_thread_switch_yield_test.c
    ${CMAKE_SOURCE_DIR}/src/common/bench_utils.c
    ${BOARD_DIR}/board_pico2.c
    ${BOARD_DIR}/clock_config.c
    ${BOARD_DIR}/pin_mux.c
    ${BOARD_DIR}/arch_pico2.c
    ${BOARD_DIR}/timer_pico2.c
)

target_sources(app PRIVATE ${BENCH_SOURCES})

target_include_directories(app PRIVATE
    ${BOARD_DIR}
    ${CMAKE_SOURCE_DIR}/h
    ${CMAKE_SOURCE_DIR}/src/freertos
)

# Timing configuration: 0 = time_us_64() (default), 1 = DWT cycle counter
option(USE_DWT_TIMING "Use DWT cycle counter for timing (experimental)" OFF)

target_compile_definitions(app PRIVATE
    PICO_RP2350=1
    FREERTOS=1
    LCD_BACKLIGHT_PIN=26
    SYS_CLOCK_HW_CYCLES_PER_SEC=150000000
    CONFIG_CPU_CORTEX_M_HAS_VTOR=1
    ITERATIONS=1000
    CALIBRATION_LOOPS=1000
    USE_DWT_TIMING=$<BOOL:${USE_DWT_TIMING}>
)

target_link_libraries(app
    freertos_kernel
    pico_stdlib
    hardware_clocks
)

# UART output (not USB)
pico_enable_stdio_uart(app 1)
pico_enable_stdio_usb(app 0)

# Generate UF2 and other output formats
pico_add_extra_outputs(app)

# Flash target - loads into flash via debug probe and starts execution
add_custom_target(flash
    DEPENDS app
    COMMAND picotool load -f ${CMAKE_BINARY_DIR}/app.uf2
    COMMAND picotool reboot
    COMMENT "Flashing to RP2350 flash and rebooting..."
)
```

### 2. `src/freertos/boards/pico2_sdk/FreeRTOSConfig.h`

```c
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "rp2040_config.h"

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      150000000
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                256
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configSTACK_DEPTH_TYPE                  uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   (64 * 1024)

#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_MALLOC_FAILED_HOOK            0

#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xEventGroupSetBitFromISR        1

/* SMP disabled for single core */
#define configNUMBER_OF_CORES                   1
#define configUSE_CORE_AFFINITY                 0
#define portSUPPORT_SMP                         0

#endif
```

### 3. `src/freertos/boards/pico2_sdk/board.h`

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 * Board definitions for RP2350 with Pico SDK
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* The board name */
#define BOARD_NAME "RP2350 (Pico 2 / Pimoroni Explorer)"

/* UART configuration for debug console */
#define BOARD_DEBUG_UART_INSTANCE   0
#define BOARD_DEBUG_UART_BAUDRATE   115200
#define BOARD_DEBUG_UART_TX_PIN     0
#define BOARD_DEBUG_UART_RX_PIN     1

/* LED/Activity indicator */
#ifndef LCD_BACKLIGHT_PIN
#define LCD_BACKLIGHT_PIN           26  /* Pimoroni Explorer LCD backlight */
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * API
 ******************************************************************************/

/**
 * @brief Initialize boot pins
 */
void BOARD_InitBootPins(void);

/**
 * @brief Initialize boot clocks
 */
void BOARD_InitBootClocks(void);

/**
 * @brief Initialize debug console (UART)
 */
void BOARD_InitDebugConsole(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _BOARD_H_ */
```

### 4. `src/freertos/boards/pico2_sdk/board_pico2.c`

```c
/*
 * Board support for RP2350 (Pico 2 / Pimoroni Explorer) with Pico SDK
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"

/* Board initialization */
void BOARD_InitBootPins(void)
{
    /* Initialize activity indicator pin (LCD backlight on Pimoroni Explorer) */
    gpio_init(LCD_BACKLIGHT_PIN);
    gpio_set_dir(LCD_BACKLIGHT_PIN, GPIO_OUT);
    gpio_put(LCD_BACKLIGHT_PIN, 1);  /* Turn on initially */
}

void BOARD_InitBootClocks(void)
{
    /* Pico SDK handles clock initialization via stdio_init_all() */
}

void BOARD_InitDebugConsole(void)
{
    stdio_init_all();
    printf("\n*** RP2350 FreeRTOS Benchmark ***\n");
    printf("CPU: %lu MHz\n", (unsigned long)(clock_get_hz(clk_sys) / 1000000));
}

/* Activity indicator toggle */
void board_led_toggle(void)
{
    static bool led_state = false;
    led_state = !led_state;
    gpio_put(LCD_BACKLIGHT_PIN, led_state);
}
```

### 5. `src/freertos/boards/pico2_sdk/clock_config.h`

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 * Clock configuration for RP2350
 */

#ifndef _CLOCK_CONFIG_H_
#define _CLOCK_CONFIG_H_

#include <stdint.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* RP2350 default clock frequency: 150 MHz */
#define BOARD_BOOTCLOCKRUN_CORE_CLOCK 150000000U

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Get system core clock frequency
 * @return Clock frequency in Hz
 */
static inline uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    return BOARD_BOOTCLOCKRUN_CORE_CLOCK;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _CLOCK_CONFIG_H_ */
```

### 6. `src/freertos/boards/pico2_sdk/clock_config.c`

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 * Clock configuration implementation for RP2350
 */

#include "clock_config.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"

/* System clock frequency variable (required by some code) */
uint32_t SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;

void SystemCoreClockUpdate(void)
{
    /* Update SystemCoreClock based on actual clock configuration */
    SystemCoreClock = clock_get_hz(clk_sys);
}
```

### 7. `src/freertos/boards/pico2_sdk/pin_mux.h`

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 * Pin mux definitions for RP2350
 */

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Initialize all pins
 */
void BOARD_InitPins(void);

#if defined(__cplusplus)
}
#endif

#endif /* _PIN_MUX_H_ */
```

### 8. `src/freertos/boards/pico2_sdk/pin_mux.c`

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 * Pin mux implementation for RP2350
 */

#include "pin_mux.h"
#include "board.h"

void BOARD_InitPins(void)
{
    /* Pin initialization handled in board_pico2.c via Pico SDK */
}
```

### 9. `src/freertos/boards/pico2_sdk/arch_pico2.c`

This implements the timing functions with a compile-time switch between `time_us_64()` and DWT:

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Timing implementation for RP2350
 * 
 * Two timing modes available (controlled by USE_DWT_TIMING):
 *   0 = time_us_64() - Reliable, ~1µs resolution (default)
 *   1 = DWT cycle counter - Cycle-accurate (~6.67ns at 150MHz)
 *
 * To enable DWT, add to cmake: -DUSE_DWT_TIMING=ON
 *
 * NOTE: DWT on RP2350 may have issues with FreeRTOS - use with caution.
 */

#include "arch_api.h"
#include "bench_api.h"
#include "bench_utils.h"
#include "FreeRTOS.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/structs/systick.h"
#include "hardware/exception.h"

#include <stdint.h>

#define NSEC_PER_SEC  1000000000ULL
#define USEC_PER_SEC  1000000ULL

/*
 * Configuration: Set USE_DWT_TIMING to switch timing source
 *   0 = time_us_64() (default, reliable)
 *   1 = DWT cycle counter (experimental)
 */
#ifndef USE_DWT_TIMING
#define USE_DWT_TIMING 0
#endif

/* ========================================================================== */
#if USE_DWT_TIMING
/* ========================================================================== */
/*
 * DWT-based timing implementation
 * 
 * Uses the Cortex-M33 DWT cycle counter for cycle-accurate timing.
 * Resolution: 1 cycle (~6.67ns at 150MHz)
 * 
 * WARNING: DWT on RP2350 may have issues with FreeRTOS. Use for testing only.
 */

/* DWT (Data Watchpoint and Trace) registers */
#define DWT_BASE        0xE0001000UL
#define DWT_CTRL        (*(volatile uint32_t *)(DWT_BASE + 0x000))
#define DWT_CYCCNT      (*(volatile uint32_t *)(DWT_BASE + 0x004))

/* CoreDebug registers */
#define CoreDebug_BASE  0xE000EDF0UL
#define CoreDebug_DEMCR (*(volatile uint32_t *)(CoreDebug_BASE + 0x00C))

/* Bit definitions */
#define CoreDebug_DEMCR_TRCENA_Msk  (1UL << 24)  /* Enable DWT and ITM */
#define DWT_CTRL_CYCCNTENA_Msk      (1UL << 0)   /* Enable cycle counter */

static uint32_t timing_start_cycles = 0;

void arch_timing_init(void)
{
    /* Enable DWT and ITM blocks */
    CoreDebug_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    /* Enable the cycle counter (do NOT reset it) */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void arch_timing_start(void)
{
    /* Record current cycle count as start point */
    timing_start_cycles = DWT_CYCCNT;
}

void arch_timing_stop(void)
{
    /* Nothing to do - counter keeps running */
}

bench_time_t arch_timing_counter_get(void)
{
    /* Return elapsed cycles since timing_start */
    uint32_t now = DWT_CYCCNT;
    return (bench_time_t)(now - timing_start_cycles);
}

/* ========================================================================== */
#else /* !USE_DWT_TIMING */
/* ========================================================================== */
/*
 * time_us_64() based timing implementation (DEFAULT)
 * 
 * Uses the Pico SDK's high-resolution timer which provides microsecond
 * resolution. We convert to/from "cycles" by multiplying/dividing by
 * the clock frequency.
 * 
 * Resolution: ~1µs (150 cycles at 150MHz)
 */

static uint64_t timing_start_us = 0;

void arch_timing_init(void)
{
    /* Pico SDK timer is already initialized by stdio_init_all() */
}

void arch_timing_start(void)
{
    timing_start_us = time_us_64();
}

void arch_timing_stop(void)
{
    /* Nothing to do - timer runs continuously */
}

bench_time_t arch_timing_counter_get(void)
{
    /* 
     * Return elapsed time in "cycles" 
     * cycles = microseconds * (clock_hz / 1000000)
     * For 150MHz: 1us = 150 cycles
     */
    uint64_t now_us = time_us_64();
    uint64_t elapsed_us = now_us - timing_start_us;
    
    /* Convert microseconds to cycles */
    return (bench_time_t)(elapsed_us * (SYS_CLOCK_HW_CYCLES_PER_SEC / USEC_PER_SEC));
}

#endif /* USE_DWT_TIMING */
/* ========================================================================== */

/*
 * Common functions for both timing implementations
 */

bench_time_t arch_timing_cycles_get(volatile bench_time_t *const start,
                                    volatile bench_time_t *const end)
{
    return (*end - *start);
}

bench_time_t arch_timing_cycles_to_ns(bench_time_t cycles)
{
    return (cycles * NSEC_PER_SEC) / SYS_CLOCK_HW_CYCLES_PER_SEC;
}
```

### 10. `src/freertos/boards/pico2_sdk/timer_pico2.c`

This implements timer ISR manipulation for interrupt latency tests:

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Timer ISR manipulation for RP2350 interrupt latency tests
 *
 * Uses SysTick for the system tick and interrupt latency measurements.
 * The RP2350 has VTOR, so we can redirect the vector table.
 */

#include "bench_api.h"
#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"

/* CMSIS-style definitions for Cortex-M33 */
#define SCB_VTOR_TBLOFF_Msk  (0x1FFFFFFFUL << 7)

/*
 * For the Cortex-M, the system tick ISR is at index 15 (offset 0x3c) in
 * the vector table. We create a copy of the vector table and switch to
 * using that one via VTOR.
 */
#define TIMER_ISR_VECTOR_TABLE_INDEX  15

static bench_isr_handler_t my_vector_table[48] __attribute__((aligned(256)));

bench_isr_handler_t bench_timer_isr_get(void)
{
    bench_isr_handler_t *table = (bench_isr_handler_t *)scb_hw->vtor;
    return table[TIMER_ISR_VECTOR_TABLE_INDEX];
}

uint32_t bench_timer_cycles_per_second(void)
{
    return SYS_CLOCK_HW_CYCLES_PER_SEC;
}

uint32_t bench_timer_cycles_per_tick(void)
{
    return (bench_timer_cycles_per_second() / configTICK_RATE_HZ);
}

void bench_timer_isr_set(bench_isr_handler_t isr)
{
    int i;
    bench_isr_handler_t *src = (bench_isr_handler_t *)scb_hw->vtor;

    /* Copy the current vector table */
    for (i = 0; i < 48; i++) {
        my_vector_table[i] = src[i];
    }

    /* Install our handler */
    my_vector_table[TIMER_ISR_VECTOR_TABLE_INDEX] = isr;
    
    /* Switch to our vector table */
    uint32_t save = save_and_disable_interrupts();
    scb_hw->vtor = (uint32_t)my_vector_table;
    __dsb();
    __isb();
    restore_interrupts(save);
}

bench_time_t bench_timer_cycles_diff(bench_time_t trigger_point,
                                     bench_time_t sample_point)
{
    /* SysTick counts down, so trigger > sample */
    return (trigger_point - sample_point + 1);
}

bench_time_t bench_timer_cycles_get(void)
{
    return (bench_time_t)systick_hw->cvr;
}

bench_time_t bench_timer_isr_expiry_set(uint32_t usec)
{
    uint32_t cycles_per_usec;
    uint32_t cycles;

    cycles_per_usec = (bench_timer_cycles_per_second() + 999999) / 1000000;
    cycles = cycles_per_usec * usec;

    systick_hw->rvr = cycles;
    systick_hw->cvr = cycles - 1;
    systick_hw->csr = 0x7;  /* Enable, interrupt, processor clock */

    return (bench_time_t)cycles;
}

void bench_timer_isr_restore(bench_isr_handler_t handler)
{
    uint32_t cycles;

    cycles = bench_timer_cycles_per_tick() - 1;
    systick_hw->rvr = cycles;
    systick_hw->cvr = 0;  /* Resets timer to cycles */
    systick_hw->csr = 0x7;  /* Enable, interrupt, processor clock */

    bench_timer_isr_set(handler);
}
```

---

## Modifications to Existing Files

### 1. `src/freertos/bench_porting_layer_freertos.h`

Add conditional include for RP2350:

```c
#if defined(PICO_RP2350)
#include <stdio.h>
#define PRINTF printf
#else
#include "fsl_debug_console.h"
#endif

/* ... existing feature flags ... */
#define RTOS_HAS_MESSAGE_QUEUE        1
```

### 2. `src/freertos/bench_porting_layer_freertos.c`

Add conditional includes at the top:

```c
/* Board-specific includes. */
#if defined(PICO_RP2350)
/* RP2350 - minimal includes */
#include <stdio.h>
#include <stdint.h>
#else
/* Freescale/NXP includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"
#endif
```

### 3. `h/bench_api.h`

Add standard type includes:

```c
#include <stdbool.h>
#include <stdint.h>
```

---

## Key Technical Details

### Why RP2040 Port Works for RP2350

The FreeRTOS RP2040 port (`portable/ThirdParty/GCC/RP2040`) is designed specifically for the Pico SDK and handles:

1. **Interrupt handlers** - Properly integrated with Pico SDK's exception system
2. **SysTick configuration** - Works with Pico SDK's timer infrastructure  
3. **Multicore support** - Can be disabled for single-core operation

The only change needed is mapping the SIO IRQ names:
- RP2040: `SIO_IRQ_PROC0`, `SIO_IRQ_PROC1`
- RP2350: `SIO_IRQ_FIFO`

### Critical Compatibility Defines

```cmake
target_compile_definitions(freertos_kernel PUBLIC
    SIO_IRQ_PROC0=SIO_IRQ_FIFO    # RP2350 IRQ name mapping
    SIO_IRQ_PROC1=SIO_IRQ_FIFO    # RP2350 IRQ name mapping  
    configNUMBER_OF_CORES=1       # Single core mode
    configUSE_CORE_AFFINITY=0     # Disable core affinity
    portSUPPORT_SMP=0             # Disable SMP
    LIB_FREERTOS_KERNEL=1         # Enable FreeRTOS in Pico SDK
    PICO_USE_SW_SPIN_LOCKS=0      # Use HW spin locks (important!)
)
```

### Why ARM_CM33_NTZ Port Doesn't Work

The standard FreeRTOS `ARM_CM33_NTZ` port conflicts with Pico SDK because:

1. It defines its own `SVC_Handler`, `PendSV_Handler`, `SysTick_Handler`
2. Pico SDK's `crt0.S` already has these handlers
3. The handlers use different calling conventions

### Timing Options

The RP2350 (Cortex-M33) actually **does have DWT** (Data Watchpoint and Trace) with a cycle counter! The current implementation uses `time_us_64()` for simplicity, but DWT provides cycle-accurate timing.

**Current Implementation (time_us_64):**
- Resolution: ~1µs (150 cycles at 150MHz)
- Simple, works out of the box with Pico SDK
- Good enough for most benchmarking

**DWT Cycle Counter:**
- Resolution: 1 cycle (~6.67ns at 150MHz)
- More accurate for fine-grained measurements
- Requires explicit initialization

---

## Using DWT for Cycle-Accurate Timing

If you need cycle-accurate timing, you can enable the DWT cycle counter on RP2350.

### DWT Register Definitions

```c
/* DWT (Data Watchpoint and Trace) registers for Cortex-M33 */
#define DWT_BASE        0xE0001000UL
#define DWT_CTRL        (*(volatile uint32_t *)(DWT_BASE + 0x000))
#define DWT_CYCCNT      (*(volatile uint32_t *)(DWT_BASE + 0x004))

/* Debug Exception and Monitor Control Register */
#define CoreDebug_BASE  0xE000EDF0UL
#define CoreDebug_DEMCR (*(volatile uint32_t *)(CoreDebug_BASE + 0x00C))

/* Bit definitions */
#define CoreDebug_DEMCR_TRCENA_Msk  (1UL << 24)  /* Enable DWT and ITM */
#define DWT_CTRL_CYCCNTENA_Msk     (1UL << 0)   /* Enable cycle counter */
```

### DWT-Based `arch_pico2.c`

Replace the `time_us_64()` implementation with this DWT-based version:

```c
/*
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Timing implementation for RP2350 using DWT cycle counter
 * 
 * The RP2350 Cortex-M33 has DWT with a 32-bit cycle counter.
 * This provides cycle-accurate timing (~6.67ns resolution at 150MHz).
 */

#include "arch_api.h"
#include "bench_api.h"
#include "bench_utils.h"
#include "FreeRTOS.h"

#include "pico/stdlib.h"
#include <stdint.h>

/* DWT registers */
#define DWT_BASE        0xE0001000UL
#define DWT_CTRL        (*(volatile uint32_t *)(DWT_BASE + 0x000))
#define DWT_CYCCNT      (*(volatile uint32_t *)(DWT_BASE + 0x004))

/* CoreDebug registers */
#define CoreDebug_BASE  0xE000EDF0UL
#define CoreDebug_DEMCR (*(volatile uint32_t *)(CoreDebug_BASE + 0x00C))

/* Bit definitions */
#define CoreDebug_DEMCR_TRCENA_Msk  (1UL << 24)
#define DWT_CTRL_CYCCNTENA_Msk     (1UL << 0)

#define NSEC_PER_SEC  1000000000ULL

void arch_timing_init(void)
{
    /* Enable DWT and ITM blocks */
    CoreDebug_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    /* Reset the cycle counter */
    DWT_CYCCNT = 0;
    
    /* Enable the cycle counter */
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void arch_timing_start(void)
{
    /* Reset cycle counter to 0 at start of timing */
    DWT_CYCCNT = 0;
}

void arch_timing_stop(void)
{
    /* Nothing to do - counter keeps running */
}

bench_time_t arch_timing_counter_get(void)
{
    /* Return current cycle count directly */
    return (bench_time_t)DWT_CYCCNT;
}

bench_time_t arch_timing_cycles_get(volatile bench_time_t *const start,
                                    volatile bench_time_t *const end)
{
    /* Handle 32-bit wraparound */
    return (*end - *start);
}

bench_time_t arch_timing_cycles_to_ns(bench_time_t cycles)
{
    return (cycles * NSEC_PER_SEC) / SYS_CLOCK_HW_CYCLES_PER_SEC;
}
```

### Key Differences: DWT vs time_us_64()

| Aspect | `time_us_64()` | DWT Cycle Counter |
|--------|---------------|-------------------|
| **Resolution** | 1 µs (~150 cycles) | 1 cycle (~6.67 ns) |
| **Counter Width** | 64-bit | 32-bit |
| **Wraparound** | ~584,542 years | ~28.6 seconds @ 150MHz |
| **Initialization** | Automatic | Requires explicit enable |
| **Use Case** | General timing | Fine-grained benchmarks |

### DWT Considerations

1. **32-bit Wraparound**: DWT_CYCCNT wraps around every ~28.6 seconds at 150MHz. The benchmark tests are fast enough that this is not an issue.

2. **Debug Connection**: On some Cortex-M33 implementations, DWT may require a debug probe to be connected, or specific chip configuration bits to be set. The RP2350 should work without a debugger attached.

3. **Verification**: You can verify DWT is working by checking the counter increments:

```c
void verify_dwt(void)
{
    arch_timing_init();
    
    uint32_t start = DWT_CYCCNT;
    for (volatile int i = 0; i < 1000; i++);
    uint32_t end = DWT_CYCCNT;
    
    printf("DWT test: start=%lu, end=%lu, diff=%lu cycles\n",
           (unsigned long)start, (unsigned long)end, 
           (unsigned long)(end - start));
    
    if (end > start) {
        printf("DWT is working!\n");
    } else {
        printf("DWT may not be enabled\n");
    }
}
```

### Switching Between Implementations

Use the `USE_DWT_TIMING` cmake option to switch between timing sources:

**Default (time_us_64 - recommended):**
```bash
cmake -DRTOS=freertos -DBOARD=pico2_sdk \
      -DPICO_SDK_PATH=$PICO_SDK_PATH \
      -DFREERTOS_KERNEL_PATH=$FREERTOS_KERNEL_PATH ..
```

**With DWT (experimental):**
```bash
cmake -DRTOS=freertos -DBOARD=pico2_sdk \
      -DUSE_DWT_TIMING=ON \
      -DPICO_SDK_PATH=$PICO_SDK_PATH \
      -DFREERTOS_KERNEL_PATH=$FREERTOS_KERNEL_PATH ..
```

> **Important:** Always do a clean rebuild when switching timing modes:
> ```bash
> rm -rf build_pico2 && mkdir build_pico2 && cd build_pico2
> ```

---

## Hardware Notes

### Pimoroni Pico Explorer (PIM720)

- **LCD Backlight**: GPIO 26 (use as activity indicator)
- **No onboard LED** on GPIO 25 (unlike standard Pico)
- **UART**: GPIO 0 (TX), GPIO 1 (RX)
- **Display**: 240x240 IPS LCD (separate driver needed)

### Standard Pico 2

- **LED**: GPIO 25
- **UART**: GPIO 0 (TX), GPIO 1 (RX)

To use with standard Pico 2, change `LCD_BACKLIGHT_PIN` to `25`:

```cmake
target_compile_definitions(app PRIVATE
    LCD_BACKLIGHT_PIN=25   # Standard Pico 2 onboard LED
    ...
)
```

### Debug Probe Connection

Connect RP Debug Probe to target:
- **SWCLK** → Target SWCLK
- **SWDIO** → Target SWDIO  
- **GND** → Target GND
- **UART TX** → Target GPIO 1 (RX)
- **UART RX** → Target GPIO 0 (TX)

---

## Troubleshooting

### No Serial Output

1. **Check UART wiring**: TX→RX, RX→TX (crossed)
2. **Check baud rate**: 115200
3. **Check GPIO pins**: 0 (TX), 1 (RX) for default Pico SDK

### Code Doesn't Run After Flash

1. **Use BOOTSEL mode**: Hold BOOTSEL, power cycle, copy UF2
2. **Use `picotool load -f <file>.uf2`** (not `.elf` or `.bin`)
3. **Reboot after flash**: `picotool reboot`
4. **Check picotool version** supports RP2350

### FreeRTOS Crashes at Startup

1. **Verify FreeRTOSConfig.h** includes `rp2040_config.h`
2. **Check static allocation callbacks** are implemented
3. **Increase stack sizes** if needed
4. **Verify `PICO_USE_SW_SPIN_LOCKS=0`** is set

### Build Errors

1. **Set `PICO_SDK_PATH`** environment variable
2. **Set `FREERTOS_KERNEL_PATH`** environment variable
3. **Use `set(PICO_BOARD pico2)`** for RP2350

### "n/a" in Benchmark Results

Some "n/a" results are expected because FreeRTOS doesn't have:
- Separate "Create" and "Start" operations (combined in `xTaskCreate`)

---

*Document updated: January 16, 2026*  
*Tested on: Pimoroni Pico Explorer (PIM720) with RP2350*  
*FreeRTOS Version: FreeRTOS-Kernel main branch*  
*Timing: `time_us_64()` (default) or DWT cycle counter *
