#ifndef BBB_MEM_H
#define BBB_MEM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct memory {
    size_t size;
    uint8_t *data;
} memory;

memory *memory_init (size_t size);

uint8_t memory_read (memory *mem, size_t address);
bool memory_write (memory *mem, size_t address, uint8_t value);

void memory_free (memory *mem);

#endif
