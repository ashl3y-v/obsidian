// Hardware Imports
#include "inc/hw_ints.h"   // Interrupt numbers
#include "inc/hw_memmap.h" // Peripheral Base Addresses
#include "inc/hw_types.h"  // Boolean type
#include "inc/lm3s6965.h"  // Peripheral Bit Masks and Registers

// Driver API Imports
#include "driverlib/flash.h"     // FLASH API
#include "driverlib/interrupt.h" // Interrupt API
#include "driverlib/sysctl.h"    // System control API (clock/reset)

// Library Imports
#include <stdbool.h>
#include <string.h>

#include <stdarg.h>


// Application Imports
#include "uart.h"
#include "utility.h"
#include "structures.h"
#include "../crypto/secrets.h"

// Forward Declarations
void load_initial_firmware(void);
void load_firmware(void);
void boot_firmware(void);
long program_flash(uint32_t, unsigned char*, unsigned int);

// Firmware Constants
#define METADATA_BASE                                                          \
    0xFC00              // base address of version and firmware size in Flash
#define FW_BASE 0x10000 // base address of firmware in Flash

// FLASH Constants
#define FLASH_PAGESIZE 1024
#define FLASH_WRITESIZE 4

// Protocol Constants
#define OK          ((uint16_t)('O'))
#define ERROR       ((uint16_t)('E'))
#define UPDATE      ((uint16_t)('U'))
#define BOOT        ((uint16_t)('B'))
#define META        ((uint16_t)('M'))
#define CHUNK       ((uint16_t)('C'))
#define DONE        ((uint16_t)('D'))
#define FRAME_SIZE  ((uint16_t)(256))

// Firmware v2 is embedded in bootloader
// Read up on these symbols in the objcopy man page (if you want)!
extern int _binary_firmware_bin_start;
extern int _binary_firmware_bin_size;

// Device metadata
uint16_t* fw_version_address = (uint16_t*)METADATA_BASE;
uint16_t* fw_size_address = (uint16_t*)(METADATA_BASE + 2);
uint8_t* fw_release_message_address;


// Program functions
void update_firmware();
metadata load_metadata();

// Setup the bootloader for communication
void init_interfaces()
{
    // The interrupt handler listens to UART0 for RESET interrupts
    uart_init(UART0); 
    IntEnable(INT_UART0);
    IntMasterEnable();

    // Bootloader Interface
    uart_init(UART1); 

    // Bootloader Output
    uart_init(UART2); 

    // load_initial_firmware();
    uart_write_str(UART2, "Obsidian Update Interface\n");
    uart_write_str(UART2, 
                        "Send \"U\" to update, and \"B\" to run the firmware.\n");
    uart_write_str(UART2, "Writing 0x20 to UART0 (space) will reset the device\n");
    uart_write_str(UART2, " ");
}


int main(void) 
{
    init_interfaces();

    int read;
    while (true)
    {
        uint16_t request = uart_read(UART1, BLOCKING, &read);
        switch (request)
        {
            case UPDATE:
                uart_write(UART1, OK);
                uart_write_str(UART2, "Received a request to update firmware.\n");
                update_firmware();
                break;
            case BOOT:
                uart_write_str(UART2, "Received a request to boot firmware.\n");
                break;
        }
    }
}

metadata load_metadata()
{
    int read;
    while (true)
    {
        uint16_t request = uart_read(UART1, BLOCKING, &read);
        if (request == META)
            break;
    }
    
    // Acknowledge that we are about to receive metadata
    uart_write_str(UART2, "META packet received on bootloader.\n");
    uart_write(UART1, OK);

    // Metadata stuff 
    metadata mdata = {};
    uart_read_wrp(UART1, BLOCKING, &read, &(mdata.version), sizeof(uint16_t));

    char buffer[5];
    itoa(mdata.version, buffer, 10);
    uart_write_str(UART2, "Firmware version received: ");
    uart_write_str(UART2, buffer);
    nl(UART2);

    uart_read_wrp(UART1, BLOCKING, &read, &(mdata.size), sizeof(uint16_t));
    itoa(mdata.size, buffer, 10);
    uart_write_str(UART2, "Firmware size received: ");
    uart_write_str(UART2, buffer);
    nl(UART2);

    uart_read_wrp(UART1, BLOCKING, &read, &(mdata.message_size), sizeof(uint16_t));
    itoa(mdata.message_size, buffer, 10);
    uart_write_str(UART2, "Release message size received: ");
    uart_write_str(UART2, buffer);
    nl(UART2);

    uart_write_wrp(UART1, &(mdata.version), sizeof(uint16_t));
    uart_write_wrp(UART1, &(mdata.size), sizeof(uint16_t));
    uart_write_wrp(UART1, &(mdata.message_size), sizeof(uint16_t));

    return mdata;
}

