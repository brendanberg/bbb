#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "../machine/memory.h"
#include "../munit/munit.h"

#define MAX_ADDRESS (64 * 1024)

static MunitResult test_memory_init(const MunitParameter params[],
                                    void *fixture) {
    memory *m = (memory *)fixture;

    for (uint8_t *i = m->data; i < m->data + m->size; i += 1) {
        munit_assert_uint8(*i, ==, 0);
    }

    return MUNIT_OK;
}

static MunitResult test_memory_read_write(const MunitParameter params[],
                                          void *fixture) {
    memory *m = (memory *)fixture;

    for (int i = 0; i < 1024; i++) {
        size_t addr = munit_rand_int_range(0, MAX_ADDRESS - 1);
        uint8_t write = munit_rand_int_range(0, 255);

        memory_write(m, addr, write);

        uint8_t read = memory_read(m, addr);
        munit_assert_uint8(write, ==, read);
    }

    return MUNIT_OK;
}

static MunitResult test_memory_read_write_indexed(const MunitParameter params[],
                                                  void *fixture) {
    memory *m = (memory *)fixture;
    uint8_t *index = m->data;

    for (int i = 0; i < 1024; i++) {
        size_t s = munit_rand_int_range(0, MAX_ADDRESS - 1);

        for (int j = 0; j < 1024; j++) {
            size_t offset = munit_rand_int_range(0, MAX_ADDRESS - 1 - s);
            uint8_t write = munit_rand_int_range(0, 255);

            memory_write_indexed(m, index + s, offset, write);

            uint8_t read = memory_read_indexed(m, index + s, offset);
            munit_assert_uint8(write, ==, read);
        }
    }

    return MUNIT_OK;
}

static MunitResult
test_memory_out_of_bounds_read_write(const MunitParameter params[],
                                     void *fixture) {
    memory *m = (memory *)fixture;

    for (int i = 0; i < 1024; i++) {
        size_t addr = munit_rand_int_range(MAX_ADDRESS, MAX_ADDRESS * 2);
        uint8_t write = munit_rand_int_range(0, 255);

        munit_assert_ulong(m->size, <=, addr);
        memory_write(m, addr, write);

        uint8_t read = memory_read(m, addr);
        munit_assert_uint8(0, ==, read);
    }

    return MUNIT_OK;
}

static void *test_memory_setup(const MunitParameter params[], void *fixture) {
    memory *m = memory_init(MAX_ADDRESS);
    munit_assert_size(m->size, ==, MAX_ADDRESS);
    return (void *)m;
}

static void test_memory_tear_down(void *fixture) {
    memory *m = (memory *)fixture;
    memory_free(m);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static MunitTest machine_memory_tests[] = {
    {(char *)"initialize", test_memory_init, test_memory_setup,
     test_memory_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"read and write", test_memory_read_write, test_memory_setup,
     test_memory_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"indexed R/W", test_memory_read_write_indexed, test_memory_setup,
     test_memory_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"out of bounds R/W", test_memory_out_of_bounds_read_write,
     test_memory_setup, test_memory_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
#pragma GCC diagnostic pop
