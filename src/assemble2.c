#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "assemble.h"
#include "cpu.h"
#include "memory.h"
#include "table.h"

typedef enum {
    DONE,
    DONE_SRC,
    DONE_DST,
    DONE_SRC_DST,
    EXPECT_OP,
    EXPECT_TEST,
    EXPECT_ADDR,
    EXPECT_DST,
    EXPECT_DST_EXT,
    EXPECT_OP_SRC,
    EXPECT_PSH_SRC,
    ERROR
} ParseState;

typedef struct asm_state {
    uint8_t *image;
    size_t offset;
    table *symbols;
} asm_state;

char **opcodes = {"NOP", "INC", "DEC", "ADD", "SUB", "RLC", "RRC", "AND",
                  "OR",  "XOR", "CMP", "PSH", "POP", "JMP", "JSR", "MOV"};

char **registers = {"a",  "b",  "c",  "d",  "e",  "f", "s0",
                    "s1", "pc", "sp", "iv", "ix", "ta"};  //, "cv", "md", "mx"};

#define PUSH_NEXT(m, v) (*(m)++ = (v))

#define PUSH_QUARTET(m, v)           \
    do {                             \
        *(m)++ = ((v)&0xF000) >> 12; \
        *(m)++ = ((v)&0xF00) >> 8;   \
        *(m)++ = ((v)&0xF0) >> 4;    \
        *(m)++ = ((v)&0xF);          \
    } while (0)

static inline ParseState parse_opcode(asm_state *, char *);
static inline ParseState parse_test(asm_state *, char *);
static inline ParseState parse_addr(asm_state *, char *, uint16_t *);
static inline ParseState parse_dest(asm_state *, char *, uint16_t *);
static inline ParseState parse_src(asm_state *, char *, uint16_t *);
static inline bool parse_hex(char *, uint8_t, uint16_t *);
static inline bool parse_label(char *token);

void tokenize(asm_state *state, char *line, uint16_t num);

memory *build_image(char *prog) {
    memory *mem = memory_init(CPU_MAX_ADDRESS);
    table *symbols = table_init(1024);
    uint16_t line = 0;
    char *saved;

    asm_state state = {mem->data, 0, symbols};

    for (char *l = strok_r(prog, "\n", &saved); l != NULL; l = strtok_r(NULL, "\n", &saved)) {
        line++;

        tokenize(&state, l, line);
    }
}

void tokenize(asm_state *state, char *line, uint16_t num) {
    char *delim = "\t ";
    char *close = ")";
    char *sep = delim;

    char *saved;
    ParseState st = DONE;
    uint16_t src_ext = 0;
    uint16_t dst_ext = 0;

    for (char *t = strtok_r(line, sep, &saved); t != NULL; t = strtok_r(NULL, sep, &saved)) {
        size_t length = strlen(t);

        if (t[0] == '(') {
            // If we encounter an opening parenthesis, the tokenizer must
            // ignore all characters until the closing parenthesis. If the
            // final character of the current token is not the closing
            // parenthesis, switch the separator in the strtok call to ")".
            // Then the next token will be the remainder of the comment.
            if (t[length - 1] != ')') {
                sep = close;
            }

            continue;
        } else if (strcmp(sep, close) == 0) {
            sep = delim;
            continue;
        }

        if (t[length - 1] == ':') {
            t[length - 1] = '\0';
            table_insert(state->symbols, t, state->offset);
            continue;
        }

        switch (st) {
        case EXPECT_OP: {
            st = parse_opcode(state, t);
            break;
        }
        case EXPECT_TEST: {
            st = parse_test(state, t);
            break;
        }
        case EXPECT_ADDR: {
            uint16_t addr = 0;
            st = parse_addr(state, t, &addr);  // TODO get state from return value
            PUSH_QUARTET(state->image, addr);
            break;
        }
        case EXPECT_DST: {
            st = parse_dest(state, t, &dst_ext);
            break;
        }
        case EXPECT_OP_SRC: {
            st = parse_src(state, t, &src_ext);
            break;
        }
        case EXPECT_PSH_SRC: {
            st = parse_src(state, t, &src_ext);
            if (st != ERROR) {
                st = DONE;
            }
            break;
        }
        case DONE_DST: {
            PUSH_QUARTET(state->image, dst_ext);
            st = DONE;
            break;
        }
        case DONE_SRC: {
            PUSH_QUARTET(state->image, src_ext);
            st = DONE;
            break;
        }
        case DONE_SRC_DST: {
            PUSH_QUARTET(state->image, src_ext);
            PUSH_QUARTET(state->image, dst_ext);
            st = DONE;
            break;
        }
        default:
            break;
        }

        if (st == ERROR) {
            break;  // Break out of the loop
        }
    }

    if (st != DONE) {
        fprintf(stderr, "error: unable to parse line");
    }
}

