#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "../assem/assem.h"
#include "../machine/cpu.h"
#include "../munit/munit.h"
#include "./test_assem.h"

#define ASSEM_MAX_ADDRESS (64 * 1024)

typedef struct assem_fixture {
    memory *mem;
    context *ctx;
} assem_fixture;

static void *test_assem_setup(const MunitParameter params[], void *fixture) {
    assem_fixture *f = calloc(1, sizeof(assem_fixture));
    f->ctx = calloc(1, sizeof(context));
    f->mem = memory_init(ASSEM_MAX_ADDRESS);

    context *c = f->ctx;

    c->data_start = f->mem->data;
    c->data = f->mem->data;
    c->symbols = table_init();
    c->line = 0;

    return (void *)f;
}

static void test_assem_tear_down(void *fixture) {
    assem_fixture *f = (assem_fixture *)fixture;
    memory_free(f->mem);
    table_free(f->ctx->symbols);
    free(f->ctx);
}

static void assert_assem_equiv(context *c, char *line, size_t count,
                               uint8_t *assem) {
    char *prog = strdup(line);
    uint8_t *offset = c->data;
    bool success = tokenize(c, prog, 1);
    munit_assert_true(success);
    munit_assert_ptr_equal(c->data, offset + count);

    for (int i = 0; i < count; i++) {
        munit_assert_uint8(offset[i], ==, assem[i]);
    }

    free(prog);
}

static MunitResult test_assem_empty_string(const MunitParameter params[],
                                           void *fixture) {
    memory *m = build_image("", "");

    for (size_t i = 0; i < m->size; i++) {
        munit_assert_uint8(m->data[i], ==, 0);
    }

    memory_free(m);
    return MUNIT_OK;
}

static MunitResult test_tokenize_empty_string(const MunitParameter params[],
                                              void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    bool success = tokenize(c, "", 0);
    munit_assert_true(success);
    munit_assert_ptr_equal(c->data, c->data_start);

    return MUNIT_OK;
}

static MunitResult test_tokenize_org(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;
    memory *m = ((assem_fixture *)fixture)->mem;

    char *prog;
    bool success;

    prog = strdup("#org 0001");
    success = tokenize(c, prog, 0);
    munit_assert_true(success);

    munit_assert_ptr_equal(c->data, c->data_start + 0x0001);

    free(prog);
    prog = strdup("#org 1000");
    success = tokenize(c, prog, 1);
    munit_assert_true(success);

    munit_assert_ptr_equal(c->data, c->data_start + 0x1000);

    free(prog);
    prog = strdup("#org 00FF");
    success = tokenize(c, prog, 2);
    munit_assert_true(success);

    munit_assert_ptr_equal(c->data, c->data_start + 0x00FF);

    free(prog);

    for (size_t i = 0; i < m->size; i++) {
        munit_assert_uint8(c->data_start[i], ==, 0);
    }

    return MUNIT_OK;
}

static MunitResult test_tokenize_data(const MunitParameter params[],
                                      void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    char *prog;
    bool success;

    prog = strdup("#data 0123 4567 89AB CDEF");
    success = tokenize(c, prog, 0);
    munit_assert_true(success);

    munit_assert_ptr_equal(c->data, c->data_start + 16);

    free(prog);
    prog = strdup("#data FEDC BA98 7654 3210");
    success = tokenize(c, prog, 1);
    munit_assert_true(success);

    munit_assert_ptr_equal(c->data, c->data_start + 32);

    free(prog);

    uint8_t expected[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                            11, 12, 13, 14, 15, 15, 14, 13, 12, 11, 10,
                            9,  8,  7,  6,  5,  4,  3,  2,  1,  0};

    for (size_t i = 0; i < 32; i++) {
        munit_assert_uint8(c->data_start[i], ==, expected[i]);
    }

    return MUNIT_OK;
}

