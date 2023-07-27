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
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Application Imports
#include "../crypto/secrets.h"
#include "structures.h"
#include "utility.h"

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
#define OK ((uint16_t)('O'))
#define ERROR ((uint16_t)('E'))
#define UPDATE ((uint16_t)('U'))
#define BOOT ((uint16_t)('B'))
#define META ((uint16_t)('M'))
#define CHUNK ((uint16_t)('C'))
#define DONE ((uint16_t)('D'))
#define FRAME_SIZE ((uint16_t)(256))

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

unsigned char data[FLASH_PAGESIZE];

// Setup the bootloader for communication
void init_interfaces() {

    // The interrupt handler listens to UART0 for RESET interrupts
    uart_init(UART0);
    IntEnable(INT_UART0);
    IntMasterEnable();

    // UART1 is used for input
    uart_init(UART1);

    // UART2 is used for output
    uart_init(UART2);

    // We need something to boot to so we use a firmware embedded in the
    // bootloader
    load_initial_firmware();

    // Setup dialogue
    uart_write_str(UART2, "Obsidian Update Interface\n");
    uart_write_str(UART2,
                   "Send \"U\" to update, and \"B\" to run the firmware.\n");
    uart_write_str(UART2,
                   "Writing 0x20 to UART0 (space) will reset the device\n");
    uart_write_str(UART2, " ");
}

