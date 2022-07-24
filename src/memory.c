#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "memory.h"


void memory_init (memory *mem, size_t size) {
    mem = malloc(1);
    mem->size = size;
    mem->data = malloc(size);
}

uint8_t memory_read (memory *mem, size_t address) {
    if (address > mem->size) {
        return 0;
    }

    return mem->data[address];
}
bool memory_write (memory *mem, size_t address, uint8_t value) {
    if (address > mem->size) {
        return false;
    }

    mem->data[address] = value;
    return true;
};

void memory_free (memory *mem) {
    free(mem->data);
    free(mem);
};
