#include "table.h"

#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SYMS_LENGTH 64
#define REFS_LENGTH 128
#define LABELS_LENGTH 1024

table *table_init() {
    table *t = calloc(1, sizeof(table));

    t->syms_length = SYMS_LENGTH;
    t->syms = calloc(SYMS_LENGTH, sizeof(symbol));
    t->syms_end = t->syms;

    t->refs_length = REFS_LENGTH;
    t->refs = calloc(REFS_LENGTH, sizeof(reference));
    t->refs_end = t->refs;

    t->labels_length = LABELS_LENGTH;
    t->labels = calloc(LABELS_LENGTH, sizeof(char));
    t->labels_end = t->labels;

    return t;
}

void table_resize_syms(table *t, size_t offset) {
    t->syms = realloc(t->syms, t->syms_length * 2);

    if (!t->syms) {
        fprintf(stderr, "error: could not allocate storage");
        exit(EXIT_FAILURE);
    }

    t->syms_length = t->syms_length * 2;
    t->syms_end = t->syms + offset * sizeof(symbol);
}

void table_resize_refs(table *t, size_t offset) {
    t->refs = realloc(t->refs, t->refs_length * 2);

    if (!t->refs) {
        fprintf(stderr, "error: could not allocate storage");
        exit(EXIT_FAILURE);
    }

    t->refs_length = t->refs_length * 2;
    t->refs_end = t->refs + offset * sizeof(reference);
}

void table_resize_labels(table *t, size_t offset) {
    t->labels = realloc(t->labels, t->labels_length * 2);

    if (!t->labels) {
        fprintf(stderr, "error: could not allocate storage");
        exit(EXIT_FAILURE);
    }

    t->labels_length = t->labels_length * 2;
    t->labels_end = t->labels + offset * sizeof(char);
}

char *table_label_find(table *t, char *label) {
    for (char *l = t->labels; l < t->labels_end; l += strlen(l) + 1) {
        if (strcmp(l, label) == 0) {
            return l;
        }
    }

    return NULL;
}

char *table_label_push(table *t, char *label) {
    size_t len = strlen(label);
    size_t offset = t->labels_end - t->labels;

    if (offset + len > t->labels_length) {
        table_resize_labels(t, offset);
    }

    char *label_start = t->labels_end;
    t->labels_end = stpcpy(t->labels_end, label) + 1;
    return label_start;
}

void table_symbol_push(table *t, symbol *s) {
    size_t offset = (t->syms_end - t->syms) / sizeof(symbol);

    if (offset >= t->syms_length) {
        table_resize_syms(t, offset);
    }

    memcpy(t->syms_end, s, sizeof(symbol));
    t->syms_end += sizeof(symbol);
}

void table_ref_push(table *t, reference *r) {
    size_t offset = t->refs_end - t->refs;

    if (offset >= t->refs_length) {
        table_resize_refs(t, offset);
    }

    memcpy(t->refs_end, r, sizeof(reference));
    t->refs_end += sizeof(reference);
}

reference *table_ref_pop(table *t) {
    reference *r = t->refs_end -= sizeof(reference);
    return r >= t->refs ? r : NULL;
}

void table_ref_add(table *t, char *label, uint8_t *offset) {
    char *loc = table_label_find(t, label);

    if (!loc) {
        loc = table_label_push(t, label);
    }

    reference r = {.offset = offset, .label = loc};
    table_ref_push(t, &r);
}

void table_symbol_define(table *t, char *label, size_t addr) {
    char *loc = table_label_find(t, label);

    if (!loc) {
        loc = table_label_push(t, label);
    }

    symbol s = {.label = loc, .address = addr};
    table_symbol_push(t, &s);
}

symbol *table_symbol_lookup(table *t, char *label) {
    char *loc = table_label_find(t, label);

    for (symbol *s = t->syms; s < t->syms_end; s += sizeof(symbol)) {
        if (s && s->label == loc) {
            return s;
        }
    }

    return NULL;
}

void table_symbol_del(table *t, char *label) {
    char *loc = table_label_find(t, label);

    for (symbol *s = t->syms; s < t->syms_end; s += sizeof(symbol)) {
        if (s && s->label == loc) {
            s->label = NULL;
            return;
        }
    }
}

void table_print(table *t) {
    printf("SYMBOLS\n-------\n");
    for (symbol *s = t->syms; s < t->syms_end; s += sizeof(symbol)) {
        if (s->label) {
            printf("%s: %zu\n", s->label, s->address);
        }
    }

    printf("\nREFERENCES\n----------\n");
    for (reference *r = t->refs; r < t->refs_end; r += sizeof(reference)) {
        if (r->label) {
            printf("%s: %p\n", r->label, r->offset);
        }
    }

    printf("\nLABELS\n------\n");
    for (char *l = t->labels; l < t->labels_end - 1; l += strlen(l) + 1) {
        printf("%s%s", l == t->labels ? "" : ", ", l);
    }
    printf("\n");
}

void table_snprintf(table *t, char *str, size_t n) {
    size_t len;
    len = snprintf(str, n, "SYMBOLS\n-------\n");
    str = str + len;
    n = n - len;

    for (symbol *s = t->syms; s < t->syms_end; s += sizeof(symbol)) {
        if (s->label) {
            len = snprintf(str, n, "%s: %zu\n", s->label, s->address);
            str = str + len;
            n = n - len;
        }
    }

    len = snprintf(str, n, "\nREFERENCES\n----------\n");
    str = str + len;
    n = n - len;

    for (reference *r = t->refs; r < t->refs_end; r += sizeof(reference)) {
        if (r->label) {
            len = snprintf(str, n, "%s: %p\n", r->label, r->offset);
            str = str + len;
            n = n - len;
        }
    }

    len = snprintf(str, n, "\nLABELS\n------\n");
    str = str + len;
    n = n - len;

    for (char *l = t->labels; l < t->labels_end - 1; l += strlen(l) + 1) {
        len = snprintf(str, n, "%s%s", l == t->labels ? "" : ", ", l);
        str = str + len;
        n = n - len;
    }
    snprintf(str, n, "\n");
}

void table_free(table *t) {
    free(t->syms);
    free(t->refs);
    free(t->labels);
    free(t);
}
