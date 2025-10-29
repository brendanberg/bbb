#include "assem.h"
#include "../assem/table.h"
#include "../machine/cpu.h"
#include "../machine/memory.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define OPCODE_COUNT 16
#define REGISTER_COUNT 13
#define DIRECTIVE_COUNT 3

typedef enum ParseState {
    PARSE_OPER,
    PARSE_TEST,
    PARSE_ADDR,
    PARSE_DEST,
    PARSE_OPER_SRC,
    PARSE_PUSH_SRC,
    PARSE_ORG_ADDR,
    PARSE_DATA,
    PARSE_DONE,
    PARSE_ERROR = 0xFF
} ParseState;

typedef enum { META_ORG, META_DATA, META_INCLUDE, META_ERROR } MetaDirective;

#define OFFSET_SRC 12
#define OFFSET_DEST 8

#define MASK_SRC_REGISTER 0xF000
#define MASK_DST_REGISTER 0x0F00
#define MASK_PARSE_STATE 0x00FF

#define DATA_OFFSET(ctx) ((ctx)->data - (ctx)->data_start)

ParseState next_state[OPCODE_COUNT] = {
    PARSE_DONE,     PARSE_DEST,     PARSE_DEST,     PARSE_OPER_SRC,
    PARSE_OPER_SRC, PARSE_DEST,     PARSE_DEST,     PARSE_OPER_SRC,
    PARSE_OPER_SRC, PARSE_OPER_SRC, PARSE_OPER_SRC, PARSE_PUSH_SRC,
    PARSE_DEST,     PARSE_TEST,     PARSE_TEST,     PARSE_OPER_SRC};

typedef struct context {
    uint8_t *data_start;
    uint8_t *data;
    table *symbols;
    uint32_t line;
} context;

char *opcodes[OPCODE_COUNT] = {"NOP", "INC", "DEC", "ADD", "SUB", "RLC",
                               "RRC", "AND", "OR",  "XOR", "CMP", "PSH",
                               "POP", "JMP", "JSR", "MOV"};

char *registers[REGISTER_COUNT] = {"a",  "b",  "c",  "d",  "e",  "f", "s0",
                                   "s1", "pc", "sp", "iv", "ix", "ta"};
// The CV, MD, and MX virtual registers are missing from this table
// because they cannot be used in register specifiers.

char *directives[DIRECTIVE_COUNT] = {"org", "data", "inc"};

#define PUSH_NEXT(m, v) (*(m)++ = (v))

#define PUSH_QUARTET(m, v)                                                     \
    do {                                                                       \
        *(m)++ = ((v) & 0xF000) >> 12;                                         \
        *(m)++ = ((v) & 0xF00) >> 8;                                           \
        *(m)++ = ((v) & 0xF0) >> 4;                                            \
        *(m)++ = ((v) & 0xF);                                                  \
    } while (0)

#define UPDATE_STATE(st, v) ((st) & ~MASK_PARSE_STATE | (v))

static inline ParseState parse_directive(context *, char *);
static inline ParseState parse_opcode(context *, char *);
static inline ParseState parse_condition(context *, char *);
static inline ParseState parse_dest(context *, char *, uint16_t *, char **);
static inline ParseState parse_src(context *, char *, uint16_t *, char **);
static inline bool parse_register(context *, char *, uint8_t *);
static inline bool parse_addr(context *, char *, uint16_t *, char **);
static inline bool parse_hex(char *, uint8_t, uint16_t *);
static inline bool parse_int(char *, uint16_t *);
static inline bool parse_label(char *);

bool tokenize(context *, char *, uint16_t);

memory *build_image(char *filename, char *prog) {
    memory *mem = memory_init(CPU_MAX_ADDRESS);
    table *symbols = table_init();

    context ctx = {.data_start = mem->data,
                   .data = mem->data,
                   .symbols = symbols,
                   .line = 0};

    for (char *l = strsep(&prog, "\n"); l != NULL; l = strsep(&prog, "\n")) {
        char *line = strdup(l);
        if (!tokenize(&ctx, line, ++ctx.line)) {
            fprintf(stderr, "%s:%d: %s\n", filename, ctx.line, l);
            break;
        }
        free(line);
    }

    // table_print(symbols);

    for (reference *r = table_ref_pop(symbols); r != NULL;
         r = table_ref_pop(symbols)) {
        symbol *s = table_symbol_lookup(symbols, r->label);

        if (s) {
            uint16_t addr = s->address;
            *(r->offset + 0) = (addr >> 12) & 0xF;
            *(r->offset + 1) = (addr >> 8) & 0xF;
            *(r->offset + 2) = (addr >> 4) & 0xF;
            *(r->offset + 3) = (addr >> 0) & 0xF;
        } else {
            fprintf(stderr, "error: reference to undefined symbol '%s'\n",
                    r->label);
            return NULL;
        }
    }

    return mem;
}