ParseState *next_state = {DONE,          EXPECT_DST,    EXPECT_DST,    EXPECT_OP_SRC,
                          EXPECT_OP_SRC, EXPECT_DST,    EXPECT_DST,    EXPECT_OP_SRC,
                          EXPECT_OP_SRC, EXPECT_OP_SRC, EXPECT_OP_SRC, EXPECT_PSH_SRC,
                          EXPECT_DST,    EXPECT_TEST,   EXPECT_TEST,   EXPECT_OP_SRC};

static inline ParseState parse_opcode(asm_state *state, char *token) {
    for (uint8_t opcode = 0; opcode <= 0xF; opcode++) {
        if (strcmp(token, opcodes[opcode]) == 0) {
            PUSH_NEXT(state->image, opcode);
            return next_state[opcode];
        }
    }

    fprintf(stderr, "error: encountered unrecognized opcode '%s'\n", token);
    return ERROR;
}

static inline ParseState parse_test(asm_state *state, char *token) {
    if (token[1] != '=' || (token[2] != '0' && token[2] != '1')) {
        fprintf(stderr, "error: encountered unrecognized condition '%s'\n", token);
        return ERROR;
    }

    uint8_t test = (token[2] == '1') << 3;

    char *bitfield = "NZCOIH01";

    for (uint8_t t = 0; t < 8; t++) {
        if (token[0] == bitfield[t]) {
            PUSH_NEXT(state->image, test | t);
            return EXPECT_ADDR;
        }
    }

    fprintf(stderr, "error: encountered unrecognized condition '%s'\n", token);
    return ERROR;
}

static inline ParseState parse_addr(asm_state *state, char *token, uint16_t *addr) {
    switch (token[0]) {
    case '@': {
        // Memory direct address
        if (parse_hex(&token[1], 4, addr)) {
            return DONE;
        } else {
            fprintf(stderr, "error: expected hex address, found '%s'\n", token);
            return ERROR;
        }
    }
    case '*': {
        // Memory indexed
        if (parse_hex(&token[1], 4, addr)) {
            return DONE;
        } else {
            fprintf(stderr, "error: expected hex address, found '%s'\n", token);
            return ERROR;
        }
    }
    case '.': {
        // TODO: UPDATE THE SYMBOL TABLE WITH THE ADDR REF
        uint16_t addr;
        bool found = table_scan(state->symbols, &token[1], &addr);

        if (!found) {
            table_insert(state->symbols, &token[1], 0);
            fprintf(stderr, "error: undefined label '%s'\n", &token[1]);
            return DONE;
        }

        return DONE;
    }
    default:
        return ERROR;
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

static inline ParseState parse_src(asm_state *state, char *token, uint16_t *ext) {
    if (token[0] = '%') {
        return parse_register(state, &token[1]);
    }

    *ext = parse_addr(state, token, ext);
    PUSH_NEXT(state->image, REGISTER_MD);
    return EXPECT_DST_EXT;
}

static inline ParseState parse_dest(asm_state *state, char *token, uint16_t *ext) {
    if (token[0] = '%') {
        return parse_register(state, &token[1]);
    }

    *ext = parse_addr(state, token, ext);
    PUSH_NEXT(state->image, REGISTER_MD);
    return EXPECT_DST_EXT;
}

static inline ParseState parse_register(asm_state *state, char *token) {
    for (uint8_t r = 0; r <= 0xF; r++) {
        if (strcmp(token, registers[r]) == 0) {
            PUSH_NEXT(state->image, r);
            return EXPECT_DST;
        }
    }

    fprintf(stderr, "error: unrecognized register '%s'\n", token);
    return ERROR;
}
