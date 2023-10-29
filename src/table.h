#ifndef BBB_TABLE
#define BBB_TABLE

#include <memory.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct sym_ref {
    size_t offset;
    size_t index;
}

typedef struct table {
    size_t size;
    size_t last;
    char **labels;
    uint16_t *addrs;

    sym_ref *references;
} table;

table *table_init(size_t size);
void table_insert(table *t, char *label, uint16_t addr);
bool table_scan(table *t, char *label, uint16_t *result);
void table_del(table *t, char *label);
void table_add_ref(table *t, char *label, size_t offset);
size_t *table_reference_list(table *t);
void table_get_ref void table_free(table *t);

#endif
