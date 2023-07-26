#include <stdint.h>

typedef struct _metadata
{
    volatile uint16_t version;
    volatile uint16_t  size;
    volatile uint16_t message_size;
} metadata;