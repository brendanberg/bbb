#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "../machine/cpu.h"
#include "../munit/munit.h"

static int test_cpu_call_setup_count = 0;

static void test_cpu_call_setup_handler(machine *m) {
    test_cpu_call_setup_count += 1;
}

static int test_cpu_call_update_count = 0;

static void test_cpu_call_update_handler(machine *m) {
    test_cpu_call_update_count += 1;
}

static int test_cpu_call_teardown_count = 0;

static void test_cpu_call_teardown_handler(machine *m) {
    test_cpu_call_teardown_count += 1;
}

static void *test_cpu_setup(const MunitParameter params[], void *fixture) {
    test_cpu_call_setup_count = 0;
    test_cpu_call_update_count = 0;
    test_cpu_call_teardown_count = 0;

    machine *m = machine_init(CPU_MAX_ADDRESS);
    return (void *)m;
}

static void test_cpu_tear_down(void *fixture) {
    machine *m = (machine *)fixture;
    machine_free(m);
}

static MunitResult test_cpu_empty_start(const MunitParameter params[],
                                        void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t *sp = m->sp;
    uint8_t *iv = m->iv;
    uint8_t *ix = m->ix;
    uint8_t *ta = m->ta;
    uint8_t *pc = m->pc;

    machine_start(m);

    // General purpose registers should be zero
    munit_assert_uint8(m->registers[0], ==, 0);
    munit_assert_uint8(m->registers[1], ==, 0);
    munit_assert_uint8(m->registers[2], ==, 0);
    munit_assert_uint8(m->registers[3], ==, 0);
    munit_assert_uint8(m->registers[4], ==, 0);
    munit_assert_uint8(m->registers[5], ==, 0);

    // Status flags should all be 0
    munit_assert_uint8(m->flags, ==, 0x80);

    // Memory registers should not have changed
    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_ptr_equal(m->iv, iv);
    munit_assert_ptr_equal(m->ix, ix);
    munit_assert_ptr_equal(m->ta, ta);
    munit_assert_ptr_equal(m->pc, pc);
    munit_assert_ptr_equal(m->memory->data, m->pc);

    // Memory registers should all point to offset 0
    munit_assert_ptr_equal(m->pc, m->sp);
    munit_assert_ptr_equal(m->pc, m->iv);
    munit_assert_ptr_equal(m->pc, m->ix);
    munit_assert_ptr_equal(m->pc, m->ta);

    // Machine status should be STATE_RUN
    munit_assert_int(m->status, ==, STATE_RUN);

    return MUNIT_OK;
}

static MunitResult test_cpu_immediate_halt(const MunitParameter params[],
                                           void *fixture) {
    machine *m = (machine *)fixture;

    machine_start(m);
    uint8_t *pc = m->pc;
    m->flags |= 0x20;

    machine_run(m);

    munit_assert_ptr_equal(m->sp, pc);
    munit_assert_ptr_equal(m->iv, pc);
    munit_assert_ptr_equal(m->ix, pc);
    munit_assert_ptr_equal(m->ta, pc);
    munit_assert_ptr_equal(m->pc, pc);

    return MUNIT_OK;
}

static MunitResult test_cpu_program_halt(const MunitParameter params[],
                                         void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[24] = {0, 0, 1, 4, 0,  0,           0,           0,
                           0, 0, 0, 0, 0,  0,           0,           0,
                           0, 0, 0, 0, OR, REGISTER_CV, REGISTER_S1, 2};
    memcpy(m->memory->data, program, 24);

    machine_start(m);
    machine_run(m);

    munit_assert_uint8(m->flags, ==, 0xA0);
    munit_assert_ptr_equal(m->pc, m->memory->data + 24);
    return MUNIT_OK;
}

