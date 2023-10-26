#include "assemble.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"

typedef enum {
    DONE,
    EXPECT_OP,
    EXPECT_TEST,
    EXPECT_ADDR,
    EXPECT_DST,
    EXPECT_OP_SRC,
    EXPECT_PSH_SRC,
    ERROR
} ParseState;

static inline ParseState parse_opcode(char *, uint16_t[]);
static inline ParseState parse_test(char *token, uint16_t instr[]);
static inline ParseState parse_addr(char *token, uint16_t instr[]);
static inline ParseState parse_dest(char *token, uint16_t instr[]);
static inline ParseState parse_src(char *token, uint16_t instr[]);
static inline bool parse_hex(char *token, uint16_t *result);
static inline bool parse_label(char *token);

#define PUSH_NEXT(m, v) (*(m)++ = (v))

#define PUSH_QUARTET(m, v)           \
    do {                             \
        *(m)++ = ((v)&0xF000) >> 12; \
        *(m)++ = ((v)&0xF00) >> 8;   \
        *(m)++ = ((v)&0xF0) >> 4;    \
        *(m)++ = ((v)&0xF);          \
    } while (0)

static inline ParseState parse_opcode(char *token, uint16_t instr[]) {
    if (strcmp(token, "NOP") == 0) {
        instr[0] |= NOP << 12;
        return DONE;
    } else if (strcmp(token, "INC") == 0) {
        instr[0] |= INC << 12;
        return EXPECT_DST;
    } else if (strcmp(token, "DEC") == 0) {
        instr[0] |= DEC << 12;
        return EXPECT_DST;
    } else if (strcmp(token, "ADD") == 0) {
        instr[0] |= ADD << 12;
        return EXPECT_OP_SRC;
    } else if (strcmp(token, "SUB") == 0) {
        instr[0] |= SUB << 12;
        return EXPECT_OP_SRC;
    } else if (strcmp(token, "RLC") == 0) {
        instr[0] |= RLC << 12;
        return EXPECT_DST;
    } else if (strcmp(token, "RRC") == 0) {
        instr[0] |= RRC << 12;
        return EXPECT_DST;
    } else if (strcmp(token, "AND") == 0) {
        instr[0] |= AND << 12;
        return EXPECT_OP_SRC;
    } else if (strcmp(token, "OR") == 0) {
        instr[0] |= OR << 12;
        return EXPECT_OP_SRC;
    } else if (strcmp(token, "XOR") == 0) {
        instr[0] |= XOR << 12;
        return EXPECT_OP_SRC;
    } else if (strcmp(token, "CMP") == 0) {
        instr[0] |= CMP << 12;
        return EXPECT_OP_SRC;
    } else if (strcmp(token, "PSH") == 0) {
        instr[0] |= PSH << 12;
        return EXPECT_PSH_SRC;
    } else if (strcmp(token, "POP") == 0) {
        instr[0] |= POP << 12;
        return EXPECT_DST;
    } else if (strcmp(token, "JMP") == 0) {
        instr[0] |= JMP << 12;
        return EXPECT_TEST;
    } else if (strcmp(token, "JSR") == 0) {
        instr[0] |= JSR << 12;
        return EXPECT_TEST;
    } else if (strcmp(token, "MOV") == 0) {
        instr[0] |= MOV << 12;
        return EXPECT_OP_SRC;
    } else {
        // We encountered an unrecognized opcode
        return ERROR;
    }
}

static inline ParseState parse_test(char *token, uint16_t instr[]) {
    if (token[1] != '=' || (token[2] != '0' && token[2] != '1')) {
        // We encountered an unrecognized test
        return ERROR;
    }

    switch (token[0]) {
    case 'N':
        instr[0] |= 0x0;
        break;
    case 'Z':
        instr[0] |= 0x1;
        break;
    case 'C':
        instr[0] |= 0x2;
        break;
    case 'O':
        instr[0] |= 0x3;
        break;
    case 'I':
        instr[0] |= 0x4;
        break;
    case 'H':
        instr[0] |= 0x5;
        break;
    case '0':
        instr[0] |= 0x6;
        break;
    case '1':
        instr[0] |= 0x7;
        break;
    default:
        // We encountered an unrecognized test
        return ERROR;
    }

    if (token[2] == '1') {
        instr[0] |= 0x8;
    }
    return EXPECT_ADDR;
}

