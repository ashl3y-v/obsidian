#include <stdint.h>

#define SIGNATURE_SIZE 64

typedef struct _metadata
{
    uint8_t signature[SIGNATURE_SIZE];
    uint16_t version;
    uint16_t size;
    uint16_t message_size;
} metadata;