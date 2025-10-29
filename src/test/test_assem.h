#ifndef BBB_ASSEM_PVT_H
#define BBB_ASSEM_PVT_H

#include "../assem/table.h"
#include "../machine/memory.h"

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

typedef struct context {
    uint8_t *data_start;
    uint8_t *data;
    table *symbols;
    uint32_t line;
} context;

bool tokenize(context *ctx, char *line, uint16_t num);
static inline ParseState parse_directive(context *ctx, char *token);
static inline ParseState parse_opcode(context *ctx, char *token);
static inline ParseState parse_condition(context *ctx, char *token);
static inline ParseState parse_src(context *ctx, char *token, uint16_t *ext,
                                   char **label);
static inline ParseState parse_dest(context *ctx, char *token, uint16_t *ext,
                                    char **label);
static inline bool parse_register(context *ctx, char *token, uint8_t *reg);
static inline bool parse_addr(context *ctx, char *token, uint16_t *addr,
                              char **label);
static inline bool parse_hex(char *token, uint8_t length, uint16_t *result);
static inline bool parse_int(char *token, uint16_t *result);
static inline bool parse_label(char *token);

#endif
