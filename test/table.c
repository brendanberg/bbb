#include "../src/table.c"
#include <stdio.h>
#include <stdlib.h>

#define MAX_SYMBOLS 1024
#define MAX_LABELS 2048

static void test_table_label_resize(void **state) {
    table *t = table_init();

    size_t prev_length = t->labels_length;
    char *prev_end = t->labels_end;
    long prev_offset = t->labels_end - t->labels;
    table_resize_labels(t);
    assert_true(prev_length < t->labels_length);
    assert_ptr_not_equal(prev_end, t->labels_end);
    assert_int_equal(prev_offset, t->labels_end - t->labels);

    table_free(t);
    (void)state;
}

static void test_table_label_push(void **state) {
    table *t = table_init();

    for (uint32_t i = 0; i < MAX_LABELS; i++) {
        char *buffer = calloc(10, sizeof(char));
        sprintf(buffer, "%05d", i);
        table_label_push(t, buffer);
        free(buffer);
    }

    for (uint32_t i = 0; i < MAX_LABELS; i++) {
        char *buffer = calloc(10, sizeof(char));
        sprintf(buffer, "%05d", i);
        long offset = table_label_find(t, buffer);
        char *label = table_label(t, offset);
        assert_non_null(label);
        assert_string_equal(buffer, label);
        free(buffer);
    }

    table_free(t);
    (void)state;
}

static void test_table_symbol_resize(void **state) {
    table *t = table_init();

    size_t prev_length = t->syms_length;
    uint32_t prev_count = t->syms_count;
    table_resize_syms(t);
    assert_true(prev_length < t->syms_length);
    assert_int_equal(prev_count, t->syms_count);
    table_free(t);
    (void)state;
}

static void test_table_symbol_push(void **state) {
    table *t = table_init();
    assert_int_equal(t->syms_count, 0);

    for (uint32_t i = 0; i < MAX_SYMBOLS; i++) {
        char *buffer = calloc(10, sizeof(char));
        sprintf(buffer, "%05d", i);
        uint32_t prev_count = t->syms_count;
        long loc = table_label_push(t, buffer);
        free(buffer);

        symbol s = {.label_offset = loc, .address = i};
        table_symbol_push(t, &s);
        assert_int_equal(t->syms_count, prev_count + 1);
    }

    for (uint32_t i = 0; i < MAX_SYMBOLS; i++) {
        char *buffer = calloc(10, sizeof(char));
        sprintf(buffer, "%05d", i);
        long offset = table_label_find(t, buffer);
        char *label = table_label(t, offset);
        assert_non_null(label);
        assert_string_equal(buffer, label);
        free(buffer);
    }

    table_free(t);
    (void)state;
}

static void test_table_symbol_define(void **state) {
    table *t = table_init();

    for (uint32_t i = 0; i < MAX_SYMBOLS; i++) {
        char *buffer = calloc(10, sizeof(char));
        sprintf(buffer, "%05d", i);
        table_symbol_define(t, buffer, i);
        free(buffer);
    }

    // table_print(t);
    for (uint32_t i = 0; i < MAX_SYMBOLS; i++) {
        char *buffer = calloc(10, sizeof(char));
        sprintf(buffer, "%05d", i);
        symbol *sym = table_symbol_lookup(t, buffer);
        char *label = table_label(t, sym->label_offset);
        assert_non_null(sym);
        assert_string_equal(buffer, label);
        free(buffer);
    }

    table_free(t);
    (void)state;
}