bool tokenize(context *ctx, char *line, uint16_t num) {
    char *delim = "\t ";
    char *close = ")";
    char *sep = delim;

    char *saveptr;
    ParseState st = PARSE_OPER;
    uint16_t src_ext = 0;
    uint16_t dst_ext = 0;
    char *src_label;
    char *dst_label;

    for (char *t = strtok_r(line, sep, &saveptr); t != NULL;
         t = strtok_r(NULL, sep, &saveptr)) {
        size_t length = strlen(t);

        src_label = NULL;
        dst_label = NULL;

        if (t[0] == '(') {
            // If we encounter an opening parenthesis, the tokenizer must ignore
            // all characters until the closing parenthesis. If the final
            // character of the current token is not the closing parenthesis,
            // switch the separator in the strtok call to ")". Then the next
            // token will be the remainder of the comment.
            if (t[length - 1] != ')') {
                sep = close;
            }

            continue;
        } else if (strcmp(sep, close) == 0) {
            // If we enter the loop when the separator is set to ")", it
            // indicates we've parsed to the end of the comment and should swap
            // the separator back.
            sep = delim;
            continue;
        }

        if (t[0] == '#') {
            // Assembler directives start with '#' and should be the first token
            // on a line.
            if ((st & MASK_PARSE_STATE) != PARSE_OPER) {
                fprintf(stderr,
                        "error: didn't expect an assembler directive here\n");
                return false;
            }

            st = UPDATE_STATE(st, parse_directive(ctx, &t[1]));
            continue;
        }

        if (t[length - 1] == ':') {
            // Label definitions start with a letter or underscore followed by
            // letters, numbers, or underscores and end with ':'. If we
            // encounter one, we need to define it in the symbol table.
            t[length - 1] = '\0';
            table_symbol_define(ctx->symbols, t, DATA_OFFSET(ctx));
            continue;
        }

        switch (st & MASK_PARSE_STATE) {
        case PARSE_OPER: {
            st = UPDATE_STATE(st, parse_opcode(ctx, t));
            break;
        }
        case PARSE_TEST: {
            st = UPDATE_STATE(st, parse_condition(ctx, t));
            break;
        }
        case PARSE_ADDR: {
            if (parse_addr(ctx, t, &dst_ext, &dst_label)) {
                st =
                    UPDATE_STATE(st, PARSE_DONE | (REGISTER_MD << OFFSET_DEST));
            } else {
                fprintf(stderr, "error: expected address, found '%s'\n", t);
            }
            break;
        }
        case PARSE_DEST: {
            st = UPDATE_STATE(st, parse_dest(ctx, t, &dst_ext, &dst_label));
            break;
        }
        case PARSE_OPER_SRC: {
            st = UPDATE_STATE(st, parse_src(ctx, t, &src_ext, &src_label));
            break;
        }
        case PARSE_PUSH_SRC: {
            st = UPDATE_STATE(st, parse_src(ctx, t, &src_ext, &src_label));
            if ((st & MASK_PARSE_STATE) != PARSE_ERROR) {
                st = UPDATE_STATE(st, PARSE_DONE);
            }
            break;
        }
        case PARSE_ORG_ADDR: {
            uint16_t offset = 0;

            if (parse_hex(t, 4, &offset)) {
                ctx->data = ctx->data_start + offset;
                st = UPDATE_STATE(st, PARSE_DONE);
            } else {
                fprintf(stderr, "error: expected address, found '%s'\n", t);
            }

            break;
        }
        case PARSE_DATA: {
            uint16_t data = 0;

            if (parse_hex(t, 1, &data)) {
                PUSH_NEXT(ctx->data, data & 0xF);
            } else if (parse_hex(t, 2, &data)) {
                PUSH_NEXT(ctx->data, data & 0xF);
                PUSH_NEXT(ctx->data, data >> 2 & 0xF);
            } else if (parse_hex(t, 4, &data)) {
                PUSH_QUARTET(ctx->data, data);
            } else {
                fprintf(stderr, "error: expected address, found '%s'\n", t);
            }

            break;
        }
        default:
            break;
        }

        if ((st & MASK_PARSE_STATE) == PARSE_ERROR) {
            // Stop parsing the line if we encountered an error
            break;
        }

        if ((st & MASK_PARSE_STATE) == PARSE_DONE) {
            int virt_register_mask =
                (1 << REGISTER_CV) | (1 << REGISTER_MD) | (1 << REGISTER_MX);
            uint8_t source = (st & MASK_SRC_REGISTER) >> OFFSET_SRC;
            uint8_t dest = (st & MASK_DST_REGISTER) >> OFFSET_DEST;

            if (((1 << source) & virt_register_mask) == (1 << source)) {
                if (src_label) {
                    table_ref_add(ctx->symbols, src_label, ctx->data);
                }

                if (source == REGISTER_CV &&
                    (dest < REGISTER_PC || dest > REGISTER_CV)) {
                    // We only want to push one word of data when a constant
                    // value's destination is a 4-bit general purpose register,
                    // a memory direct address, or a memory index address.
                    PUSH_NEXT(ctx->data, src_ext & 0xF);
                } else {
                    // In all other cases, we push four words of data.
                    PUSH_QUARTET(ctx->data, src_ext);
                }
            }

            if (((1 << dest) & virt_register_mask) == (1 << dest)) {
                if (dst_label) {
                    table_ref_add(ctx->symbols, dst_label, ctx->data);
                }

                PUSH_QUARTET(ctx->data, dst_ext);
            }
        }
    }

    int state = st & MASK_PARSE_STATE;

    if (state == PARSE_OPER || state == PARSE_DONE || state == PARSE_DATA) {
        return true;
    }

    return false;
}

