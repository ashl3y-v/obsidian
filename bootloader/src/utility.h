#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "uart.h"
#include "inc/hw_types.h" 
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"


void uart_write_hex_bytes(uint8_t uart, uint8_t* start, uint32_t len);



/*
 * UART Read
 * Parameters:
 * uart - uart port 
 * blocking - wait until data is read before continuing
 * read - true if data was read succesfully, false otherwise
 * out - output buffer
 * bytes - amount of bytes to read
 * 
 * Returns:
 * data is written to the output buffer
 */
void uart_read_wrp(uint8_t uart, int blocking, int* read, uint8_t* out, size_t bytes);

/*
 * UART Write
 * Parameters:
 * uart - uart port 
 * in - input buffer
 * bytes - amount of bytes to write
 * 
 * Returns:
 * None
 */
void uart_write_wrp(uint8_t uart, uint8_t* in, size_t bytes);

/*
 * Reject
 * Called after the bootloader fails during a critical operation
 * 
 * Parameters:
 * None
 * 
 * Returns:
 * None
 */
void reject();
#endif