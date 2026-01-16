/*
 * SPDX-License-Identifier: Apache-2.0
 * Board definitions for Raspberry Pi Pico 2 (RP2350)
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* The board name */
#define BOARD_NAME "Raspberry Pi Pico 2"

/* UART configuration for debug console */
#define BOARD_DEBUG_UART_INSTANCE   0
#define BOARD_DEBUG_UART_BAUDRATE   115200
#define BOARD_DEBUG_UART_TX_PIN     0
#define BOARD_DEBUG_UART_RX_PIN     1

/* LED configuration */
#define BOARD_LED_PIN               25

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