void update_firmware()
{
    metadata mdata = load_metadata();
    if (!mdata.size)
    {
        uart_write_str(UART2, "Something went wrong trying to load the metadata; restarting device\n");
        SysCtlReset();
    }

    // Wait for firmware to be sent
    while (true)
    {
        uint16_t request = uart_read(UART1, BLOCKING, &read);
        if (request == CHUNK)
            break;
    }
    
    // Acknowledge that we are about to receive firmware
    uart_write_str(UART2, "CHUNK packet received on bootloader.\n");
    uart_write(UART1, OK);

    // whhhhoooooo here we go
    uint16_t frame_length = 0;
    
    uint32_t rcv = 0;
    uint32_t data_index = 0;
    uint32_t page_addr = FW_BASE;

    while (1){

        // Get the frame length
        uart_read_wrp(UART1, BLOCKING, &read, &frame_length, 2);

        // Get the frame bytes
        for (int i = 0; i < frame_length; ++i) {
            data[data_index] = uart_read(UART1, BLOCKING, &read);
            data_index += 1;
            uart_write(UART1, OK); // Ack
        }
        

        // If we filed our page buffer, program it
        if (data_index == FLASH_PAGESIZE || frame_length == 0) {
            uart_write_str(UART1, "new flash");
            if (!frame_length) {
                uart_write_str(UART2, "Got zero length frame.\n");
            }

            // Try to write flash and check for error
            if (program_flash(page_addr, data, data_index)) {
                uart_write(UART1, ERROR); 
                SysCtlReset();            // goodbye device kek
                return;
            }

            // Verify flash program
            if (memcmp(data, (void*)page_addr, data_index) != 0) {
                uart_write_str(UART2, "Flash check failed, aborting.\n");
                uart_write(UART1, ERROR); 
                SysCtlReset();            // goodbye device kek
                return;
            }
            
            uart_write_str(UART2, "Page successfully programmed\nAddress: ");
            uart_write_hex(UART2, page_addr);
            uart_write_str(UART2, "\nBytes: ");
            uart_write_hex(UART2, data_index);
            nl(UART2);


            // Update to next page
            page_addr += FLASH_PAGESIZE;
            data_index = 0;

            // If at end of firmware, go to main
            if (frame_length == 0) {
                uart_write(UART1, OK);
                uart_write_str(UART2, "end of the firm");
                break;
            }
        } 
        //
        
       
        
        
    }
        
}