static inline ParseState parse_addr(char *token, uint16_t instr[]) {
    uint16_t result = 0;
    size_t len = strlen(token);

    switch (token[0]) {
    case '@': {
        // Memory direct address
        if (len == 5) {
            if (parse_hex(&token[1], &result)) {
                instr[1] |= result;
            } else {
                return ERROR;
            }
        } else {
            return ERROR;
        }
        break;
    }
    case '*': {
        // Memory indexed
        if (len == 5) {
            if (parse_hex(&token[1], &result)) {
                instr[1] |= result;
            } else {
                return ERROR;
            }
        } else {
            return ERROR;
        }
        break;
    }
    // case '.': {
    //     // Label reference
    //     char *label;
    //     if (parse_label(&token[1])) {
    //         // TODO: look up label in lookup table
    //     } else {
    //         return ERROR;
    //     }
    //     break;
    // }
    default:
        return ERROR;
    }

    return DONE;
}

static inline ParseState parse_dest(char *token, uint16_t instr[]) {
    uint16_t result = 0;
    size_t len = strlen(token);

    switch (token[0]) {
    case '@': {
        // Memory direct address
        if (len == 5) {
            instr[0] |= REGISTER_MD;

            if (parse_hex(&token[1], &result)) {
                instr[2] |= result;
            } else {
                return ERROR;
            }
        } else {
            return ERROR;
        }
        break;
    }
    case '*': {
        // Memory indexed
        if (len == 5) {
            instr[0] |= REGISTER_MX;

            if (parse_hex(&token[1], &result)) {
                instr[2] |= result;
            } else {
                return ERROR;
            }
        } else {
            return ERROR;
        }
        break;
    }
    case '.': {
        // Label reference
        char *label;
        if (parse_label(&token[1])) {
            // TODO: look up label in lookup table
        } else {
            return ERROR;
        }
        break;
    }
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F': {
        if (strlen(token) != 1) {
            return ERROR;
        }
        instr[0] |= (token[0] - 'A');
        break;
    }
    case 'S': {
        if (strlen(token) != 2) {
            return ERROR;
        }

        if (token[1] == '0') {
            instr[0] |= REGISTER_S0;
        } else if (token[1] == '1') {
            instr[0] |= REGISTER_S1;
        } else if (token[1] == 'P') {
            instr[0] |= REGISTER_SP;
        } else {
            return ERROR;
        }
        break;
    }
    case 'P': {
        if (strcmp(token, "PC") == 0) {
            instr[0] |= REGISTER_PC;
        } else {
            return ERROR;
        }
        break;
    }
    case 'I': {
        if (strlen(token) != 2) {
            return ERROR;
        }

        if (token[1] == 'V') {
            instr[0] |= REGISTER_IV;
        } else if (token[1] == 'X') {
            instr[0] |= REGISTER_IX;
        } else {
            return ERROR;
        }
        break;
    }
    case 'T': {
        if (strcmp(token, "TA") == 0) {
            instr[0] |= REGISTER_TA;
        } else {
            return ERROR;
        }
        break;
    }
    default:
        return ERROR;
    }

    return DONE;
}

static inline bool parse_hex(char *token, uint16_t *result) {
    *result = 0;

    for (char *ch = token; *ch != 0; ch++) {
        *result <<= 4;

        if (*ch >= '0' && *ch <= '9') {
            *result += (*ch - '0');
        } else if (*ch >= 'A' && *ch <= 'F') {
            *result += (*ch - 'A') + 0xA;
        } else {
            return false;
        }
    }

    return true;
}

