# Add the board-specific subdirectory based on BOARD variable
if(BOARD STREQUAL "frdm_k64f")
    add_subdirectory(frdm_k64f)
elseif(BOARD STREQUAL "pico2")
    add_subdirectory(pico2)
else()
    message(FATAL_ERROR "Unsupported BOARD: ${BOARD}. Supported: frdm_k64f, pico2")
endif()
