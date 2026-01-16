# FreeRTOS benchmark for RP2350 (Pico 2) using Pico SDK
#
# This configuration uses the Pico SDK for proper RP2350 initialization
# and the FreeRTOS RP2040 port adapted for RP2350.
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

# Fallback paths if env vars not set
if(NOT PICO_SDK_PATH OR PICO_SDK_PATH STREQUAL "")
    set(PICO_SDK_PATH "/home/alberto_rodriguez/Code/pico-sdk")
endif()
if(NOT FREERTOS_KERNEL_PATH OR FREERTOS_KERNEL_PATH STREQUAL "")
    set(FREERTOS_KERNEL_PATH "/home/alberto_rodriguez/Code/FreeRTOS-Kernel")
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
