#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "memory.h"


// General purpose registers are 4-bit. Special registers are 16-bit
static inline void machine_instr_fetch (machine *m);
static inline void machine_instr_decode (machine *m);
static inline void machine_instr_execute (machine *m);

static inline uint16_t machine_get_value(machine *m, Register src, uint16_t src_ext) {
    switch (src) {
    case REGISTER_CV:
        return src_ext;
    case REGISTER_MD:
        return m->memory->data[src_ext];
    case REGISTER_MX: {
        uint16_t offset = m->ix - m->memory->data;
        return m->memory->data[offset + src_ext];
    }
    case REGISTER_PC:
        return m->pc - m->memory->data;
    case REGISTER_SP:
        return m->sp - m->memory->data;
    case REGISTER_IV:
        return m->iv - m->memory->data;
    case REGISTER_IX:
        return m->ix - m->memory->data;
    case REGISTER_TA:
        return m->ta - m->memory->data;
    case REGISTER_S0:
        return m->flags & 0x0F;
    case REGISTER_S1:
        return m->flags >> 4;
    default:
        return m->registers[src];
    }
}

static inline uint8_t flags (uint16_t value) {
    return (value ? 0 : 1) << 1 | (value < 0 ? 1 : 0);
}

static inline void machine_set_value(machine *m, Register dst, uint16_t dst_ext, uint16_t value) {
    switch (dst) {
    case REGISTER_CV:
        m->flags |= 0x20;
        break;
    case REGISTER_MD:
        m->flags = flags(value & 0x0F) | (m->flags & 0xF0);
        m->memory->data[dst_ext] = value;
        break;
    case REGISTER_MX:
        m->flags = flags(value & 0x0F) | (m->flags & 0xF0);
        m->memory->data[(m->ix - m->memory->data) + dst_ext] = value;
        break;
    case REGISTER_PC:
        m->pc = (m->memory->data + value);
        break;
    case REGISTER_SP:
        m->sp = (m->memory->data + value);
        break;
    case REGISTER_IV:
        m->iv = (m->memory->data + value);
        break;
    case REGISTER_IX:
        m->ix = (m->memory->data + value);
        break;
    case REGISTER_TA:
        m->ta = (m->memory->data + value);
        break;
    case REGISTER_S0:
        m->flags = value & 0x0F;
        break;
    case REGISTER_S1:
        m->flags = (value & 0x0F) << 4;
        break;
    default:
        m->flags = flags(value & 0x0F) | (m->flags & 0xF0);
        m->registers[dst] = value;
        break;
    }


}

void machine_init (machine **m, size_t size) {
    *m = malloc(sizeof(machine));
    memory_init(&((*m)->memory), size);
    machine_reset(*m);
}

void machine_show (machine *m) {
    printf("+-------------+--------+------------------------------+\n");
    printf("| A B C D E F | HIOCZN | PROG  STAK  INTR  INDX  TEMP |\n");
    printf("| %X %X %X %X %X %X | ",
        m->registers[REGISTER_A],
        m->registers[REGISTER_B],
        m->registers[REGISTER_C],
        m->registers[REGISTER_D],
        m->registers[REGISTER_E],
        m->registers[REGISTER_F]
    );

    printf(m->flags & 0x20 ? "*" : ".");
    printf(m->flags & 0x10 ? "*" : ".");
    printf(m->flags & 0x08 ? "*" : ".");
    printf(m->flags & 0x04 ? "*" : ".");
    printf(m->flags & 0x02 ? "*" : ".");
    printf(m->flags & 0x01 ? "*" : ".");

    printf(" | %04X  %04X  %04X  %04X  %04X |\n",
        (uint8_t) (m->pc - m->memory->data),
        (uint8_t) (m->sp - m->memory->data),
        (uint8_t) (m->iv - m->memory->data),
        (uint8_t) (m->ix - m->memory->data),
        (uint8_t) (m->ta - m->memory->data)
    );
    printf("+-------------+--------+------------------------------+\n\n");
}

void machine_start (machine *m) {
    m->status = STATE_RUN;
}

void machine_pause (machine *m);
void machine_halt (machine *m);

void machine_reset (machine *m) {
    m->status = STATE_HALT;
    m->pc = m->sp = m->iv = m->ix = m->ta = m->memory->data;
    m->flags = 0x80;

    for (uint8_t i = 0; i < 8; i++) {
        m->registers[i] = 0;
    }
}

#define READ_NEXT(x) (*(x)++)
#define READ_QUARTET(x) (*(x)++|*(x)++<<4|*(x)++<<8|*(x)++<<12)

void machine_run (machine *m) {
    while (!(m->flags & 0x20)) {
        machine_instr_fetch(m);
        machine_instr_decode(m);
        machine_instr_execute(m);
        machine_show(m);
    }
}