int main(void) {
    init_interfaces();

    int read;
    while (true) {
        uint16_t request = uart_read(UART1, BLOCKING, &read);
        switch (request) {
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

metadata load_metadata() {
    // Wait until we receive a metadata header
    int read;
    while (true) {
        uint16_t request = uart_read(UART1, BLOCKING, &read);
        if (request == META)
            break;
    }

    // Acknowledge that we are about to receive metadata
    uart_write_str(UART2, "[META] Received packet\n");
    uart_write(UART1, OK);

    // Start reading in information about our metadata
    metadata mdata = {0};
    uart_read_wrp(UART1, BLOCKING, &read, mdata.signature, SIGNATURE_SIZE);
    uart_read_wrp(UART1, BLOCKING, &read, (uint8_t*)(&mdata.version),
                  sizeof(uint16_t));
    uart_read_wrp(UART1, BLOCKING, &read, (uint8_t*)(&mdata.size),
                  sizeof(uint16_t));
    uart_read_wrp(UART1, BLOCKING, &read, (uint8_t*)(&mdata.message_size),
                  sizeof(uint16_t));

    uart_write_str(UART2, "[META] File Signature: ");
    uart_write_hex_bytes(UART2, mdata.signature, SIGNATURE_SIZE);
    nl(UART2);

    uart_write_str(UART2, "[META] Version: ");

    char buffer[5];
    itoa(mdata.version, buffer, 10);
    uart_write_str(UART2, buffer);
    uart_write_wrp(UART1, (uint8_t*)(&mdata.version), sizeof(uint16_t));
    nl(UART2);

    uart_write_str(UART2, "[META] Size: ");
    itoa(mdata.size, buffer, 10);
    uart_write_str(UART2, buffer);
    uart_write_wrp(UART1, (uint8_t*)(&mdata.size), sizeof(uint16_t));
    nl(UART2);

    uart_write_str(UART2, "[META] Message Size: ");
    itoa(mdata.message_size, buffer, 10);
    uart_write_str(UART2, buffer);
    uart_write_wrp(UART1, (uint8_t*)(&mdata.message_size), sizeof(uint16_t));
    nl(UART2);

    // Prevent rollbacks except for debug binaries
    uint16_t old_version = *fw_version_address;
    if (mdata.version != 0 && mdata.version < old_version) {
        uart_write_str(UART1, "[METADATA] Version not supported\n");
        reject();
    }

    // Bounds checking
    if (mdata.size > MAX_FIRMWARE_SIZE) {
        uart_write_str(UART1, "[METADATA] Firmware size not supported\n");
        reject();
    }

    if (mdata.message_size > MAX_MESSAGE_SIZE) {
        uart_write_str(UART1, "[METADATA] Release message not supported\n");
        reject();
    }

    uart_write_str(UART2, "[METADATA] Loaded metadata succesfully\n");
    return mdata;
}

void update_firmware() {

    unsigned char firmware[UINT16_MAX] = {0};

    // We don't want to proceed if we have no metadata...
    metadata mdata = load_metadata();
    if (!mdata.size) {
        uart_write_str(UART2, "[FIRMWARE] Failed to load firmware\n");
        SysCtlReset();
    }

    // An initalized context is needed for hash functions
    br_sha256_context sha256 = {0};
    br_sha256_init(&sha256);
    uart_write_str(UART2, "[FIRMWARE] Initalized SHA256 hash\n");

    // Update our SHA256 hash with our current metadata
    br_sha256_update(&sha256, &mdata.version, sizeof(uint16_t));
    br_sha256_update(&sha256, &mdata.size, sizeof(uint16_t));
    br_sha256_update(&sha256, &mdata.message_size, sizeof(uint16_t));

    uint8_t hash[32] = {0};
    br_sha256_out(&sha256, hash);

    uart_write_str(UART2, "[FIRMWARE] SHA256 of metadata: ");
    uart_write_hex_bytes(UART2, hash, 32);
    nl(UART2);

    // Wait for firmware header to be sent
    int read;
    while (true) {
        uint16_t request = uart_read(UART1, BLOCKING, &read);
        if (request == CHUNK)
            break;
    }

    // Acknowledge that we are about to receive firmware
    uart_write_str(UART2, "[FIRMWARE] CHUNK packet received\n");
    uart_write(UART1, OK);

    // Wouldn't want your device to reset mid update..
    IntMasterEnable();

    //...... whhhhoooooo here we go
    uint16_t frame_length = 0;
    uint32_t data_index = 0;
    uint32_t page_addr = FW_BASE;

    // Set up decryption stuff

    // Iterate over the size of the firmware
    for (int idx = 0; idx < mdata.size; idx += FRAME_SIZE) {

        // Chunks should be 256 bytes or less
        uart_write_str(UART2, "[FIRMWARE] Waiting for new frame..\n");
        uart_read_wrp(UART1, BLOCKING, &read, (uint8_t*)(&frame_length), 2);
        uart_write_str(UART2, "[FIRMWARE] Frame received\n");

        // Update the current SHA256 hash with the data we just received
        uart_read_wrp(UART1, BLOCKING, &read, data + data_index, frame_length);
        br_sha256_update(&sha256, data + data_index, frame_length);
        data_index += frame_length;

        // memcpy(firmware + data_index, &(data[data_index]), frame_length);

        // If we filed our page buffer, program it
        if (data_index == FLASH_PAGESIZE - 1 || frame_length == 0) {

            uart_write_str(UART1, "New flash page.");
            if (!frame_length) {
                uart_write_str(UART2, "Got zero length frame.\n");
            }

            // Try to write flash and check for error
            if (program_flash(page_addr, data, data_index)) {
                uart_write(UART1, ERROR);
                SysCtlReset(); // goodbye device kek
                return;
            }

            // Verify flash program
            if (memcmp(data, (void*)page_addr, data_index) != 0) {
                uart_write_str(UART2, "Flash check failed, aborting.\n");
                uart_write(UART1, ERROR);
                SysCtlReset(); // goodbye device kek
                return;
            }

            uart_write_str(UART2, "Page successfully programmed at address ");
            uart_write_hex(UART2, page_addr);
            uart_write_str(UART2, "\nBytes: ");
            uart_write_hex(UART2, data_index);
            nl(UART2);

            // Update to next page
            page_addr += FLASH_PAGESIZE;
            data_index = 0;
        }

        // Let fw_update.py know that we've received the packet and processed it
        uart_write(UART1, OK);

        // If it ran out
        if (frame_length == 0) {
            uart_write(UART1, OK);
            uart_write_str(UART2, "End of firmware reached.");
            break;
        }
    }
    br_sha256_out(&sha256, hash);

    uart_write_str(UART2, "[FIRMWARE] Calculated SHA256 Signature: ");
    uart_write_hex_bytes(UART2, hash, 32);
    nl(UART2);

    if (br_ecdsa_i31_vrfy_raw(&br_ec_p256_m31, hash, 32, &EC_PUBLIC,
                              &mdata.signature, SIGNATURE_SIZE))
        uart_write_str(UART2, "[VERIFICATION] ECC Verification Passed\n");
    else
        uart_write_str(UART2, "[VERIFICATION] ECC Verification Failed\n");

    uart_write(UART1, OK);
    uart_write_str(UART2, "Finished writing firmware.\n");

    
}

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
