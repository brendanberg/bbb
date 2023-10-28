#ifndef BBB_TABLE
#define BBB_TABLE

#include <memory.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct table {
    size_t size;
    size_t last;
    char **labels;
    uint16_t *addrs;
} table;

table *table_init(size_t size);
void table_insert(table *t, char *label, uint16_t addr);
bool table_scan(table *t, char *label, uint16_t *result);
void table_del(table *t, char *label);
void table_free(table *t);

#endif