static MunitResult test_tokenize_comment(const MunitParameter params[],
                                         void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    char *prog;
    bool success;

    prog = strdup("(this is a comment)");
    success = tokenize(c, prog, 0);
    munit_assert_true(success);

    munit_assert_ptr_equal(c->data, c->data_start);

    free(prog);
    prog = strdup("NOP");
    success = tokenize(c, prog, 1);
    munit_assert_true(success);

    free(prog);
    prog = strdup("(((((()");
    success = tokenize(c, prog, 2);
    munit_assert_true(success);

    munit_assert_ptr_equal(c->data, c->data_start + 1);

    free(prog);
    return MUNIT_OK;
}

static MunitResult test_tokenize_label(const MunitParameter params[],
                                       void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    char *prog;
    bool success;
    symbol *s;

    prog = strdup("A:");
    success = tokenize(c, prog, 0);
    munit_assert_true(success);

    s = table_symbol_lookup(c->symbols, "A");

    munit_assert_ptr_not_null(s);
    munit_assert_string_equal(s->label, "A");
    munit_assert_long(s->address, ==, 0);
    munit_assert_ptr_equal(c->data, c->data_start);

    free(prog);
    prog = strdup("NOP");
    success = tokenize(c, prog, 1);
    munit_assert_true(success);

    free(prog);
    prog = strdup("ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz:");
    success = tokenize(c, prog, 2);
    munit_assert_true(success);

    s = table_symbol_lookup(
        c->symbols, "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz");

    munit_assert_ptr_not_null(s);
    munit_assert_string_equal(
        s->label, "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz");
    munit_assert_long(s->address, ==, 1);
    munit_assert_ptr_equal(c->data, c->data_start + 1);

    free(prog);
    return MUNIT_OK;
}

static MunitResult test_tokenize_nop(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    uint8_t assem[] = {NOP};

    assert_assem_equiv(c, "NOP", 1, assem);
    assert_assem_equiv(c, "NOP", 1, assem);

    return MUNIT_OK;
}

static MunitResult test_tokenize_inc(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "INC \%a", 2, (uint8_t[]){INC, REGISTER_A});

    assert_assem_equiv(c, "INC \%pc", 2, (uint8_t[]){INC, REGISTER_PC});

    assert_assem_equiv(c, "INC \%sp", 2, (uint8_t[]){INC, REGISTER_SP});

    assert_assem_equiv(c, "INC \%iv", 2, (uint8_t[]){INC, REGISTER_IV});

    assert_assem_equiv(c, "INC \%ix", 2, (uint8_t[]){INC, REGISTER_IX});

    assert_assem_equiv(c, "INC \%ta", 2, (uint8_t[]){INC, REGISTER_TA});

    assert_assem_equiv(c, "INC @ABCD", 6,
                       (uint8_t[]){INC, REGISTER_MD, 0xA, 0xB, 0xC, 0xD});

    assert_assem_equiv(c, "INC *FEDC", 6,
                       (uint8_t[]){INC, REGISTER_MX, 0xF, 0xE, 0xD, 0xC});

    return MUNIT_OK;
}

static MunitResult test_tokenize_dec(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "DEC \%a", 2, (uint8_t[]){DEC, REGISTER_A});

    assert_assem_equiv(c, "DEC \%pc", 2, (uint8_t[]){DEC, REGISTER_PC});

    assert_assem_equiv(c, "DEC @ABCD", 6,
                       (uint8_t[]){DEC, REGISTER_MD, 0xA, 0xB, 0xC, 0xD});

    assert_assem_equiv(c, "DEC *FEDC", 6,
                       (uint8_t[]){DEC, REGISTER_MX, 0xF, 0xE, 0xD, 0xC});

    return MUNIT_OK;
}