static inline bool parse_label(char *token) {
    for (char *ch = token; *ch != 0; ch++) {
        if (!((*ch >= 'A' && *ch <= 'Z') || (*ch >= 'a' && *ch <= 'z') ||
              (*ch >= '0' && *ch <= '9') || (*ch == '_'))) {
            return false;
        }
    }

    return true;
}

static inline ParseState parse_src(char *token, uint16_t instr[]) {
    uint16_t result = 0;
    size_t len = strlen(token);

    switch (token[0]) {
    case '#': {
        // Constant value
        instr[0] |= REGISTER_CV << 8;

        if (len == 2) {
            if (parse_hex(&token[1], &result)) {
                instr[1] |= result & 0xF;
            } else {
                return ERROR;
            }
        } else if (len == 5) {
            if (parse_hex(&token[1], &result)) {
                instr[1] |= result;
            } else {
                return ERROR;
            }
        } else {
            return ERROR;
        }
        break;
    }
    case '@': {
        // Memory direct address
        if (len == 5) {
            instr[0] |= REGISTER_MD << 8;

            if (parse_hex(&token[1], &result)) {
                instr[1] |= result;
            } else {
                return ERROR;
            }
        } else {
            return ERROR;
        }
        break;
    }
    case '*': {
        // Memory indexed
        if (len == 5) {
            instr[0] |= REGISTER_MX << 8;

            if (parse_hex(&token[1], &result)) {
                instr[1] |= result;
            } else {
                return ERROR;
            }
        } else {
            return ERROR;
        }
        break;
    }
    case '.': {
        // Label reference
        char *label;
        if (parse_label(&token[1])) {
            // TODO: look up label in lookup table
        } else {
            return ERROR;
        }
        break;
    }
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F': {
        if (strlen(token) != 1) {
            return ERROR;
        }
        instr[0] |= (token[0] - 'A') << 8;
        break;
    }
    case 'S': {
        if (strlen(token) != 2) {
            return ERROR;
        }

        if (token[1] == '0') {
            instr[0] |= REGISTER_S0 << 8;
        } else if (token[1] == '1') {
            instr[0] |= REGISTER_S1 << 8;
        } else if (token[1] == 'P') {
            instr[0] |= REGISTER_SP << 8;
        } else {
            return ERROR;
        }
        break;
    }
    case 'P': {
        if (strcmp(token, "PC") == 0) {
            instr[0] |= REGISTER_PC << 8;
        } else {
            return ERROR;
        }
        break;
    }
    case 'I': {
        if (strlen(token) != 2) {
            return ERROR;
        }

        if (token[1] == 'V') {
            instr[0] |= REGISTER_IV << 8;
        } else if (token[1] == 'X') {
            instr[0] |= REGISTER_IX << 8;
        } else {
            return ERROR;
        }
        break;
    }
    case 'T': {
        if (strcmp(token, "TA") == 0) {
            instr[0] |= REGISTER_TA << 8;
        } else {
            return ERROR;
        }
        break;
    }
    default:
        return ERROR;
    }

    return EXPECT_DST;
}

