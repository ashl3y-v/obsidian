#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "uart.h"
#include "inc/hw_types.h" 
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"

void uart_write_hex_bytes(uint8_t uart, uint8_t* start, uint32_t len);
void uart_read_wrp(uint8_t uart, int blocking, int* read, uint8_t* out, size_t bytes);
void uart_write_wrp(uint8_t uart, uint8_t* in, size_t bytes);
void reject();

#endif