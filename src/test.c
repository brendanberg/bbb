#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit/munit.h"
#include "test/test_table.c"

static const MunitSuite test_suite = {
    // prefix, test_suites[], other_suites[], iterations, options
    (char *)"assem/table: ", assem_table_tests, NULL, 1,
    MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    /* Finally, we'll actually run our test suite!  That second argument
     * is the user_data parameter which will be passed either to the
     * test or (if provided) the fixture setup function. */
    return munit_suite_main(&test_suite, (void *)"bbb", argc, argv);
}
