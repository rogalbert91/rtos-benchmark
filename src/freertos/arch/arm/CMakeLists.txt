# Select architecture-specific code based on target
if(CONFIG_NO_DWT)
    # Cortex-M33 without DWT - use SysTick-based timing
    add_subdirectory(cortex_m33_systick)
elseif(CONFIG_CPU_CORTEX_M)
    # FRDM-K64F and other Cortex-M with DWT - use DWT cycle counter
    add_subdirectory(cortex_m)
else()
    message(FATAL_ERROR "Unknown ARM processor!")
endif()
