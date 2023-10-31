#ifndef BBB_TABLE
#define BBB_TABLE

#include <memory.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct symbol {
    char *label;
    size_t address;
} symbol;

typedef struct reference {
    uint8_t *offset;
    char *label;
} reference;

typedef struct table {
    symbol *syms;
    symbol *syms_end;
    size_t syms_length;

    reference *refs;
    reference *refs_end;
    size_t refs_length;

    char *labels;
    char *labels_end;
    size_t labels_length;
} table;

table *table_init();

void table_symbol_define(table *t, char *label, size_t addr);
symbol *table_symbol_lookup(table *t, char *label);
void table_symbol_del(table *t, char *label);

void table_ref_push(table *t, reference *r);
reference *table_ref_pop(table *t);
void table_ref_add(table *t, char *label, uint8_t *offset);

void table_print(table *t);

void table_free(table *t);

#endif
