cmake_minimum_required(VERSION 3.22)

set(AVAILABLE_TESTS
    interrupt_latency
    malloc_free
    message_queue
    mutex_lock_unlock
    sem_context_switch
    sem_signal_release
    thread_switch_yield
    thread)

set(AVAILABLE_RTOSES
    zephyr
    freertos)

set(ITERATIONS 10000 CACHE STRING "Number of iterations for each test")
set(CALIBRATION_LOOPS 10000 CACHE STRING "Number of calibration loops for each test")

if (NOT RTOS IN_LIST AVAILABLE_RTOSES)
    message(FATAL_ERROR "Choose either [zephyr] or [freertos]")
endif()

# Add include directory for bench_api.h and bench_utils.h
include_directories(h)

# Include any ${RTOS} specific build rules
# Check for board-specific cmake file first (e.g., freertos_pico2.cmake)
if(DEFINED BOARD)
    set(_RTOS_BOARD_CMAKE "src/${RTOS}/${RTOS}_${BOARD}.cmake")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_RTOS_BOARD_CMAKE}")
        include(${_RTOS_BOARD_CMAKE})
        # For Pico SDK builds, the included cmake handles everything
        if(PICO_SDK_BUILD)
            add_subdirectory(src/${RTOS})
            return()
        endif()
    else()
        include(src/${RTOS}/${RTOS}.cmake)
    endif()
else()
    include(src/${RTOS}/${RTOS}.cmake)
endif()
add_subdirectory(src/${RTOS})

# project() not called for Pico SDK builds (already done in the board cmake)
if(NOT PICO_SDK_BUILD)
    project(bench)
    # Set iteration flags AFTER project()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DITERATIONS=${ITERATIONS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCALIBRATION_LOOPS=${CALIBRATION_LOOPS}")
endif()

list(LENGTH TEST tests_to_run)
if (${tests_to_run} EQUAL 0)
    list(TRANSFORM AVAILABLE_TESTS REPLACE "(.+)" "src/common/bench_\\1_test.c" OUTPUT_VARIABLE sources)

    target_sources(app PRIVATE ${sources})
    target_sources(app PRIVATE src/common/bench_utils.c)
    target_sources(app PRIVATE src/common/bench_all.c)

    return()
endif()

if(NOT ${tests_to_run} EQUAL 1)
    message(FATAL_ERROR "Choose exactly one test")
endif()

if (NOT ${TEST} IN_LIST AVAILABLE_TESTS)
    list(JOIN AVAILABLE_TESTS "\n" pretty_print)
    message(FATAL_ERROR "Choose one test from:\n${pretty_print}")
endif()

string(TOUPPER ${TEST} test_upper)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRUN_${test_upper}")
target_sources(app PRIVATE src/common/bench_utils.c)
target_sources(app PRIVATE src/common/bench_${TEST}_test.c)