static MunitResult test_cpu_reset(const MunitParameter params[],
                                  void *fixture) {
    machine *m = (machine *)fixture;

    m->registers[0] = munit_rand_int_range(0, 15);
    m->registers[1] = munit_rand_int_range(0, 15);
    m->registers[2] = munit_rand_int_range(0, 15);
    m->registers[3] = munit_rand_int_range(0, 15);
    m->registers[4] = munit_rand_int_range(0, 15);
    m->registers[5] = munit_rand_int_range(0, 15);

    m->flags = munit_rand_int_range(0, 255);

    m->pc = m->memory->data + munit_rand_int_range(0, CPU_MAX_ADDRESS - 1);
    m->sp = m->memory->data + munit_rand_int_range(0, CPU_MAX_ADDRESS - 1);
    m->iv = m->memory->data + munit_rand_int_range(0, CPU_MAX_ADDRESS - 1);
    m->ix = m->memory->data + munit_rand_int_range(0, CPU_MAX_ADDRESS - 1);
    m->ta = m->memory->data + munit_rand_int_range(0, CPU_MAX_ADDRESS - 1);

    machine_reset(m);

    munit_assert_uint8(m->registers[0], ==, 0);
    munit_assert_uint8(m->registers[1], ==, 0);
    munit_assert_uint8(m->registers[2], ==, 0);
    munit_assert_uint8(m->registers[3], ==, 0);
    munit_assert_uint8(m->registers[4], ==, 0);
    munit_assert_uint8(m->registers[5], ==, 0);

    munit_assert_uint8(m->flags, ==, 0x80);

    munit_assert_int(m->status, ==, STATE_HALT);

    return MUNIT_OK;
}

static MunitResult test_cpu_call_setup(const MunitParameter params[],
                                       void *fixture) {
    machine *m = (machine *)fixture;
    m->event_setup = test_cpu_call_setup_handler;

    munit_assert_int(test_cpu_call_setup_count, ==, 0);
    machine_start(m);
    munit_assert_int(test_cpu_call_setup_count, ==, 1);

    return MUNIT_OK;
}

static MunitResult test_cpu_call_update(const MunitParameter params[],
                                        void *fixture) {
    machine *m = (machine *)fixture;
    m->event_update = test_cpu_call_update_handler;

    uint8_t program[24] = {0, 0, 1, 4, 0,  0,           0,           0,
                           0, 0, 0, 0, 0,  0,           0,           0,
                           0, 0, 0, 0, OR, REGISTER_CV, REGISTER_S1, 2};
    memcpy(m->memory->data, program, 24);

    munit_assert_int(test_cpu_call_update_count, ==, 0);
    machine_start(m);
    machine_run(m);
    munit_assert_int(test_cpu_call_update_count, ==, 3);

    return MUNIT_OK;
}

static MunitResult test_cpu_call_teardown(const MunitParameter params[],
                                          void *fixture) {
    machine *m = (machine *)fixture;
    m->event_teardown = test_cpu_call_teardown_handler;

    uint8_t program[24] = {0, 0, 1, 4, 0,  0,           0,           0,
                           0, 0, 0, 0, 0,  0,           0,           0,
                           0, 0, 0, 0, OR, REGISTER_CV, REGISTER_S1, 2};
    memcpy(m->memory->data, program, 24);

    munit_assert_int(test_cpu_call_teardown_count, ==, 0);
    machine_start(m);
    machine_run(m);
    munit_assert_int(test_cpu_call_teardown_count, ==, 0);
    machine_free(m);
    munit_assert_int(test_cpu_call_teardown_count, ==, 1);

    return MUNIT_OK;
}

// static MunitResult test_cpu_interrupt(const MunitParameter params[], void
// *fixture) {
//     machine *m = (machine *)fixture;

// }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static MunitTest machine_cpu_tests[] = {
    {(char *)"start with empty image", test_cpu_empty_start, test_cpu_setup,
     test_cpu_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"will not run if halt flag is set", test_cpu_immediate_halt,
     test_cpu_setup, test_cpu_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"will halt by setting flag", test_cpu_program_halt, test_cpu_setup,
     test_cpu_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"reset clears registers", test_cpu_reset, test_cpu_setup,
     test_cpu_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"calling machine_setup calls event_setup callback",
     test_cpu_call_setup, test_cpu_setup, test_cpu_tear_down,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"calling machine_run calls event_update callback",
     test_cpu_call_update, test_cpu_setup, test_cpu_tear_down,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"calling machine_free calls event_teardown callback",
     test_cpu_call_teardown, test_cpu_setup, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
#pragma GCC diagnostic pop
