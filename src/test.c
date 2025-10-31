#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit/munit.h"
#include "test/test_assem.c"
#include "test/test_build.c"
#include "test/test_cpu.c"
#include "test/test_cpu_exec.c"
#include "test/test_memory.c"
#include "test/test_table.c"

MunitSuite suites[] = { // Comment here to force formatting
    {(char *)"assem/table: ", assem_table_tests, NULL, 1,
     MUNIT_SUITE_OPTION_NONE},
    {(char *)"assem/assem: ", assem_assem_tests, NULL, 1,
     MUNIT_SUITE_OPTION_NONE},
    {(char *)"assem/build: ", assem_build_tests, NULL, 1,
     MUNIT_SUITE_OPTION_NONE},
    {(char *)"machine/memory: ", machine_memory_tests, NULL, 1,
     MUNIT_SUITE_OPTION_NONE},
    {(char *)"machine/cpu: ", machine_cpu_tests, NULL, 1,
     MUNIT_SUITE_OPTION_NONE},
    {(char *)"machine/cpu_exec: ", machine_cpu_exec_tests, NULL, 1,
     MUNIT_SUITE_OPTION_NONE},
    {NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE}};

static const MunitSuite test_suite = {
    // prefix, test_suites[], other_suites[], iterations, options
    (char *)"bbb/", NULL, suites, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
    /* Finally, we'll actually run our test suite!  That second argument
     * is the user_data parameter which will be passed either to the
     * test or (if provided) the fixture setup function. */
    return munit_suite_main(&test_suite, (void *)"bbb", argc, argv);
}
