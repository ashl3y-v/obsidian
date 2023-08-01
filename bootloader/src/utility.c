#include "utility.h"

#define ERROR (uint8_t)('E')

// This function is called whenever the device fails some part of the update process
void reject()
{
    // Re-enable interrupts if they were disabled
    IntMasterEnable();

    // We failed, oh no...
    uart_write(UART1, ERROR); 
    SysCtlReset();
}

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
        byte_str[1] = tolower(right_nibble);
        if (left_nibble > 9) {
            left_nibble += 0x37;
        } else {
            left_nibble += 0x30;
        }
        byte_str[0] = tolower(left_nibble);
        byte_str[2] = '\0';

        uart_write_str(uart, byte_str);
        uart_write_str(uart, " "); // uncomment if you want space between your bytes
    }
}



void uart_read_wrp(uint8_t uart, int blocking, int* read, uint8_t* out, size_t bytes)
{
    // Hope that the out variable is a valid pointer and read a byte
    // UART register only holds up to a byte so we have to repeat this operation X times
    for (int i = 0; i < bytes; ++i) {
        volatile uint8_t data = uart_read(uart, blocking , read);
        memcpy(out + i, (uint8_t*)(&data), 1);
    }
}

void uart_write_wrp(uint8_t uart, uint8_t* in, size_t bytes)
{
    // Same thing as uart_read_wrp except we write...
    for (int i = 0; i < bytes; ++i) {
        volatile uint8_t data = {0};
        memcpy(&data, in + i, 1);

        uart_write(uart, data);
    }
}