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

    t->labels_length = LABELS_LENGTH;
    t->labels_end = t->labels = calloc(LABELS_LENGTH, sizeof(char));

    t->syms_length = SYMS_LENGTH;
    t->syms = calloc(SYMS_LENGTH, sizeof(symbol));
    t->syms_count = 0;

    if (t->syms == NULL) {
        fprintf(stderr, "error: could not allocate storage\n");
        exit(EXIT_FAILURE);
    }

    t->refs_length = REFS_LENGTH;
    t->refs = calloc(REFS_LENGTH, sizeof(reference));
    t->refs_count = 0;

    if (t->refs == NULL) {
        fprintf(stderr, "error: could not allocate storage\n");
        exit(EXIT_FAILURE);
    }

    return t;
}

void table_resize_syms(table *t) {
    size_t new_len = t->syms_length * 2;
    symbol *new = calloc(new_len, sizeof(symbol));

    if (new == NULL) {
        fprintf(stderr, "error: could not allocate storage\n");
        exit(EXIT_FAILURE);
    }

    symbol *result = memcpy(new, t->syms, t->syms_length * sizeof(symbol));

    if (result == NULL) {
        fprintf(stderr, "error: could not copy symbols\n");
        exit(EXIT_FAILURE);
    }

    free(t->syms);
    t->syms = new;
    t->syms_length = new_len;
}

void table_resize_refs(table *t) {
    size_t new_len = t->refs_length * 2;
    reference *new = calloc(new_len, sizeof(reference));

    if (new == NULL) {
        fprintf(stderr, "error: could not allocate storage\n");
        exit(EXIT_FAILURE);
    }

    reference *result = memcpy(new, t->refs, t->refs_length * sizeof(reference));

    if (result == NULL) {
        fprintf(stderr, "error: could not copy references\n");
        exit(EXIT_FAILURE);
    }

    free(t->refs);
    t->refs = new;
    t->refs_length = new_len;
}

void table_resize_labels(table *t) {
    size_t new_len = t->labels_length * 2;
    long offset = t->labels_end - t->labels;
    char *labels = calloc(new_len, sizeof(char));

    if (labels == NULL) {
        fprintf(stderr, "error: could not allocate storage\n");
        exit(EXIT_FAILURE);
    }

    char *result = memcpy(labels, t->labels, t->labels_length * sizeof(char));

    if (result == NULL) {
        fprintf(stderr, "error: could not copy labels\n");
        exit(EXIT_FAILURE);
    }

    free(t->labels);
    t->labels = labels;
    t->labels_end = t->labels + offset;
    t->labels_length = new_len;
}

long table_label_find(table *t, char *label) {
    for (char *l = t->labels; l < t->labels_end; l += strlen(l) + 1) {
        if (strcmp(l, label) == 0) {
            return l - t->labels;
        }
    }

    return -1;
}

char *table_get_label(table *t, long offset) { return t->labels + offset; }

long table_label_push(table *t, char *label) {
    size_t len = strlen(label);
    long offset = t->labels_end - t->labels;

    if ((size_t)offset + len > t->labels_length) {
        table_resize_labels(t);
    }

    char *label_start = t->labels_end;
    char *end = stpcpy(t->labels_end, label);

    if (end == NULL) {
        fprintf(stderr, "error: unable to copy label\n");
        exit(EXIT_FAILURE);
    }

    t->labels_end = end + 1;

    return label_start - t->labels;
}

void table_symbol_push(table *t, symbol *s) {
    if (t->syms_count >= t->syms_length) {
        table_resize_syms(t);
    }

    symbol *dst = t->syms + t->syms_count;
    symbol *result = memcpy(dst, s, sizeof(symbol));

    if (result == NULL) {
        fprintf(stderr, "error: unable to copy symbol to symbol table\n");
    }

    t->syms_count++;
}

void table_ref_push(table *t, reference *r) {
    if (t->refs_count >= t->refs_length) {
        table_resize_refs(t);
    }

    reference *dst = t->refs + t->refs_count;
    reference *result = memcpy(dst, r, sizeof(reference));

    if (result == NULL) {
        fprintf(stderr, "error: unable to copy reference to table\n");
    }

    t->refs_count++;
}

reference *table_ref_pop(table *t) {
    if (t->refs_count == 0) {
        return NULL;
    }

    reference *r = t->refs + t->refs_count - 1;
    t->refs_count--;

    return r;
}

void table_ref_add(table *t, char *label, uint8_t *offset) {
    long loc = table_label_find(t, label);

    if (loc < 0) {
        loc = table_label_push(t, label);
    }

    reference r = {.label_offset = loc, .prog_offset = offset};
    table_ref_push(t, &r);
}

void table_symbol_define(table *t, char *label, size_t addr) {
    long loc = table_label_find(t, label);

    if (loc < 0) {
        loc = table_label_push(t, label);
    }

    if (loc < 0) {
        fprintf(stderr, "error: unable to push label\n");
        exit(EXIT_FAILURE);
    }

    symbol s = {.label_offset = loc, .address = addr};
    table_symbol_push(t, &s);
}

symbol *table_symbol_lookup(table *t, char *label) {
    long loc = table_label_find(t, label);

    if (loc < 0) {
        printf("couldn't find label\n");
        return NULL;
    }

    for (uint32_t i = 0; i < t->syms_count; i++) {
        symbol *s = t->syms + i;
        if (s->label_offset == loc) {
            return s;
        }
    }

    printf("couldn't find symbol\n");
    return NULL;
}

symbol *table_symbol_lookup_offset(table *t, long offset) {
    long loc = table_label_find(t, t->labels + offset);

    if (loc < 0) {
        printf("couldn't find label\n");
        return NULL;
    }

    for (uint32_t i = 0; i < t->syms_count; i++) {
        symbol *s = t->syms + i;
        if (s->label_offset == loc) {
            return s;
        }
    }

    printf("couldn't find symbol\n");
    return NULL;
}

void table_symbol_del(table *t, char *label) {
    long loc = table_label_find(t, label);

    if (loc < 0) {
        return;
    }

    for (uint32_t i = 0; i < t->syms_count; i++) {
        symbol *s = t->syms + i;
        if (s->label_offset == loc) {
            s->label_offset = -1;
            return;
        }
    }
}

void table_print(table *t) {
    printf("LABELS\n------\n");
    for (char *l = t->labels; l < t->labels_end; l += strlen(l) + 1) {
        printf("%s%s", l == t->labels ? "" : ", ", l);
    }

    printf("\n");

    printf("\nREFERENCES\n----------\n");

    for (uint32_t i = 0; i < t->refs_count; i++) {
        reference *r = t->refs + i;
        if (r->label_offset >= 0) {
            char *label = table_get_label(t, r->label_offset);
            printf("%s: %p\n", label, r->prog_offset);
        }
    }

    printf("\nSYMBOLS\n-------\n");
    for (uint32_t i = 0; i < t->syms_count; i++) {
        symbol *s = t->syms + i;
        if (s->label_offset >= 0) {
            char *label = table_get_label(t, s->label_offset);
            printf("%s: %04zu\n", label, s->address);
        }
    }

    printf("\n");
}

void table_free(table *t) {
    free(t->syms);
    free(t->refs);
    free(t->labels);
    free(t);
}
