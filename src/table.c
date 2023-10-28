#include "table.h"

#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

table *table_init(size_t size) {
    table *t = calloc(1, sizeof(table));
    t->size = size;
    t->last = 0;
    t->labels = calloc(size, sizeof(char *));
    t->addrs = calloc(size, sizeof(uint16_t));

    return t;
}

bool table_resize(table *t, size_t size) {
    if (size <= t->size) {
        return false;
    }

    char **labels = calloc(size, sizeof(char *));
    uint16_t *addrs = calloc(size, sizeof(uint16_t));

    memcpy(labels, t->labels, t->size);
    memcpy(addrs, t->addrs, t->size);

    free(t->labels);
    free(t->addrs);

    t->labels = labels;
    t->addrs = addrs;

    return true;
}

void table_insert(table *t, char *label, uint16_t addr) {
    if (t->last == t->size - 1) {
        table_resize(t, t->size * 2);
    }

    char *insert = calloc(strlen(label) + 1, sizeof(char));
    memcpy(insert, label, strlen(label));

    t->labels[t->last] = insert;
    t->addrs[t->last] = addr;
    t->last += 1;

    return;
}

bool table_scan(table *t, char *label, uint16_t *result) {
    for (size_t i = 0; i < t->size; i++) {
        if (t->labels[i] && strcmp(label, t->labels[i]) == 0) {
            *result = i;
            return true;
        }
    }

    return false;
}

void table_del(table *t, char *label) {
    uint16_t index;
    bool found = table_scan(t, label, &index);

    if (!found) {
        return;
    }

    free(t->labels[index]);
    t->labels[index] = NULL;
}

void table_free(table *t) {
    for (size_t i = 0; i < t->size; i++) {
        if (t->labels[i]) {
            free(t->labels[i]);
        }
    }
    free(t->labels);
    free(t->addrs);
    free(t);
}
