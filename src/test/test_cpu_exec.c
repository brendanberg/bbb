#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "../machine/cpu.h"
#include "../munit/munit.h"
#include "./test_cpu.h"

static void *test_cpu_exec_setup(const MunitParameter params[], void *fixture) {
    machine *m = machine_init(CPU_MAX_ADDRESS);
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

    return (void *)m;
}

static void test_cpu_exec_tear_down(void *fixture) {
    machine *m = (machine *)fixture;
    machine_free(m);
}

static void machine_step(machine *m) {
    machine_instr_fetch(m);
    machine_instr_decode(m);
    machine_instr_execute(m);
    machine_interrupt_check(m);
}

static MunitResult test_cpu_exec_nop(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[] = {NOP};
    memcpy(m->memory->data, program, 1);

    machine_step(m);

    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 1);
    return MUNIT_OK;
}

static MunitResult test_cpu_exec_inc(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t inc_a[] = {INC, REGISTER_A};
    memcpy(m->memory->data, inc_a, 2);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 2);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    // TODO Should flags update on INC / DEC?
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 2);

    machine_reset(m);

    uint8_t inc_sp[] = {INC, REGISTER_SP};
    memcpy(m->memory->data, inc_sp, 2);
    munit_assert_ptr_equal(m->sp, m->memory->data);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, m->memory->data + 1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 2);

    machine_reset(m);

    uint8_t inc_md[] = {INC, REGISTER_MD, 0x1, 0x0, 0x0, 0x0};
    memcpy(m->memory->data, inc_md, 6);
    munit_assert_uint8(m->memory->data[0x1000], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x1000], ==, 1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 6);

    machine_reset(m);

    m->ix += 0xF000;
    uint8_t inc_mx[] = {INC, REGISTER_MX, 0x0, 0x0, 0x0, 0xF};
    memcpy(m->memory->data, inc_mx, 6);
    munit_assert_uint8(m->ix[0x000F], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->ix[0x000F], ==, 1);
    munit_assert_uint8(m->memory->data[0xF00F], ==, 1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 6);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_dec(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t dec_a[] = {DEC, REGISTER_A};
    memcpy(m->memory->data, dec_a, 2);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 2);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xE);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 2);

    machine_reset(m);

    uint8_t dec_sp[] = {DEC, REGISTER_SP};
    memcpy(m->memory->data, dec_sp, 2);
    munit_assert_ptr_equal(m->sp, m->memory->data);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, m->memory->data - 1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 2);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_add(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t add_a_b[] = {ADD, REGISTER_A, REGISTER_B};
    memcpy(m->memory->data, add_a_b, 3);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0x1;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x1);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);
    // munit_assert_uint8(m->flags, ==, 0x8A);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1002;
    uint8_t *sp = m->sp;
    m->registers[REGISTER_F] = 0x8;
    uint8_t add_sp_f[] = {ADD, REGISTER_SP, REGISTER_F};
    memcpy(m->memory->data, add_sp_f, 3);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0x8);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0xA);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1234;
    sp = m->sp;
    m->registers[REGISTER_C] = 0x6;
    uint8_t add_c_sp[] = {ADD, REGISTER_C, REGISTER_SP};
    memcpy(m->memory->data, add_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);
    munit_assert_ptr_equal(m->sp, sp + 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    uint8_t add_cv_d[] = {ADD, REGISTER_CV, REGISTER_D, 0x3};
    m->registers[REGISTER_D] = 0x2;
    memcpy(m->memory->data, add_cv_d, 4);
    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x2);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x5);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 4);

    machine_reset(m);

    uint8_t add_cv_md[] = {ADD, REGISTER_CV, REGISTER_MD, 0x7,
                           0x2, 0x0,         0x0,         0x0};
    m->memory->data[0x2000] = 0x8;
    memcpy(m->memory->data, add_cv_md, 8);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x8);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t add_cv_mx[] = {ADD, REGISTER_CV, REGISTER_MX, 0x7,
                           0x0, 0x0,         0x2,         0x0};
    m->ix += 0x2000;
    m->memory->data[0x2020] = 0x8;
    memcpy(m->memory->data, add_cv_mx, 8);
    munit_assert_uint8(m->memory->data[0x2020], ==, 0x8);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2020], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t add_md_md[] = {ADD, REGISTER_MD, REGISTER_MD, 0x2, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->memory->data[0x2000] = 0x5;
    m->memory->data[0x2001] = 0x6;
    memcpy(m->memory->data, add_md_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x6);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xB);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t add_md_mx[] = {ADD, REGISTER_MD, REGISTER_MX, 0x2, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0xA;
    m->memory->data[0x2001] = 0x4;
    memcpy(m->memory->data, add_md_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0xA);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x4);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0xA);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xE);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t add_mx_md[] = {ADD, REGISTER_MX, REGISTER_MD, 0x0, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0xA;
    m->memory->data[0x2001] = 0x4;
    memcpy(m->memory->data, add_mx_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0xA);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x4);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0xA);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xE);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t add_mx_mx[] = {ADD, REGISTER_MX, REGISTER_MX, 0x0, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x2};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0xA;
    m->memory->data[0x2002] = 0x4;
    memcpy(m->memory->data, add_mx_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0xA);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0x4);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0xA);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xE);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_sub(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t sub_a_b[] = {SUB, REGISTER_A, REGISTER_B};
    memcpy(m->memory->data, sub_a_b, 3);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0x0;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x1);
    // munit_assert_uint8(m->flags, ==, 0x8A);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1002;
    uint8_t *sp = m->sp;
    m->registers[REGISTER_F] = 0x8;
    uint8_t sub_sp_f[] = {SUB, REGISTER_SP, REGISTER_F};
    memcpy(m->memory->data, sub_sp_f, 3);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0x8);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1234;
    sp = m->sp;
    m->registers[REGISTER_C] = 0x6;
    uint8_t sub_c_sp[] = {SUB, REGISTER_C, REGISTER_SP};
    memcpy(m->memory->data, sub_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);
    munit_assert_ptr_equal(m->sp, sp - 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    uint8_t sub_cv_d[] = {SUB, REGISTER_CV, REGISTER_D, 0x3};
    m->registers[REGISTER_D] = 0x5;
    memcpy(m->memory->data, sub_cv_d, 4);
    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x5);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x2);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 4);

    machine_reset(m);

    uint8_t sub_cv_md[] = {SUB, REGISTER_CV, REGISTER_MD, 0x7,
                           0x2, 0x0,         0x0,         0x0};
    m->memory->data[0x2000] = 0x8;
    memcpy(m->memory->data, sub_cv_md, 8);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x8);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t sub_cv_mx[] = {SUB, REGISTER_CV, REGISTER_MX, 0x7,
                           0x0, 0x0,         0x2,         0x0};
    m->ix += 0x2000;
    m->memory->data[0x2020] = 0x8;
    memcpy(m->memory->data, sub_cv_mx, 8);
    munit_assert_uint8(m->memory->data[0x2020], ==, 0x8);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2020], ==, 0x1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t sub_md_md[] = {SUB, REGISTER_MD, REGISTER_MD, 0x2, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->memory->data[0x2000] = 0x6;
    m->memory->data[0x2001] = 0xB;
    memcpy(m->memory->data, sub_md_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x6);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xB);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x6);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x5);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t sub_md_mx[] = {SUB, REGISTER_MD, REGISTER_MX, 0x2, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, sub_md_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t sub_mx_md[] = {SUB, REGISTER_MX, REGISTER_MD, 0x0, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, sub_mx_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t sub_mx_mx[] = {SUB, REGISTER_MX, REGISTER_MX, 0x0, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x2};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2002] = 0xA;
    memcpy(m->memory->data, sub_mx_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_rlc(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[] = {NOP};
    memcpy(m->memory->data, program, 1);

    machine_step(m);

    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 1);
    return MUNIT_OK;
}

static MunitResult test_cpu_exec_rrc(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[] = {NOP};
    memcpy(m->memory->data, program, 1);

    machine_step(m);

    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 1);
    return MUNIT_OK;
}

