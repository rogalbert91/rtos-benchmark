# SPDX-License-Identifier: Apache-2.0
# CMake configuration for RP2350 / Raspberry Pi Pico 2
#
# This fragment is included by "../../CMakeLists.txt".
# All relative paths are relative to "../..".

# Find ARM toolchain
find_program(ARM_GCC arm-none-eabi-gcc)
if(NOT ARM_GCC)
    message(FATAL_ERROR "arm-none-eabi-gcc not found. Please install ARM GCC toolchain.")
endif()

# Set cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# Compiler flags for RP2350 / Cortex-M33
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFREERTOS")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m33")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=softfp")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=fpv5-sp-d16")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSYS_CLOCK_HW_CYCLES_PER_SEC=150000000")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCONFIG_CPU_CORTEX_M_HAS_VTOR")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPICO_RP2350=1")

# Configuration flags for build system
set(CONFIG_ARM ON)
set(CONFIG_CPU_CORTEX_M33 ON)
set(CONFIG_CORTEX_M_SYSTICK ON)
set(CONFIG_NO_DWT ON)  # RP2350 doesn't have DWT

enable_language(ASM)

# FreeRTOS Kernel path - can be overridden via -DFREERTOS_KERNEL_PATH
if(NOT DEFINED FREERTOS_KERNEL_PATH)
    # Try common locations
    if(EXISTS "$ENV{HOME}/Code/FreeRTOS-Kernel/tasks.c")
        set(FREERTOS_KERNEL_PATH "$ENV{HOME}/Code/FreeRTOS-Kernel")
    elseif(EXISTS "/home/alberto_rodriguez/Code/FreeRTOS-Kernel/tasks.c")
        set(FREERTOS_KERNEL_PATH "/home/alberto_rodriguez/Code/FreeRTOS-Kernel")
    else()
        message(FATAL_ERROR "FreeRTOS-Kernel not found. Please set -DFREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel")
    endif()
endif()
message(STATUS "Using FreeRTOS-Kernel from: ${FREERTOS_KERNEL_PATH}")

# Include directories - ORDER MATTERS!
# Board-specific includes first (to pick up pico2's FreeRTOSConfig.h)
include_directories(src/freertos/boards/pico2)
include_directories(src/freertos)

# FreeRTOS include directories (Cortex-M33 non-TrustZone port)
include_directories(${FREERTOS_KERNEL_PATH}/include)
include_directories(${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure)

# CMSIS headers - use local minimal headers for RP2350
include_directories(src/freertos/boards/pico2/cmsis)

# Also include RP2040 FreeRTOS port headers if available (for portmacro.h)
if(EXISTS "${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include")
    include_directories(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include)
endif()

# Create executable
add_executable(app src/freertos/bench_porting_layer_freertos.c)

# FreeRTOS kernel sources
target_sources(app PRIVATE ${FREERTOS_KERNEL_PATH}/tasks.c)
target_sources(app PRIVATE ${FREERTOS_KERNEL_PATH}/queue.c)
target_sources(app PRIVATE ${FREERTOS_KERNEL_PATH}/list.c)
target_sources(app PRIVATE ${FREERTOS_KERNEL_PATH}/timers.c)
target_sources(app PRIVATE ${FREERTOS_KERNEL_PATH}/stream_buffer.c)
target_sources(app PRIVATE ${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure/port.c)
target_sources(app PRIVATE ${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c)

# Linker settings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nano.specs")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_CURRENT_SOURCE_DIR}/src/freertos/boards/pico2/memmap_ram.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostartfiles")

target_link_libraries(app PRIVATE -Wl,--start-group)
target_link_libraries(app PRIVATE c)
target_link_libraries(app PRIVATE gcc)
target_link_libraries(app PRIVATE nosys)
target_link_libraries(app PRIVATE -Wl,--end-group)

# Output settings
set(EXEC_NAME freertos_pico2.elf)
set_target_properties(app PROPERTIES OUTPUT_NAME freertos_pico2 SUFFIX .elf)

# Post-build: generate binary and UF2 files
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:app> freertos_pico2.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:app>
    COMMENT "Generating binary file"
)

# Flash target using picotool (if available)
add_custom_target(flash 
    USES_TERMINAL 
    DEPENDS app 
    COMMAND picotool load -f $<TARGET_FILE:app>
)