memory *assemble(char *prog) {
    uint8_t *image = calloc(64 * 1024, sizeof(uint8_t));
    uint8_t *offset = image;

    char *delim = "\n\t ";
    char *close = ")";
    char *sep = delim;

    uint16_t instr[3] = {0, 0, 0};
    ParseState state = EXPECT_OP;

    for (char *p = strtok(prog, sep); p != NULL; p = strtok(NULL, sep)) {
        size_t length = strlen(p);

        if (p[0] == '(') {
            // If we encounter an opening parenthesis, the tokenizer must
            // ignore all characters until the closing parenthesis. If the
            // final character of the current token is not the closing
            // parenthesis, switch the separator in the strtok call to ")".
            // Then the next token will be the remainder of the comment.
            if (p[length - 1] != ')') {
                sep = close;
            }

            continue;
        } else if (strcmp(sep, close) == 0) {
            sep = delim;
            continue;
        }

        if (p[length - 1] == ':') {
            // We encountered a label definition.
            // TODO: Get the current offset in the image and add the label name
            // to the lookup table
            continue;
        }

        switch (state) {
        case EXPECT_OP: {
            state = parse_opcode(p, instr);
            break;
        }
        case EXPECT_TEST: {
            state = parse_test(p, instr);
            break;
        }
        case EXPECT_ADDR: {
            state = parse_addr(p, instr);
            break;
        }
        case EXPECT_DST: {
            state = parse_dest(p, instr);
            break;
        }
        case EXPECT_OP_SRC: {
            state = parse_src(p, instr);
            break;
        }
        case EXPECT_PSH_SRC: {
            if (parse_src(p, instr) != ERROR) {
                // The parse_src function sets the parse state to EXPECT_DST,
                // which is the wrong state after parsing the source of a PSH
                // instruction. We set the state to DONE if we did not encounter
                // an error.
                state = DONE;
            } else {
                state = ERROR;
            }
            break;
        }
        case DONE:
        case ERROR: {
            break;
        }
        }

        if (state == DONE) {
            // TODO: Push instr into image buffer
            // printf("%04X\n", instr[0]);
            // printf("%04X\n", instr[1]);
            // printf("%04X\n", instr[2]);
            // printf("----\n");
            uint8_t opcode = instr[0] >> 12;
            uint8_t src = (instr[0] & 0xF00) >> 8;
            uint8_t dst = instr[0] & 0xF;

            switch (opcode) {
            case NOP: {
                PUSH_NEXT(offset, NOP);
                break;
            }
            case JMP:
            case JSR: {
                PUSH_NEXT(offset, opcode);
                PUSH_NEXT(offset, dst);
                PUSH_QUARTET(offset, instr[1]);
                break;
            }
            case PSH: {
                if (src >= REGISTER_A && src <= REGISTER_TA) {
                    PUSH_NEXT(offset, opcode);
                    PUSH_NEXT(offset, src);
                } else if (src == REGISTER_CV) {
                    PUSH_NEXT(offset, opcode);
                    PUSH_NEXT(offset, REGISTER_CV);
                    PUSH_NEXT(offset, instr[1] & 0xF);
                } else {
                    PUSH_NEXT(offset, opcode);
                    PUSH_NEXT(offset, src);
                    PUSH_QUARTET(offset, instr[1]);
                }
                break;
            }
            case INC:
            case DEC:
            case RLC:
            case RRC:
            case POP: {
                if (dst >= REGISTER_A && dst <= REGISTER_TA) {
                    PUSH_NEXT(offset, opcode);
                    PUSH_NEXT(offset, dst);
                } else if (dst == REGISTER_CV) {
                    state = ERROR;
                    return NULL;
                } else {
                    PUSH_NEXT(offset, opcode);
                    PUSH_NEXT(offset, dst);
                    PUSH_QUARTET(offset, instr[2]);
                }
                break;
            }
            case ADD:
            case SUB:
            case AND:
            case OR:
            case XOR:
            case CMP:
            case MOV: {
                PUSH_NEXT(offset, opcode);
                PUSH_NEXT(offset, src);
                PUSH_NEXT(offset, dst);

                if (src == REGISTER_CV && dst <= REGISTER_S1) {
                    PUSH_NEXT(offset, instr[1] & 0xF);
                } else if (src == REGISTER_CV && dst >= REGISTER_PC) {
                    PUSH_QUARTET(offset, instr[1]);
                } else if (src >= REGISTER_MD) {
                    PUSH_QUARTET(offset, instr[1]);
                }

                if (dst >= REGISTER_MD) {
                    PUSH_QUARTET(offset, instr[2]);
                }

                break;
            }
            }

            // Reset the instruction assembly area
            instr[0] = 0;
            instr[1] = 0;
            instr[2] = 0;

            state = EXPECT_OP;
        } else if (state == ERROR) {
            // TODO: Print friendly error messages
            printf("ERROR encountered %s\n", p);
            break;
        }
    }

    memory *m = memory_init(0);
    return m;
}