static MunitResult test_cpu_exec_and(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t and_a_b[] = {AND, REGISTER_A, REGISTER_B};
    memcpy(m->memory->data, and_a_b, 3);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0x0;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x100F;
    uint8_t *sp = m->sp;
    m->registers[REGISTER_F] = 0xA;
    uint8_t and_sp_f[] = {AND, REGISTER_SP, REGISTER_F};
    memcpy(m->memory->data, and_sp_f, 3);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0xA);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0xA);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    sp = m->sp;
    m->sp += 0x1234;
    m->registers[REGISTER_C] = 0xF;
    uint8_t and_c_sp[] = {AND, REGISTER_C, REGISTER_SP};
    memcpy(m->memory->data, and_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xF);
    munit_assert_ptr_equal(m->sp, sp + 0x1234);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    sp = m->sp;
    m->sp += 0x1234;
    m->registers[REGISTER_C] = 0xB;
    memcpy(m->memory->data, and_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xB);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xB);
    munit_assert_ptr_equal(m->sp, sp + 0x1230);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    uint8_t and_cv_d[] = {AND, REGISTER_CV, REGISTER_D, 0x3};
    m->registers[REGISTER_D] = 0x5;
    memcpy(m->memory->data, and_cv_d, 4);
    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x5);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x1);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 4);

    machine_reset(m);

    uint8_t and_cv_md[] = {AND, REGISTER_CV, REGISTER_MD, 0x7,
                           0x2, 0x0,         0x0,         0x0};
    m->memory->data[0x2000] = 0xE;
    memcpy(m->memory->data, and_cv_md, 8);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0xE);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t and_cv_mx[] = {AND, REGISTER_CV, REGISTER_MX, 0x7,
                           0x0, 0x0,         0x2,         0x0};
    m->ix += 0x2000;
    m->memory->data[0x2020] = 0xE;
    memcpy(m->memory->data, and_cv_mx, 8);
    munit_assert_uint8(m->memory->data[0x2020], ==, 0xE);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2020], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t and_md_md[] = {AND, REGISTER_MD, REGISTER_MD, 0x2, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->memory->data[0x2000] = 0x7;
    m->memory->data[0x2001] = 0xE;
    memcpy(m->memory->data, and_md_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xE);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t and_md_mx[] = {AND, REGISTER_MD, REGISTER_MX, 0x2, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x7;
    m->memory->data[0x2001] = 0xE;
    memcpy(m->memory->data, and_md_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xE);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t and_mx_md[] = {AND, REGISTER_MX, REGISTER_MD, 0x0, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x7;
    m->memory->data[0x2001] = 0xE;
    memcpy(m->memory->data, and_mx_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xE);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t and_mx_mx[] = {AND, REGISTER_MX, REGISTER_MX, 0x0, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x2};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x7;
    m->memory->data[0x2002] = 0xE;
    memcpy(m->memory->data, and_mx_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xE);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_or(const MunitParameter params[],
                                    void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t or_a_b[] = {OR, REGISTER_A, REGISTER_B};
    memcpy(m->memory->data, or_a_b, 3);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0x0;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0x0;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0x0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0x0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1003;
    uint8_t *sp = m->sp;
    m->registers[REGISTER_F] = 0xC;
    uint8_t or_sp_f[] = {OR, REGISTER_SP, REGISTER_F};
    memcpy(m->memory->data, or_sp_f, 3);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0xC);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    sp = m->sp;
    m->sp += 0x1233;
    m->registers[REGISTER_C] = 0xC;
    uint8_t or_c_sp[] = {OR, REGISTER_C, REGISTER_SP};
    memcpy(m->memory->data, or_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xC);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xC);
    munit_assert_ptr_equal(m->sp, sp + 0x123F);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    uint8_t or_cv_d[] = {OR, REGISTER_CV, REGISTER_D, 0x3};
    m->registers[REGISTER_D] = 0xC;
    memcpy(m->memory->data, or_cv_d, 4);
    munit_assert_uint8(m->registers[REGISTER_D], ==, 0xC);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_D], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 4);

    machine_reset(m);

    uint8_t or_cv_md[] = {OR,  REGISTER_CV, REGISTER_MD, 0x3,
                          0x2, 0x0,         0x0,         0x0};
    m->memory->data[0x2000] = 0xC;
    memcpy(m->memory->data, or_cv_md, 8);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0xC);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t or_cv_mx[] = {OR,  REGISTER_CV, REGISTER_MX, 0x3,
                          0x0, 0x0,         0x2,         0x0};
    m->ix += 0x2000;
    m->memory->data[0x2020] = 0xC;
    memcpy(m->memory->data, or_cv_mx, 8);
    munit_assert_uint8(m->memory->data[0x2020], ==, 0xC);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2020], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t or_md_md[] = {OR,  REGISTER_MD, REGISTER_MD, 0x2, 0x0, 0x0,
                          0x0, 0x2,         0x0,         0x0, 0x1};
    m->memory->data[0x2000] = 0x3;
    m->memory->data[0x2001] = 0xC;
    memcpy(m->memory->data, or_md_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xC);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t or_md_mx[] = {OR,  REGISTER_MD, REGISTER_MX, 0x2, 0x0, 0x0,
                          0x0, 0x0,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x3;
    m->memory->data[0x2001] = 0xC;
    memcpy(m->memory->data, or_md_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xC);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t or_mx_md[] = {OR,  REGISTER_MX, REGISTER_MD, 0x0, 0x0, 0x0,
                          0x0, 0x2,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x3;
    m->memory->data[0x2001] = 0xF;
    memcpy(m->memory->data, or_mx_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t or_mx_mx[] = {OR,  REGISTER_MX, REGISTER_MX, 0x0, 0x0, 0x0,
                          0x0, 0x0,         0x0,         0x0, 0x2};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x3;
    m->memory->data[0x2002] = 0xC;
    memcpy(m->memory->data, or_mx_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xC);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x3);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_xor(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t xor_a_b[] = {XOR, REGISTER_A, REGISTER_B};
    memcpy(m->memory->data, xor_a_b, 3);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0x0;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1005;
    uint8_t *sp = m->sp;
    m->registers[REGISTER_F] = 0xA;
    uint8_t xor_sp_f[] = {XOR, REGISTER_SP, REGISTER_F};
    memcpy(m->memory->data, xor_sp_f, 3);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0xA);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    sp = m->sp;
    m->sp += 0x1235;
    m->registers[REGISTER_C] = 0xA;
    uint8_t xor_c_sp[] = {XOR, REGISTER_C, REGISTER_SP};
    memcpy(m->memory->data, xor_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0xA);
    munit_assert_ptr_equal(m->sp, sp + 0x123F);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    uint8_t xor_cv_d[] = {XOR, REGISTER_CV, REGISTER_D, 0xA};
    m->registers[REGISTER_D] = 0x5;
    memcpy(m->memory->data, xor_cv_d, 4);
    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x5);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_D], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 4);

    machine_reset(m);

    uint8_t xor_cv_md[] = {XOR, REGISTER_CV, REGISTER_MD, 0xA,
                           0x2, 0x0,         0x0,         0x0};
    m->memory->data[0x2000] = 0x5;
    memcpy(m->memory->data, xor_cv_md, 8);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t xor_cv_mx[] = {XOR, REGISTER_CV, REGISTER_MX, 0xA,
                           0x0, 0x0,         0x2,         0x0};
    m->ix += 0x2000;
    m->memory->data[0x2020] = 0x5;
    memcpy(m->memory->data, xor_cv_mx, 8);
    munit_assert_uint8(m->memory->data[0x2020], ==, 0x5);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2020], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t xor_md_md[] = {XOR, REGISTER_MD, REGISTER_MD, 0x2, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->memory->data[0x2000] = 0x5;
    m->memory->data[0x2001] = 0xF;
    memcpy(m->memory->data, xor_md_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t xor_md_mx[] = {XOR, REGISTER_MD, REGISTER_MX, 0x2, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x5;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, xor_md_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t xor_mx_md[] = {XOR, REGISTER_MX, REGISTER_MD, 0x0, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x5;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, xor_mx_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t xor_mx_mx[] = {XOR, REGISTER_MX, REGISTER_MX, 0x0, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x2};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x5;
    m->memory->data[0x2002] = 0xA;
    memcpy(m->memory->data, xor_mx_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x5);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_cmp(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t cmp_a_b[] = {CMP, REGISTER_A, REGISTER_B};
    memcpy(m->memory->data, cmp_a_b, 3);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0x0;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);
    munit_assert_uint8(m->flags, ==, 0x81);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0x0;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0x0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0x0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1002;
    uint8_t *sp = m->sp;
    m->registers[REGISTER_F] = 0x8;
    uint8_t cmp_sp_f[] = {CMP, REGISTER_SP, REGISTER_F};
    memcpy(m->memory->data, cmp_sp_f, 3);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0x8);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0x8);
    munit_assert_uint8(m->flags, ==, 0xA0);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1234;
    sp = m->sp;
    m->registers[REGISTER_C] = 0x6;
    uint8_t cmp_c_sp[] = {CMP, REGISTER_C, REGISTER_SP};
    memcpy(m->memory->data, cmp_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);
    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->flags, ==, 0xA0);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    uint8_t cmp_cv_d[] = {CMP, REGISTER_CV, REGISTER_D, 0x3};
    m->registers[REGISTER_D] = 0x5;
    memcpy(m->memory->data, cmp_cv_d, 4);
    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x5);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x5);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 4);

    machine_reset(m);

    uint8_t cmp_cv_md[] = {CMP, REGISTER_CV, REGISTER_MD, 0x8,
                           0x2, 0x0,         0x0,         0x0};
    m->memory->data[0x2000] = 0x7;
    memcpy(m->memory->data, cmp_cv_md, 8);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);
    munit_assert_uint8(m->flags, ==, 0x81);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t cmp_cv_mx[] = {CMP, REGISTER_CV, REGISTER_MX, 0x8,
                           0x0, 0x0,         0x2,         0x0};
    m->ix += 0x2000;
    m->memory->data[0x2020] = 0x8;
    memcpy(m->memory->data, cmp_cv_mx, 8);
    munit_assert_uint8(m->memory->data[0x2020], ==, 0x8);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2020], ==, 0x8);
    munit_assert_uint8(m->flags, ==, 0x82);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t cmp_md_md[] = {CMP, REGISTER_MD, REGISTER_MD, 0x2, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->memory->data[0x2000] = 0x6;
    m->memory->data[0x2001] = 0xB;
    memcpy(m->memory->data, cmp_md_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x6);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xB);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x6);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xB);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t cmp_md_mx[] = {CMP, REGISTER_MD, REGISTER_MX, 0x2, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, cmp_md_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t cmp_mx_md[] = {CMP, REGISTER_MX, REGISTER_MD, 0x0, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, cmp_mx_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t cmp_mx_mx[] = {CMP, REGISTER_MX, REGISTER_MX, 0x0, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x2};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2002] = 0xA;
    memcpy(m->memory->data, cmp_mx_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xA);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    return MUNIT_OK;
}