/*
int main(void) {
    // A 'reset' on UART0 will re-start this code at the top of main, won't
    // clear flash, but will clean ram.

    // Initialize UART channels
    // 0: Reset
    // 1: Host Connection
    // 2: Debug
    uart_init(UART0);
    uart_init(UART1);
    uart_init(UART2);

    // Enable UART0 interrupt
    IntEnable(INT_UART0);
    IntMasterEnable();

    load_initial_firmware(); // note the short-circuit behavior in this
                             // function, it doesn't finish running on reset!

    uart_write_str(UART2, "Welcome to the BWSI Vehicle Update Service!\n");
    uart_write_str(UART2,
                   "Send \"U\" to update, and \"B\" to run the firmware.\n");
    uart_write_str(UART2, "Writing 0x20 to UART0 will reset the device.\n");

    int resp;
    while (1) {
        uint32_t instruction = uart_read(UART1, BLOCKING, &resp);
        if (instruction == UPDATE) {
            uart_write_str(UART1, "U");
            load_firmware();
            uart_write_str(UART2, "Loaded new firmware.\n");
            nl(UART2);
        } else if (instruction == BOOT) {
            uart_write_str(UART1, "B");
            boot_firmware();
        }
    }
}

/*
void load_initial_firmware(void) {

    if (*((uint32_t*)(METADATA_BASE)) != 0xFFFFFFFF) {
       
        return;
    }

    // Create buffers for saving the release message
    uint8_t temp_buf[FLASH_PAGESIZE];
    char initial_msg[] = "amogus";
    uint16_t msg_len = strlen(initial_msg) + 1;
    uint16_t rem_msg_bytes;

    // Get included initial firmware
    int size = (int)&_binary_firmware_bin_size;
    uint8_t* initial_data = (uint8_t*)&_binary_firmware_bin_start;

    // Set version 2 and install
    uint16_t version = 2;
    uint32_t metadata = (((uint16_t)size & 0xFFFF) << 16) | (version & 0xFFFF);
    program_flash(METADATA_BASE, (uint8_t*)(&metadata), 4);

    int i;

    for (i = 0; i < size / FLASH_PAGESIZE; i++) {
        program_flash(FW_BASE + (i * FLASH_PAGESIZE),
                      initial_data + (i * FLASH_PAGESIZE), FLASH_PAGESIZE);
    }


    uint16_t rem_fw_bytes = size % FLASH_PAGESIZE;
    if (rem_fw_bytes == 0) {
        // No firmware left. Just write the release message
        program_flash(FW_BASE + (i * FLASH_PAGESIZE), (uint8_t*)initial_msg,
                      msg_len);
    } else {
        // Some firmware left. Determine how many bytes of release message can
        // fit
        if (msg_len > (FLASH_PAGESIZE - rem_fw_bytes)) {
            rem_msg_bytes = msg_len - (FLASH_PAGESIZE - rem_fw_bytes);
        } else {
            rem_msg_bytes = 0;
        }

        // Copy rest of firmware
        memcpy(temp_buf, initial_data + (i * FLASH_PAGESIZE), rem_fw_bytes);
        // Copy what will fit of the release message
        memcpy(temp_buf + rem_fw_bytes, initial_msg, msg_len - rem_msg_bytes);
        // Program the final firmware and first part of the release message
        program_flash(FW_BASE + (i * FLASH_PAGESIZE), temp_buf,
                      rem_fw_bytes + (msg_len - rem_msg_bytes));

        // If there are more bytes, program them directly from the release
        // message string
        if (rem_msg_bytes > 0) {
            // Writing to a new page. Increment pointer
            i++;
            program_flash(FW_BASE + (i * FLASH_PAGESIZE),
                          (uint8_t*)(initial_msg + (msg_len - rem_msg_bytes)),
                          rem_msg_bytes);
        }
    }
}


void load_firmware(void) {
    int frame_length = 0;
    int read = 0;
    uint32_t rcv = 0;

    uint32_t data_index = 0;
    uint32_t page_addr = FW_BASE;
    uint32_t version = 0;
    uint32_t size = 0;

    // Get version as 16 bytes
    rcv = uart_read(UART1, BLOCKING, &read);
    version = (uint32_t)rcv;
    rcv = uart_read(UART1, BLOCKING, &read);
    version |= (uint32_t)rcv << 8;

    uart_write_str(UART2, "Received firmware version: ");
    uart_write_hex(UART2, version);
    nl(UART2);

    // Get size as 16 bytes
    rcv = uart_read(UART1, BLOCKING, &read);
    size = (uint32_t)rcv;
    rcv = uart_read(UART1, BLOCKING, &read);
    size |= (uint32_t)rcv << 8;

    uart_write_str(UART2, "Received firmware size: ");
    uart_write_hex(UART2, size);
    nl(UART2);

    // Compare to old version and abort if older (note special case for version
    // 0).
    uint16_t old_version = *fw_version_address;

    if (version != 0 && version < old_version) {
        uart_write(UART1, ERROR); // Reject the metadata.
        SysCtlReset();            // Reset device
        return;
    } else if (version == 0) {
        // If debug firmware, don't change version
        version = old_version;
    }

    // Write new firmware size and version to Flash
    // Create 32 bit word for flash programming, version is at lower address,
    // size is at higher address
    uint32_t metadata = ((size & 0xFFFF) << 16) | (version & 0xFFFF);
    program_flash(METADATA_BASE, (uint8_t*)(&metadata), 4);

    uart_write(UART1, OK); // Acknowledge the metadata.

   
    while (1) {

        // Get two bytes for the length.
        rcv = uart_read(UART1, BLOCKING, &read);
        frame_length = (int)rcv << 8;
        rcv = uart_read(UART1, BLOCKING, &read);
        frame_length += (int)rcv;

        // Get the number of bytes specified
        for (int i = 0; i < frame_length; ++i) {
            data[data_index] = uart_read(UART1, BLOCKING, &read);
            data_index += 1;
        }

        // If we filed our page buffer, program it
        if (data_index == FLASH_PAGESIZE || frame_length == 0) {
            if (frame_length == 0) {
                uart_write_str(UART2, "Got zero length frame.\n");
            }

            // Try to write flash and check for error
            if (program_flash(page_addr, data, data_index)) {
                uart_write(UART1, ERROR); // Reject the firmware
                SysCtlReset();            // Reset device
                return;
            }

            // Verify flash program
            if (memcmp(data, (void*)page_addr, data_index) != 0) {
                uart_write_str(UART2, "Flash check failed.\n");
                uart_write(UART1, ERROR); // Reject the firmware
                SysCtlReset();            // Reset device
                return;
            }
#if 1
            // Write debugging messages to UART2.
            uart_write_str(UART2, "Page successfully programmed\nAddress: ");
            uart_write_hex(UART2, page_addr);
            uart_write_str(UART2, "\nBytes: ");
            uart_write_hex(UART2, data_index);
            nl(UART2);
#endif

            // Update to next page
            page_addr += FLASH_PAGESIZE;
            data_index = 0;

            // If at end of firmware, go to main
            if (frame_length == 0) {
                uart_write(UART1, OK);
                break;
            }
        } // if

        uart_write(UART1, OK); // Acknowledge the frame.
    }                          // while(1)
}
*/