static MunitResult test_tokenize_add(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "ADD \%b \%a", 3,
                       (uint8_t[]){ADD, REGISTER_B, REGISTER_A});

    assert_assem_equiv(c, "ADD 15 \%a", 4,
                       (uint8_t[]){ADD, REGISTER_CV, REGISTER_A, 15});

    assert_assem_equiv(
        c, "ADD 0xFEDC \%pc", 7,
        (uint8_t[]){ADD, REGISTER_CV, REGISTER_PC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(
        c, "ADD 10 @FACE", 8,
        (uint8_t[]){ADD, REGISTER_CV, REGISTER_MD, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(
        c, "ADD 10 *FACE", 8,
        (uint8_t[]){ADD, REGISTER_CV, REGISTER_MX, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(c, "ADD @ABCD @FE35", 11,
                       (uint8_t[]){ADD, REGISTER_MD, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "ADD @ABCD *FE35", 11,
                       (uint8_t[]){ADD, REGISTER_MD, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "ADD *ABCD @FE35", 11,
                       (uint8_t[]){ADD, REGISTER_MX, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "ADD *ABCD *FE35", 11,
                       (uint8_t[]){ADD, REGISTER_MX, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    return MUNIT_OK;
}

static MunitResult test_tokenize_sub(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "SUB \%b \%a", 3,
                       (uint8_t[]){SUB, REGISTER_B, REGISTER_A});

    assert_assem_equiv(c, "SUB 15 \%a", 4,
                       (uint8_t[]){SUB, REGISTER_CV, REGISTER_A, 15});

    assert_assem_equiv(
        c, "SUB 0xFEDC \%pc", 7,
        (uint8_t[]){SUB, REGISTER_CV, REGISTER_PC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(
        c, "SUB 10 @FACE", 8,
        (uint8_t[]){SUB, REGISTER_CV, REGISTER_MD, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(
        c, "SUB 10 *FACE", 8,
        (uint8_t[]){SUB, REGISTER_CV, REGISTER_MX, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(c, "SUB @ABCD @FE35", 11,
                       (uint8_t[]){SUB, REGISTER_MD, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "SUB @ABCD *FE35", 11,
                       (uint8_t[]){SUB, REGISTER_MD, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "SUB *ABCD @FE35", 11,
                       (uint8_t[]){SUB, REGISTER_MX, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "SUB *ABCD *FE35", 11,
                       (uint8_t[]){SUB, REGISTER_MX, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    return MUNIT_OK;
}

static MunitResult test_tokenize_rlc(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "RLC \%a", 2, (uint8_t[]){RLC, REGISTER_A});

    // TODO: Is this a legal CPU instruction?
    assert_assem_equiv(c, "RLC \%pc", 2, (uint8_t[]){RLC, REGISTER_PC});

    assert_assem_equiv(c, "RLC @ABCD", 6,
                       (uint8_t[]){RLC, REGISTER_MD, 0xA, 0xB, 0xC, 0xD});

    assert_assem_equiv(c, "RLC *FEDC", 6,
                       (uint8_t[]){RLC, REGISTER_MX, 0xF, 0xE, 0xD, 0xC});

    return MUNIT_OK;
}

static MunitResult test_tokenize_rrc(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "RRC \%a", 2, (uint8_t[]){RRC, REGISTER_A});

    // TODO: Is this a legal CPU instruction?
    assert_assem_equiv(c, "RRC \%pc", 2, (uint8_t[]){RRC, REGISTER_PC});

    assert_assem_equiv(c, "RRC @ABCD", 6,
                       (uint8_t[]){RRC, REGISTER_MD, 0xA, 0xB, 0xC, 0xD});

    assert_assem_equiv(c, "RRC *FEDC", 6,
                       (uint8_t[]){RRC, REGISTER_MX, 0xF, 0xE, 0xD, 0xC});

    return MUNIT_OK;
}

static MunitResult test_tokenize_and(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "AND \%b \%a", 3,
                       (uint8_t[]){AND, REGISTER_B, REGISTER_A});

    assert_assem_equiv(c, "AND 15 \%a", 4,
                       (uint8_t[]){AND, REGISTER_CV, REGISTER_A, 15});

    assert_assem_equiv(
        c, "AND 0xFEDC \%pc", 7,
        (uint8_t[]){AND, REGISTER_CV, REGISTER_PC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(
        c, "AND 10 @FACE", 8,
        (uint8_t[]){AND, REGISTER_CV, REGISTER_MD, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(
        c, "AND 10 *FACE", 8,
        (uint8_t[]){AND, REGISTER_CV, REGISTER_MX, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(c, "AND @ABCD @FE35", 11,
                       (uint8_t[]){AND, REGISTER_MD, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "AND @ABCD *FE35", 11,
                       (uint8_t[]){AND, REGISTER_MD, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "AND *ABCD @FE35", 11,
                       (uint8_t[]){AND, REGISTER_MX, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "AND *ABCD *FE35", 11,
                       (uint8_t[]){AND, REGISTER_MX, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    return MUNIT_OK;
}

static MunitResult test_tokenize_or(const MunitParameter params[],
                                    void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "OR \%b \%a", 3,
                       (uint8_t[]){OR, REGISTER_B, REGISTER_A});

    assert_assem_equiv(c, "OR 15 \%a", 4,
                       (uint8_t[]){OR, REGISTER_CV, REGISTER_A, 15});

    assert_assem_equiv(
        c, "OR 0xFEDC \%pc", 7,
        (uint8_t[]){OR, REGISTER_CV, REGISTER_PC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(
        c, "OR 10 @FACE", 8,
        (uint8_t[]){OR, REGISTER_CV, REGISTER_MD, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(
        c, "OR 10 *FACE", 8,
        (uint8_t[]){OR, REGISTER_CV, REGISTER_MX, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(c, "OR @ABCD @FE35", 11,
                       (uint8_t[]){OR, REGISTER_MD, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "OR @ABCD *FE35", 11,
                       (uint8_t[]){OR, REGISTER_MD, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "OR *ABCD @FE35", 11,
                       (uint8_t[]){OR, REGISTER_MX, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "OR *ABCD *FE35", 11,
                       (uint8_t[]){OR, REGISTER_MX, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    return MUNIT_OK;
}

static MunitResult test_tokenize_xor(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "XOR \%b \%a", 3,
                       (uint8_t[]){XOR, REGISTER_B, REGISTER_A});

    assert_assem_equiv(c, "XOR 15 \%a", 4,
                       (uint8_t[]){XOR, REGISTER_CV, REGISTER_A, 15});

    assert_assem_equiv(
        c, "XOR 0xFEDC \%pc", 7,
        (uint8_t[]){XOR, REGISTER_CV, REGISTER_PC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(
        c, "XOR 10 @FACE", 8,
        (uint8_t[]){XOR, REGISTER_CV, REGISTER_MD, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(
        c, "XOR 10 *FACE", 8,
        (uint8_t[]){XOR, REGISTER_CV, REGISTER_MX, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(c, "XOR @ABCD @FE35", 11,
                       (uint8_t[]){XOR, REGISTER_MD, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "XOR @ABCD *FE35", 11,
                       (uint8_t[]){XOR, REGISTER_MD, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "XOR *ABCD @FE35", 11,
                       (uint8_t[]){XOR, REGISTER_MX, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "XOR *ABCD *FE35", 11,
                       (uint8_t[]){XOR, REGISTER_MX, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    return MUNIT_OK;
}

static MunitResult test_tokenize_cmp(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "CMP \%b \%a", 3,
                       (uint8_t[]){CMP, REGISTER_B, REGISTER_A});

    assert_assem_equiv(c, "CMP 15 \%a", 4,
                       (uint8_t[]){CMP, REGISTER_CV, REGISTER_A, 15});

    assert_assem_equiv(
        c, "CMP 0xFEDC \%pc", 7,
        (uint8_t[]){CMP, REGISTER_CV, REGISTER_PC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(
        c, "CMP 10 @FACE", 8,
        (uint8_t[]){CMP, REGISTER_CV, REGISTER_MD, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(
        c, "CMP 10 *FACE", 8,
        (uint8_t[]){CMP, REGISTER_CV, REGISTER_MX, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(c, "CMP @ABCD @FE35", 11,
                       (uint8_t[]){CMP, REGISTER_MD, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "CMP @ABCD *FE35", 11,
                       (uint8_t[]){CMP, REGISTER_MD, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "CMP *ABCD @FE35", 11,
                       (uint8_t[]){CMP, REGISTER_MX, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "CMP *ABCD *FE35", 11,
                       (uint8_t[]){CMP, REGISTER_MX, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    return MUNIT_OK;
}

static MunitResult test_tokenize_psh(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "PSH \%a", 2, (uint8_t[]){PSH, REGISTER_A});

    assert_assem_equiv(c, "PSH \%pc", 2, (uint8_t[]){PSH, REGISTER_PC});

    assert_assem_equiv(c, "PSH 10", 3, (uint8_t[]){PSH, REGISTER_CV, 0xA});

    assert_assem_equiv(c, "PSH @ABCD", 6,
                       (uint8_t[]){PSH, REGISTER_MD, 0xA, 0xB, 0xC, 0xD});

    assert_assem_equiv(c, "PSH *FEDC", 6,
                       (uint8_t[]){PSH, REGISTER_MX, 0xF, 0xE, 0xD, 0xC});

    return MUNIT_OK;
}

static MunitResult test_tokenize_pop(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "POP \%a", 2, (uint8_t[]){POP, REGISTER_A});

    assert_assem_equiv(c, "POP \%pc", 2, (uint8_t[]){POP, REGISTER_PC});

    assert_assem_equiv(c, "POP @ABCD", 6,
                       (uint8_t[]){POP, REGISTER_MD, 0xA, 0xB, 0xC, 0xD});

    assert_assem_equiv(c, "POP *FEDC", 6,
                       (uint8_t[]){POP, REGISTER_MX, 0xF, 0xE, 0xD, 0xC});

    return MUNIT_OK;
}

static MunitResult test_tokenize_jmp(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "JMP N=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x0, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP N=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0x8, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP Z=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x1, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP Z=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0x9, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP C=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x2, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP C=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0xA, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP O=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x3, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP O=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0xB, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP I=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x4, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP I=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0xC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP H=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x5, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP H=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0xD, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP 0=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x6, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP 0=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0xE, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP 1=0 @FEDC", 6,
                       (uint8_t[]){JMP, 0x7, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP 1=1 @FEDC", 6,
                       (uint8_t[]){JMP, 0xF, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JMP 1=1 *1234", 6,
                       (uint8_t[]){JMP, 0xF, 0x1, 0x2, 0x3, 0x4});

    char *prog = strdup("LOOP:");
    bool success = tokenize(c, prog, 1);
    munit_assert_true(success);
    free(prog);

    assert_assem_equiv(c, "JMP 1=1 .LOOP", 6,
                       (uint8_t[]){JMP, 0xF, 0x0, 0x0, 0x0, 0x0});

    return MUNIT_OK;
}

static MunitResult test_tokenize_jsr(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "JSR N=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x0, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR N=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0x8, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR Z=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x1, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR Z=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0x9, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR C=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x2, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR C=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0xA, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR O=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x3, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR O=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0xB, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR I=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x4, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR I=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0xC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR H=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x5, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR H=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0xD, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR 0=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x6, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR 0=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0xE, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR 1=0 @FEDC", 6,
                       (uint8_t[]){JSR, 0x7, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR 1=1 @FEDC", 6,
                       (uint8_t[]){JSR, 0xF, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(c, "JSR 1=1 *1234", 6,
                       (uint8_t[]){JSR, 0xF, 0x1, 0x2, 0x3, 0x4});

    char *prog = strdup("LOOP:");
    bool success = tokenize(c, prog, 1);
    munit_assert_true(success);
    free(prog);

    assert_assem_equiv(c, "JSR 1=1 .LOOP", 6,
                       (uint8_t[]){JSR, 0xF, 0x0, 0x0, 0x0, 0x0});

    return MUNIT_OK;
}

static MunitResult test_tokenize_mov(const MunitParameter params[],
                                     void *fixture) {
    context *c = ((assem_fixture *)fixture)->ctx;

    assert_assem_equiv(c, "MOV \%b \%a", 3,
                       (uint8_t[]){MOV, REGISTER_B, REGISTER_A});

    assert_assem_equiv(c, "MOV 15 \%a", 4,
                       (uint8_t[]){MOV, REGISTER_CV, REGISTER_A, 15});

    assert_assem_equiv(c, "MOV \%pc \%sp", 3,
                       (uint8_t[]){MOV, REGISTER_PC, REGISTER_SP});

    assert_assem_equiv(c, "MOV \%pc \%iv", 3,
                       (uint8_t[]){MOV, REGISTER_PC, REGISTER_IV});

    assert_assem_equiv(c, "MOV \%pc \%ix", 3,
                       (uint8_t[]){MOV, REGISTER_PC, REGISTER_IX});

    assert_assem_equiv(c, "MOV \%pc \%ta", 3,
                       (uint8_t[]){MOV, REGISTER_PC, REGISTER_TA});

    assert_assem_equiv(
        c, "MOV 0xFEDC \%pc", 7,
        (uint8_t[]){MOV, REGISTER_CV, REGISTER_PC, 0xF, 0xE, 0xD, 0xC});

    assert_assem_equiv(
        c, "MOV 10 @FACE", 8,
        (uint8_t[]){MOV, REGISTER_CV, REGISTER_MD, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(
        c, "MOV 10 *FACE", 8,
        (uint8_t[]){MOV, REGISTER_CV, REGISTER_MX, 0xA, 0xF, 0xA, 0xC, 0xE});

    assert_assem_equiv(c, "MOV @ABCD @FE35", 11,
                       (uint8_t[]){MOV, REGISTER_MD, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "MOV @ABCD *FE35", 11,
                       (uint8_t[]){MOV, REGISTER_MD, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "MOV *ABCD @FE35", 11,
                       (uint8_t[]){MOV, REGISTER_MX, REGISTER_MD, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    assert_assem_equiv(c, "MOV *ABCD *FE35", 11,
                       (uint8_t[]){MOV, REGISTER_MX, REGISTER_MX, 0xA, 0xB, 0xC,
                                   0xD, 0xF, 0xE, 0x3, 0x5});

    return MUNIT_OK;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static MunitTest assem_assem_tests[] = {
    {(char *)"trivial case assembles correctly", test_assem_empty_string, NULL,
     NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"empty string tokenizes correctly", test_tokenize_empty_string,
     test_assem_setup, test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"org directives tokenize correctly", test_tokenize_org,
     test_assem_setup, test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"data directives tokenize correctly", test_tokenize_data,
     test_assem_setup, test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"comments tokenize correctly", test_tokenize_comment,
     test_assem_setup, test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"labels tokenize correctly", test_tokenize_label, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"NOP tokenizes correctly", test_tokenize_nop, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"INC tokenizes correctly", test_tokenize_inc, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"DEC tokenizes correctly", test_tokenize_dec, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"ADD tokenizes correctly", test_tokenize_add, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"SUB tokenizes correctly", test_tokenize_sub, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"RLC tokenizes correctly", test_tokenize_rlc, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"RRC tokenizes correctly", test_tokenize_rrc, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"AND tokenizes correctly", test_tokenize_and, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)" OR tokenizes correctly", test_tokenize_or, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"XOR tokenizes correctly", test_tokenize_xor, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"CMP tokenizes correctly", test_tokenize_cmp, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"PSH tokenizes correctly", test_tokenize_psh, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"POP tokenizes correctly", test_tokenize_pop, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"JMP tokenizes correctly", test_tokenize_jmp, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"JSR tokenizes correctly", test_tokenize_jsr, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"MOV tokenizes correctly", test_tokenize_mov, test_assem_setup,
     test_assem_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
#pragma GCC diagnostic pop