static MunitResult test_cpu_exec_psh(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[] = {NOP};
    memcpy(m->memory->data, program, 1);

    machine_step(m);

    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 1);
    return MUNIT_OK;
}

static MunitResult test_cpu_exec_pop(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[] = {NOP};
    memcpy(m->memory->data, program, 1);

    machine_step(m);

    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 1);
    return MUNIT_OK;
}

static MunitResult test_cpu_exec_jmp(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[] = {NOP};
    memcpy(m->memory->data, program, 1);

    machine_step(m);

    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 1);
    return MUNIT_OK;
}

static MunitResult test_cpu_exec_jsr(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;
    uint8_t program[] = {NOP};
    memcpy(m->memory->data, program, 1);

    machine_step(m);

    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 1);
    return MUNIT_OK;
}

static MunitResult test_cpu_exec_mov(const MunitParameter params[],
                                     void *fixture) {
    machine *m = (machine *)fixture;

    uint8_t mov_a_b[] = {MOV, REGISTER_A, REGISTER_B};
    memcpy(m->memory->data, mov_a_b, 3);
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0xF;
    m->registers[REGISTER_B] = 0x0;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0xF);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->registers[REGISTER_A] = 0x0;
    m->registers[REGISTER_B] = 0xF;
    munit_assert_uint8(m->registers[REGISTER_A], ==, 0x0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0xF);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_A], ==, 0x0);
    munit_assert_uint8(m->registers[REGISTER_B], ==, 0x0);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    m->sp += 0x1002;
    uint8_t *sp = m->sp;
    m->registers[REGISTER_F] = 0x8;
    uint8_t mov_sp_f[] = {MOV, REGISTER_SP, REGISTER_F};
    memcpy(m->memory->data, mov_sp_f, 3);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0x8);

    machine_step(m);

    munit_assert_ptr_equal(m->sp, sp);
    munit_assert_uint8(m->registers[REGISTER_F], ==, 0x2);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    sp = m->sp;
    m->sp += 0x1234;
    m->registers[REGISTER_C] = 0x6;
    uint8_t mov_c_sp[] = {MOV, REGISTER_C, REGISTER_SP};
    memcpy(m->memory->data, mov_c_sp, 3);
    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_C], ==, 0x6);
    munit_assert_ptr_equal(m->sp, sp + 0x1236);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 3);

    machine_reset(m);

    uint8_t mov_cv_d[] = {MOV, REGISTER_CV, REGISTER_D, 0x3};
    m->registers[REGISTER_D] = 0x5;
    memcpy(m->memory->data, mov_cv_d, 4);
    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x5);

    machine_step(m);

    munit_assert_uint8(m->registers[REGISTER_D], ==, 0x3);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 4);

    machine_reset(m);

    uint8_t mov_cv_md[] = {MOV, REGISTER_CV, REGISTER_MD, 0x8,
                           0x2, 0x0,         0x0,         0x0};
    m->memory->data[0x2000] = 0x7;
    memcpy(m->memory->data, mov_cv_md, 8);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x7);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x8);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t mov_cv_mx[] = {MOV, REGISTER_CV, REGISTER_MX, 0xF,
                           0x0, 0x0,         0x2,         0x0};
    m->ix += 0x2000;
    m->memory->data[0x2020] = 0xA;
    memcpy(m->memory->data, mov_cv_mx, 8);
    munit_assert_uint8(m->memory->data[0x2020], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2020], ==, 0xF);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 8);

    machine_reset(m);

    uint8_t mov_md_md[] = {MOV, REGISTER_MD, REGISTER_MD, 0x2, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->memory->data[0x2000] = 0x6;
    m->memory->data[0x2001] = 0xB;
    memcpy(m->memory->data, mov_md_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x6);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xB);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x6);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x6);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t mov_md_mx[] = {MOV, REGISTER_MD, REGISTER_MX, 0x2, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, mov_md_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x4);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t mov_mx_md[] = {MOV, REGISTER_MX, REGISTER_MD, 0x0, 0x0, 0x0,
                           0x0, 0x2,         0x0,         0x0, 0x1};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2001] = 0xA;
    memcpy(m->memory->data, mov_mx_md, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2001], ==, 0x4);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    machine_reset(m);

    uint8_t mov_mx_mx[] = {MOV, REGISTER_MX, REGISTER_MX, 0x0, 0x0, 0x0,
                           0x0, 0x0,         0x0,         0x0, 0x2};
    m->ix += 0x2000;
    m->memory->data[0x2000] = 0x4;
    m->memory->data[0x2002] = 0xA;
    memcpy(m->memory->data, mov_mx_mx, 11);
    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0xA);

    machine_step(m);

    munit_assert_uint8(m->memory->data[0x2000], ==, 0x4);
    munit_assert_uint8(m->memory->data[0x2002], ==, 0x4);
    munit_assert_uint8(m->flags, ==, 0x80);
    munit_assert_ptr_equal(m->pc, m->memory->data + 11);

    return MUNIT_OK;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static MunitTest machine_cpu_exec_tests[] = {
    {(char *)"NOP executes correctly", test_cpu_exec_nop, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"INC executes correctly", test_cpu_exec_inc, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"ADD executes correctly", test_cpu_exec_add, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"SUB executes correctly", test_cpu_exec_sub, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    // RLC
    // RRC
    {(char *)"AND executes correctly", test_cpu_exec_and, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)" OR executes correctly", test_cpu_exec_or, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"XOR executes correctly", test_cpu_exec_xor, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"CMP executes correctly", test_cpu_exec_cmp, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    // PSH
    // POP
    // JMP
    // JSR
    {(char *)"MOV executes correctly", test_cpu_exec_mov, test_cpu_exec_setup,
     test_cpu_exec_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
#pragma GCC diagnostic pop