long program_flash(uint32_t page_addr, unsigned char* data,
                   unsigned int data_len) {
    uint32_t word = 0;
    int ret;
    int i;

    // Erase next FLASH page
    FlashErase(page_addr);

    // Clear potentially unused bytes in last word
    // If data not a multiple of 4 (word size), program up to the last word
    // Then create temporary variable to create a full last word
    if (data_len % FLASH_WRITESIZE) {
        // Get number of unused bytes
        int rem = data_len % FLASH_WRITESIZE;
        int num_full_bytes = data_len - rem;

        // Program up to the last word
        ret = FlashProgram((unsigned long*)data, page_addr, num_full_bytes);
        if (ret != 0) {
            return ret;
        }

        // Create last word variable -- fill unused with 0xFF
        for (i = 0; i < rem; i++) {
            word = (word >> 8) |
                   (data[num_full_bytes + i]
                    << 24); // Essentially a shift register from MSB->LSB
        }
        for (i = i; i < 4; i++) {
            word = (word >> 8) | 0xFF000000;
        }

        // Program word
        return FlashProgram(&word, page_addr + num_full_bytes, 4);
    } else {
        // Write full buffer of 4-byte words
        return FlashProgram((unsigned long*)data, page_addr, data_len);
    }
}

void boot_firmware(void) {
    // compute the release message address, and then print it
    uint16_t fw_size = *fw_size_address;
    fw_release_message_address = (uint8_t*)(FW_BASE + fw_size);
    uart_write_str(UART2, (char*)fw_release_message_address);

    // Boot the firmware
    __asm("LDR R0,=0x10001\n\t"
          "BX R0\n\t");
}
