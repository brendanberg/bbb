#ifndef BBB_TABLE
#define BBB_TABLE

#include <memory.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct symbol {
    long label_offset;
    size_t address;
} symbol;

typedef struct reference {
    long label_offset;
    uint8_t *prog_offset;
} reference;

typedef struct table {
    char *labels;
    char *labels_end;
    size_t labels_length;

    symbol *syms;
    uint32_t syms_count;
    size_t syms_length;

    reference *refs;
    reference *refs_end;
    uint32_t refs_count;
    size_t refs_length;
} table;

table *table_init();

char *table_get_label(table *t, long offset);

void table_symbol_define(table *t, char *label, size_t addr);
symbol *table_symbol_lookup(table *t, char *label);
symbol *table_symbol_lookup_offset(table *t, long offset);
void table_symbol_del(table *t, char *label);

void table_ref_push(table *t, reference *r);
reference *table_ref_pop(table *t);
void table_ref_add(table *t, char *label, uint8_t *offset);

void table_print(table *t);

void table_free(table *t);

#endif