static inline void machine_instr_fetch (machine *m) {
    m->instr = READ_NEXT(m->pc);
}

static inline void machine_instr_decode (machine *m) {
    switch (m->instr) {
        case ADD:
        case SUB:
        case AND:
        case OR:
        case XOR:
        case CMP:
        case MOV: {
            m->src = (Register) READ_NEXT(m->pc);
            m->dst = (Register) READ_NEXT(m->pc);

            switch (m->src) {
                case REGISTER_CV: {
                    if (m->dst < REGISTER_PC) {
                        m->src_ext = READ_NEXT(m->pc);
                    } else {
                        m->src_ext = READ_QUARTET(m->pc);
                    }
                    break;
                }
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->src_ext = READ_QUARTET(m->pc);
                }
                default: break;
            }
            
            switch (m->dst) {
                case REGISTER_CV: {
                    // Read-only registers are not valid destinations; HALT.
                    m->flags |= 0x20;
                    break;
                }
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->dst_ext = READ_QUARTET(m->pc);
                }
                default: break;
            }

            break;
        }

        case INC:
        case DEC:
        case ROLC:
        case RORC:
        case POP: {
            m->dst = (Register) READ_NEXT(m->pc);

            switch (m->dst) {
                case REGISTER_CV: {
                    // Read-only registers are not valid destinations; HALT.
                    m->flags |= 0x20;
                    break;
                }
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->dst_ext = READ_QUARTET(m->pc);
                }
                default: break;
            }

            break;
        }


        case JMP:
        case JSR: {
            // Note: the DST nybble in a JMP or JSR instruction
            // contains the type of jump to be executed
            m->dst = READ_NEXT(m->pc);
            m->dst_ext = READ_QUARTET(m->pc);
            break;
        }

        case PSH: {
            m->src = READ_NEXT(m->pc);

            switch (m->src) {
                case REGISTER_CV:
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->src_ext = READ_QUARTET(m->pc);
                }
                default: break;
            }

            break;
        }
    }
}

static inline void machine_instr_execute (machine *m) {
    switch (m->instr) {
        case NOP: {
            break;
        }
        case INC: {
            uint16_t value = machine_get_value(m, m->dst, m->dst_ext) + 1;
            machine_set_value(m, m->dst, m->dst_ext, value);
            break;
        }
        case DEC: {
            uint16_t value = machine_get_value(m, m->dst, m->dst_ext) - 1;
            machine_set_value(m, m->dst, m->dst_ext, value);
            break;
        }
        case ADD: {
            uint16_t value = (machine_get_value(m, m->src, m->src_ext) +
                machine_get_value(m, m->dst, m->dst_ext));
            machine_set_value(m, m->dst, m->dst_ext, value);
            break;
        }
        case SUB: {
            machine_set_value(m, m->dst, m->dst_ext, (
                machine_get_value(m, m->dst, m->dst_ext) -
                machine_get_value(m, m->src, m->src_ext)
            ));
            break;
        }
        case ROLC:
        case RORC:
        case AND: {
            machine_set_value(m, m->dst, m->dst_ext, (
                machine_get_value(m, m->src, m->src_ext) &
                machine_get_value(m, m->dst, m->dst_ext)
            ));
            break;
        }
        case OR: {
            machine_set_value(m, m->dst, m->dst_ext, (
                machine_get_value(m, m->src, m->src_ext) |
                machine_get_value(m, m->dst, m->dst_ext)
            ));
            break;
        }
        case XOR: {
            machine_set_value(m, m->dst, m->dst_ext, (
                machine_get_value(m, m->src, m->src_ext) ^
                machine_get_value(m, m->dst, m->dst_ext)
            ));
            break;
        }
        case CMP: {
            uint16_t lhs = machine_get_value(m, m->src, m->src_ext);
            uint16_t rhs = machine_get_value(m, m->dst, m->dst_ext);
            m->flags = ((m->flags & 0xF0)
                | ((lhs == rhs) << 1)
                | (lhs > rhs));
            break;
        }
        case PSH: {
            *(m->sp)++ = machine_get_value(m, m->src, m->src_ext);
            break;
        }
        case POP: {
            machine_set_value(m, m->dst, m->dst_ext, *(m->sp)--);
            break;
        }
        case JMP: {
            if ((m->flags & (1 << (m->dst & 7))) == (((m->dst & 8) >> 3) << (m->dst & 7))) {
                m->pc = m->memory->data + m->dst_ext;
            }
            break;
        }
        case JSR: {
            break;
        }
        case MOV: {
            machine_set_value(m, m->dst, m->dst_ext, 
                machine_get_value(m, m->src, m->src_ext)
            );
            break;
        }
    }
}

void machine_free (machine *m) {
    memory_free(m->memory);
    free(m);
}
