#ifndef UTILITY_H
#define UTILITY_H

void uart_write_hex_bytes(uint8_t uart, uint8_t* start, uint32_t len);
void uart_read_wrp(uint8_t uart, int blocking, int* read, uint8_t* out, size_t bytes);
void uart_write_wrp(uint8_t uart, uint8_t* in, size_t bytes);
#endif