static inline ParseState parse_directive(context *ctx, char *token) {
    MetaDirective dir = META_ERROR;

    for (size_t i = 0; i < DIRECTIVE_COUNT; i++) {
        if (strcmp(token, directives[i]) == 0) {
            dir = i;
            break;
        }
    }

    switch (dir) {
    case META_ORG:
        return PARSE_ORG_ADDR;
    case META_DATA:
        return PARSE_DATA;
    // TODO: add case META_INC:
    case META_ERROR:
    default:
        fprintf(stderr, "error: unrecognized assembler directive '%s'\n",
                token);
        return PARSE_ERROR;
    }
}

static inline ParseState parse_opcode(context *ctx, char *token) {
    for (uint8_t opcode = 0; opcode <= 0xF; opcode++) {
        if (strcmp(token, opcodes[opcode]) == 0) {
            PUSH_NEXT(ctx->data, opcode);
            return next_state[opcode];
        }
    }

    fprintf(stderr, "error: encountered unrecognized opcode '%s'\n", token);
    return PARSE_ERROR;
}

static inline ParseState parse_condition(context *ctx, char *token) {
    if (token[1] != '=' || (token[2] != '0' && token[2] != '1')) {
        fprintf(stderr, "error: encountered unrecognized condition '%s'\n",
                token);
        return PARSE_ERROR;
    }

    uint8_t test = (token[2] == '1') << 3;

    char *bitfield = "NZCOIH01";

    for (uint8_t t = 0; t < 8; t++) {
        if (token[0] == bitfield[t]) {
            PUSH_NEXT(ctx->data, test | t);
            return PARSE_ADDR;
        }
    }

    fprintf(stderr, "error: encountered unrecognized condition '%s'\n", token);
    return PARSE_ERROR;
}

