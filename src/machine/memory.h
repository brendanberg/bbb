#ifndef BBB_MEM_H
#define BBB_MEM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct memory {
    size_t size;
    uint8_t *data;
} memory;

memory *memory_init(size_t size);

uint8_t memory_read(memory *mem, size_t address);
uint8_t memory_read_indexed(memory *mem, uint8_t *index, size_t offset);

bool memory_write(memory *mem, size_t address, uint8_t value);
bool memory_write_indexed(memory *mem, uint8_t *index, size_t offset,
                          uint8_t value);

void memory_free(memory *mem);

#endif
