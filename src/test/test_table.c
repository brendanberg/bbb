#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "../assem/table.h"
#include "../munit/munit.h"

static MunitResult test_symbol_lookup(const MunitParameter params[],
                                      void *fixture) {
    table *t = (table *)fixture;
    symbol *s = NULL;

    // Test looking up an undefined symbol
    s = table_symbol_lookup(t, "undefined");
    munit_assert_null(s);

    // Test defining a symbol with an empty string
    table_symbol_define(t, "", 1);
    munit_assert_ptr_equal(t->syms_end - 1, t->syms);

    s = table_symbol_lookup(t, "");
    munit_assert_string_equal(s->label, "");
    munit_assert_ulong(s->address, ==, 1);

    // Test defining a symbol with a non-empty string
    table_symbol_define(t, "label", 2);
    munit_assert_ptr_equal(t->syms_end - 2, t->syms);

    s = table_symbol_lookup(t, "label");
    munit_assert_string_equal(s->label, "label");
    munit_assert_ulong(s->address, ==, 2);

    // Test redefining a symbol with a previously defined label
    table_symbol_define(t, "label", 3);
    munit_assert_ptr_equal(t->syms_end - 3, t->syms);
    // TODO: fix symbol redefinition

    s = table_symbol_lookup(t, "label");
    munit_assert_string_equal(s->label, "label");
    // munit_assert_ulong(s->address, ==, 3);

    munit_assert_ptr_equal(t->syms + 3, t->syms_end);
    return MUNIT_OK;
}

static MunitResult test_symbol_delete(const MunitParameter params[],
                                      void *fixture) {
    table *t = (table *)fixture;
    symbol *s = NULL;

    // Test deleting a non-existant label
    s = table_symbol_lookup(t, "label");
    munit_assert_null(s);

    table_symbol_define(t, "label", 3);
    s = table_symbol_lookup(t, "label");
    munit_assert_string_equal(s->label, "label");
    munit_assert_uint(s->address, ==, 3);
    table_symbol_del(t, "label");
    s = table_symbol_lookup(t, "label");
    munit_assert_null(s);

    return MUNIT_OK;
}

static MunitResult test_symbol_resize(const MunitParameter params[],
                                      void *fixture) {
    table *t = (table *)fixture;

    char *label = malloc(100 * sizeof(char));
    int length = t->syms_length;

    for (int i = 0; i < length; i++) {
        snprintf(label, 100, "%d", i);
        table_symbol_define(t, label, i);
        munit_assert_ulong(t->syms_length, ==, length);
    }

    for (int j = 0; j < length; j++) {
        snprintf(label, 100, "%d", 64 + j);
        table_symbol_define(t, label, 64 + j);
        munit_assert_ulong(t->syms_length, ==, length * 2);
    }

    snprintf(label, 100, "%d", 128);
    table_symbol_define(t, label, 128);
    munit_assert_ulong(t->syms_length, ==, length * 4);

    return MUNIT_OK;
}

static MunitResult test_ref_add(const MunitParameter params[], void *fixture) {
    table *t = (table *)fixture;
    reference *r = NULL;

    char *string = "abcdefghijklmnopqrstuvwxyz";
    table_ref_add(t, "label", (uint8_t *)string);
    munit_assert_ptr_equal(t->refs + 1, t->refs_end);

    r = table_ref_pop(t);
    munit_assert_string_equal(r->label, "label");
    munit_assert_ptr_equal(r->offset, (uint8_t *)(string));
    munit_assert_ptr_equal(t->refs, t->refs_end);

    table_ref_add(t, "a", (uint8_t *)(string + 1));
    munit_assert_ptr_equal(t->refs + 1, t->refs_end);

    table_ref_add(t, "b", (uint8_t *)(string + 2));
    munit_assert_ptr_equal(t->refs + 2, t->refs_end);

    table_ref_add(t, "c", (uint8_t *)(string + 3));
    munit_assert_ptr_equal(t->refs + 3, t->refs_end);

    r = table_ref_pop(t);
    munit_assert_string_equal(r->label, "c");
    munit_assert_ptr_equal(r->offset, (uint8_t *)(string + 3));
    munit_assert_ptr_equal(t->refs + 2, t->refs_end);

    r = table_ref_pop(t);
    munit_assert_string_equal(r->label, "b");
    munit_assert_ptr_equal(r->offset, (uint8_t *)(string + 2));
    munit_assert_ptr_equal(t->refs + 1, t->refs_end);

    r = table_ref_pop(t);
    munit_assert_string_equal(r->label, "a");
    munit_assert_ptr_equal(r->offset, (uint8_t *)(string + 1));
    munit_assert_ptr_equal(t->refs, t->refs_end);

    return MUNIT_OK;
}

static MunitResult test_table_print(const MunitParameter params[],
                                    void *fixture) {
    table *t = (table *)fixture;
    size_t length = 1024;
    char *buffer = malloc(length * sizeof(char));

    table_snprintf(t, buffer, length);

    char *dest = malloc(length * sizeof(char));
    strncpy(dest, buffer, 16);
    dest[16] = '\0';
    munit_assert_string_equal(dest, "SYMBOLS\n-------\n");

    strncpy(dest, buffer + 17, 22);
    dest[22] = '\0';
    munit_assert_string_equal(dest, "REFERENCES\n----------\n");

    strncpy(dest, buffer + 40, 14);
    dest[14] = '\0';
    munit_assert_string_equal(dest, "LABELS\n------\n");

    uint8_t *data = malloc(length * sizeof(uint8_t));
    table_symbol_define(t, "a", 1);
    table_symbol_define(t, "b", 2);
    table_ref_add(t, "q", data);
    table_ref_add(t, "r", data + 1);
    table_ref_add(t, "s", data + 2);

    table_snprintf(t, buffer, length);

    char *test = strtok(buffer, "\n");
    munit_assert_string_equal(test, "SYMBOLS");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "-------");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "a: 1");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "b: 2");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "REFERENCES");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "----------");

    test = strtok(NULL, "\n");
    test[5] = '\0';
    munit_assert_string_equal(test, "q: 0x");

    test = strtok(NULL, "\n");
    test[5] = '\0';
    munit_assert_string_equal(test, "r: 0x");

    test = strtok(NULL, "\n");
    test[5] = '\0';
    munit_assert_string_equal(test, "s: 0x");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "LABELS");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "------");

    test = strtok(NULL, "\n");
    munit_assert_string_equal(test, "a, b, q, r, s");

    free(data);
    free(buffer);
    free(dest);
    return MUNIT_OK;
}

static void *test_table_setup(const MunitParameter params[], void *user_data) {
    table *t = table_init();

    munit_assert_ptr_equal(t->syms, t->syms_end);
    munit_assert_ptr_equal(t->refs, t->refs_end);
    munit_assert_ptr_equal(t->labels, t->labels_end);

    return (void *)t;
}

static void test_table_tear_down(void *fixture) {
    table *t = (table *)fixture;
    table_free(t);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static MunitTest assem_table_tests[] = {
    {(char *)"look up symbol", test_symbol_lookup, test_table_setup,
     test_table_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"delete symbol", test_symbol_delete, test_table_setup,
     test_table_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"resize symbols", test_symbol_resize, test_table_setup,
     test_table_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"add reference", test_ref_add, test_table_setup,
     test_table_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"print table", test_table_print, test_table_setup,
     test_table_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
#pragma GCC diagnostic pop