static inline ParseState parse_src(context *ctx, char *token, uint16_t *ext,
                                   char **label) {
    uint8_t reg;

    if (token[0] == '%') {
        if (parse_register(ctx, &token[1], &reg)) {
            return PARSE_DEST | (reg << OFFSET_SRC);
        } else {
            fprintf(stderr, "error: unrecognized register '%s'\n", token);
            return PARSE_ERROR;
        }
    } else if (token[0] == '0' && token[1] == 'x') {
        if (parse_hex(&token[2], 4, ext)) {
            PUSH_NEXT(ctx->data, REGISTER_CV);
            return PARSE_DEST | (REGISTER_CV << OFFSET_SRC);
        } else if (parse_hex(&token[2], 1, ext)) {
            PUSH_NEXT(ctx->data, REGISTER_CV);
            return PARSE_DEST | (REGISTER_CV << OFFSET_SRC);
        } else {
            fprintf(stderr, "error: unable to parse hex literal '%s'\n", token);
            return PARSE_ERROR;
        }
    }

    if (parse_addr(ctx, token, ext, label)) {
        switch (token[0]) {
        case '@': {
            PUSH_NEXT(ctx->data, REGISTER_MD);
            break;
        }
        case '*': {
            PUSH_NEXT(ctx->data, REGISTER_MX);
            break;
        }
        }
        return PARSE_DEST | (REGISTER_MD << OFFSET_SRC);
    } else if (parse_int(token, ext)) {
        PUSH_NEXT(ctx->data, REGISTER_CV);
        return PARSE_DEST | (REGISTER_CV << OFFSET_SRC);
    }

    fprintf(stderr, "error: unable to parse source operand '%s'\n", token);
    return PARSE_ERROR;
}

static inline ParseState parse_dest(context *ctx, char *token, uint16_t *ext,
                                    char **label) {
    uint8_t reg;

    if (token[0] == '%') {
        if (parse_register(ctx, &token[1], &reg)) {
            return PARSE_DONE | (reg << OFFSET_DEST);
        } else {
            fprintf(stderr, "error: unrecognized register '%s'\n", token);
            return PARSE_ERROR;
        }
    }

    if (parse_addr(ctx, token, ext, label)) {
        switch (token[0]) {
        case '@': {
            PUSH_NEXT(ctx->data, REGISTER_MD);
            break;
        }
        case '*': {
            PUSH_NEXT(ctx->data, REGISTER_MX);
            break;
        }
        }
        return PARSE_DONE | (REGISTER_MD << OFFSET_DEST);
    } else {
        fprintf(stderr, "error: unable to parse destination operand '%s'\n",
                token);
        return PARSE_ERROR;
    }
}

static inline bool parse_register(context *ctx, char *token, uint8_t *reg) {
    for (uint8_t r = 0; r < REGISTER_COUNT; r++) {
        if (strcmp(token, registers[r]) == 0) {
            PUSH_NEXT(ctx->data, r);
            *reg = r;
            return true;
        }
    }

    return false;
}

static inline bool parse_addr(context *ctx, char *token, uint16_t *addr,
                              char **label) {
    switch (token[0]) {
    case '@': {
        // Memory direct address
        if (parse_hex(&token[1], 4, addr)) {
            return true;
        } else {
            fprintf(stderr, "error: expected hex address, found '%s'\n", token);
            return false;
        }
    }
    case '*': {
        // Memory indexed
        if (parse_hex(&token[1], 4, addr)) {
            return true;
        } else {
            fprintf(stderr, "error: expected hex address, found '%s'\n", token);
            return false;
        }
    }
    case '.': {
        // Label reference
        if (parse_label(&token[1])) {
            *label = &token[1];
            return true;
        } else {
            fprintf(stderr, "error: expected label, found '%s'\n", token);
            return false;
        }
    }
    default:
        return false;
    }
}

static inline bool parse_hex(char *token, uint8_t length, uint16_t *result) {
    *result = 0;

    for (char *ch = token; *ch != 0; ch++) {
        *result <<= 4;
        length--;
        if (*ch >= '0' && *ch <= '9') {
            *result += (*ch - '0');
        } else if (*ch >= 'A' && *ch <= 'F') {
            *result += (*ch - 'A') + 0xA;
        } else {
            return false;
        }
    }

    return length == 0;
}

static inline bool parse_int(char *token, uint16_t *result) {
    *result = 0;

    for (char *ch = token; *ch != 0; ch++) {
        *result *= 10;
        if (*ch >= '0' && *ch <= '9') {
            *result += (*ch - '0');
        } else {
            return false;
        }
    }

    return true;
}

static inline bool parse_label(char *token) {
    // Parsing a label involves validating the characters and then scanning the
    // label list for the current label. If the label is found, we set the
    // result value to the pointer into the label list. If the label is not
    // found, we add the label and return the pointer to the newly added label.

    for (char *ch = token; *ch != 0; ch++) {
        if (!((*ch >= 'A' && *ch <= 'Z') || (*ch >= 'a' && *ch <= 'z') ||
              (*ch == '_'))) {
            return false;
        }
    }

    return true;
}
