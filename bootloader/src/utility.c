#include "utility.h"



void uart_write_hex_bytes(uint8_t uart, uint8_t* start, uint32_t len) {
    for (uint8_t* cursor = start; cursor < (start + len); cursor += 1) {
        uint8_t data = *((uint8_t*)cursor);
        uint8_t right_nibble = data & 0xF;
        uint8_t left_nibble = (data >> 4) & 0xF;
        char byte_str[3];
        if (right_nibble > 9) {
            right_nibble += 0x37;
        } else {
            right_nibble += 0x30;
        }
        byte_str[1] = right_nibble;
        if (left_nibble > 9) {
            left_nibble += 0x37;
        } else {
            left_nibble += 0x30;
        }
        byte_str[0] = left_nibble;
        byte_str[2] = '\0';

        uart_write_str(uart, byte_str);
        uart_write_str(uart, " ");
    }
}



void uart_read_wrp(uint8_t uart, int blocking, int* read, uint8_t* out, size_t bytes)
{
    for (int i = 0; i < bytes; ++i) {
        volatile uint8_t data = uart_read(uart, blocking , read);
        memcpy(out + i, (uint8_t*)(&data), 1);
    }
}

void uart_write_wrp(uint8_t uart, uint8_t* in, size_t bytes)
{
    for (int i = 0; i < bytes; ++i) {
        volatile uint8_t data = {0};
        memcpy(&data, in + i, 1);

        uart_write(uart, data);
    }
}