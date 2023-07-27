#include <stdint.h>

#define SIGNATURE_SIZE 64

typedef struct _metadata
{
    uint8_t signature[SIGNATURE_SIZE];
    volatile uint16_t version;
    volatile uint16_t  size;
    volatile uint16_t message_size;
} metadata;