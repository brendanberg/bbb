#include "memory.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

memory *memory_init(size_t size) {
    memory *mem = malloc(sizeof(memory));
    mem->size = size;
    mem->data = calloc(size, sizeof(uint8_t));
    return mem;
}

uint8_t memory_read(memory *mem, size_t address) {
    if (address >= mem->size) {
        return 0;
    }

    return mem->data[address];
}

uint8_t memory_read_indexed(memory *mem, uint8_t *index, size_t offset) {
    size_t address = ((index - mem->data) + offset) % mem->size;

    if (address >= mem->size) {
        return 0;
    }

    return mem->data[address];
}

bool memory_write(memory *mem, size_t address, uint8_t value) {
    if (address >= mem->size) {
        return false;
    }

    mem->data[address] = value;
    return true;
};

bool memory_write_indexed(memory *mem, uint8_t *index, size_t offset,
                          uint8_t value) {
    size_t address = ((index - mem->data) + offset) % mem->size;

    if (address >= mem->size) {
        return false;
    }

    mem->data[address] = value;
    return true;
}

void memory_free(memory *mem) {
    free(mem->data);
    free(mem);
};
