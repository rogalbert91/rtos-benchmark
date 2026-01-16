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

/*******************************************************************************
 * API
 ******************************************************************************/

/**
 * @brief Get system core clock frequency
 * @return Clock frequency in Hz
 */
static inline uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    return BOARD_BOOTCLOCKRUN_CORE_CLOCK;
}

/**
 * @brief Update system core clock variable
 */
void SystemCoreClockUpdate(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _CLOCK_CONFIG_H_ */
