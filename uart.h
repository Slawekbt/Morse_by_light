/******************************************************************************
 * This file is a part of the Sysytem Microprocessor Tutorial (C).            *
 ******************************************************************************/

/**
 * @file uart.h
 * @author Koryciak
 * @date Sep 2020
 * @brief File containing enums, structures and declarations.
 * @ver 0.1
 */

#ifndef UART_H
#define UART_H

#include "frdm_bsp.h"

/**
 * @brief UART initialization
 *
 * @param Baud rate (e.g. 9600)
 */
void UART_Init(void);
/**
 * @brief Print to serial port
 *
 * @param String to display.
 */
void UART_Print(const char *str);
/**
 * @brief Print to serial port and go to the next line
 *
 * @param String to display.
 */
void UART_Println(const char *str);

void uart_tx_ready(void);
int UART_Read(int znacznik_transmisji, char* rx_buf, int rx_buf_len);

#endif /* UART_H */
