#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "../assem/assem.h"
#include "../munit/munit.h"

#define ASSEM_MAX_ADDRESS (64 * 1024)

// static void *test_memory_setup(const MunitParameter params[], void *fixture)
// {
//     memory *m = memory_init(ASSEM_MAX_ADDRESS);
//     return (void *)m;
// }

// static void test_memory_tear_down(void *fixture) {
//     memory *m = (memory *)fixture;
//     memory_free(m);
// }

static MunitResult test_build_image_trivial_case(const MunitParameter params[],
                                                 void *fixture) {
    memory *m = build_image("", "");

    for (size_t i = 0; i < m->size; i++) {
        munit_assert_uint8(m->data[i], ==, 0);
    }

    memory_free(m);
    return MUNIT_OK;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static MunitTest assem_build_tests[] = {
    {(char *)"trivial case builds correctly", test_build_image_trivial_case,
     NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
#pragma GCC diagnostic pop
