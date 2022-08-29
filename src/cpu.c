#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "io.h"
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
        return memory_read(m->memory, src_ext);
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

static inline void machine_set_value(machine *m, Register dst, uint16_t dst_ext, uint16_t value) {
    switch (dst) {
    case REGISTER_CV:
        m->flags |= 0x20;
        break;
    case REGISTER_MD:
        memory_write(m->memory, dst_ext, value);
        break;
    case REGISTER_MX:
        m->memory->data[(m->ix - m->memory->data) + dst_ext] = value & 0xF;
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
        m->flags = (value & 0x03) << 4;
        break;
    default:
        m->registers[dst] = value;
        break;
    }


}

machine *machine_init (size_t size) {
    machine *m = malloc(sizeof(machine));
    m->memory = memory_init(size);
    machine_reset(m);
    kbio_setup();
    printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
    // fprintf(stderr, "Starting up...\n");
    return m;
}

#define RENDER(t, c, e) ((t)?printf((c),(t)):printf((e),(t)))
#define E(x) "\e[" #x

void machine_show (machine *m) {
    // Return to home
    printf(E(8A) E(?25l) "\r\n" E(38:5:13m));
    printf("╔═════════════╤════════╤══════════════════════════════╗\r\n");
    printf("║" E(38:5:6m) " A B C D E F " E(38:5:13m));
    printf("│" E(38:5:6m) " HIOCZN " E(38:5:13m));
    printf("│" E(38:5:6m) " PROG  STAK  INTR  INDX  TEMP " E(38:5:13m) "║\r\n");
    printf("║" E(38:5:5m) " ");

    RENDER(m->registers[REGISTER_A], E(1m) E(38:5:5m) "%X " E(0;35m), E(38:5:11m) "%X ");
    RENDER(m->registers[REGISTER_B], E(1m) E(38:5:5m) "%X " E(0;35m), E(38:5:11m) "%X ");
    RENDER(m->registers[REGISTER_C], E(1m) E(38:5:5m) "%X " E(0;35m), E(38:5:11m) "%X ");
    RENDER(m->registers[REGISTER_D], E(1m) E(38:5:5m) "%X " E(0;35m), E(38:5:11m) "%X ");
    RENDER(m->registers[REGISTER_E], E(1m) E(38:5:5m) "%X " E(0;35m), E(38:5:11m) "%X ");
    RENDER(m->registers[REGISTER_F], E(1m) E(38:5:5m) "%X " E(0;35m), E(38:5:11m) "%X ");

    printf(E(38:5:13m) "│" E(38:5:4m) " ");

    printf(m->flags & 0x20 ? E(38:5:5m) "✺" E(38:5:4m) : E(38:5:11m) "◌");
    printf(m->flags & 0x10 ? E(38:5:5m) "✺" E(38:5:4m) : E(38:5:11m) "◌");
    printf(m->flags & 0x08 ? E(38:5:5m) "✺" E(38:5:4m) : E(38:5:11m) "◌");
    printf(m->flags & 0x04 ? E(38:5:5m) "✺" E(38:5:4m) : E(38:5:11m) "◌");
    printf(m->flags & 0x02 ? E(38:5:5m) "✺" E(38:5:4m) : E(38:5:11m) "◌");
    printf(m->flags & 0x01 ? E(38:5:5m) "✺" E(38:5:4m) : E(38:5:11m) "◌");

    printf(" " E(38:5:13m) "│" E(38:5:5m));

    RENDER((uint16_t) (m->pc - m->memory->data), " " E(1m) E(38:5:5m) "%04X" E(0;35m) " ", " " E(38:5:11m) "%04X ");
    RENDER((uint16_t) (m->sp - m->memory->data), " " E(1m) E(38:5:5m) "%04X" E(0;35m) " ", " " E(38:5:11m) "%04X ");
    RENDER((uint16_t) (m->iv - m->memory->data), " " E(1m) E(38:5:5m) "%04X" E(0;35m) " ", " " E(38:5:11m) "%04X ");
    RENDER((uint16_t) (m->ix - m->memory->data), " " E(1m) E(38:5:5m) "%04X" E(0;35m) " ", " " E(38:5:11m) "%04X ");
    RENDER((uint16_t) (m->ta - m->memory->data), " " E(1m) E(38:5:5m) "%04X" E(0;35m) " ", " " E(38:5:11m) "%04X ");

    printf(E(38:5:13m) "║\r\n");
    printf("╚═════════════╧════════╧══════════════════════════════╝" E(0m));
    // printf("⢕⢕⢕⢕⢕⢕⢕⢕⢕⢕\r\n");
    // printf("⢕⢕⣿⣿⣿⣿⣿⣿⣿⣿\r\n");
    // printf("⢕⢕⣿        \r\n");
    printf("  %0X%0X%0X%0X",
        m->memory->data[0xFFF0],
        m->memory->data[0xFFF1],
        m->memory->data[0xFFF2],
        m->memory->data[0xFFF3]
    );
    printf("\r\n\r\n\r\n\r\n" E(?25h));
}

static inline void machine_io (machine *m) {
    uint16_t keymap = kbio_get_keymap();
    m->memory->data[0xFFF0] = keymap & 0x000F;
    m->memory->data[0xFFF1] = (keymap & 0x00F0) >> 4;
    m->memory->data[0xFFF2] = (keymap & 0x0F00) >> 8;
    m->memory->data[0xFFF3] = (keymap & 0xF000) >> 12;
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

#define SET_HALT(x) ((x) |= 0x20)
#define SET_CMP(x, lhs, rhs) ((x) = ((x) & 0xFC) | (((lhs) == (rhs)) << 1) | ((lhs) > (rhs)))
#define SET_ZN(x, val) ((x) = ((x) & 0xFC) | ((val) ? 0 : 1) << 1 | ((val) < 0 ? 1 : 0))

void machine_run (machine *m) {
    while (!(m->flags & 0x20)) {
        machine_instr_fetch(m);
        machine_instr_decode(m);
        machine_instr_execute(m);
        machine_show(m);
        machine_io(m);
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
                    if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
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
                    SET_HALT(m->flags);
                    break;
                }
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->dst_ext = READ_QUARTET(m->pc);
                    break;
                }
                default: break;
            }

            break;
        }

        case INC:
        case DEC:
        case RLC:
        case RRC:
        case POP: {
            m->dst = (Register) READ_NEXT(m->pc);

            switch (m->dst) {
                case REGISTER_CV: {
                    // Read-only registers are not valid destinations; HALT.
                    SET_HALT(m->flags);
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
                case REGISTER_CV: {
                    m->src_ext = READ_NEXT(m->pc);
                    break;
                }
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->src_ext = READ_QUARTET(m->pc);
                    break;
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
            SET_ZN(m->flags, value);
            break;
        }
        case DEC: {
            uint16_t value = machine_get_value(m, m->dst, m->dst_ext) - 1;
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
            break;
        }
        case ADD: {
            // Set overflow and carry based on the target register width
            uint16_t value = (machine_get_value(m, m->src, m->src_ext) +
                machine_get_value(m, m->dst, m->dst_ext));
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
            break;
        }
        case SUB: {
            uint16_t value = machine_get_value(m, m->dst, m->dst_ext) -
                machine_get_value(m, m->src, m->src_ext);
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
            break;
        }
        case RLC: {
            uint16_t value = machine_get_value(m, m->dst, m->dst_ext);
            uint8_t flags = m->flags;
            m->flags = (value & 0xA000) | (flags & 0xFB);
            value = (value << 1) | ((flags & 0x4) >> 2);
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
        break;
        }
        case RRC: {
            uint16_t value = machine_get_value(m, m->dst, m->dst_ext);
            uint8_t flags = m->flags;
            m->flags = (value & 0x1) | (flags & 0xFB);
            value = (value >> 1) | ((flags & 0x4) << 13);
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
            break;
        }
        case AND: {
            uint16_t src = machine_get_value(m, m->src, m->src_ext);
            uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);

            if (m->src < REGISTER_PC && m->dst >= REGISTER_PC) {
                // Moving a 4-bit value into a 16-bit register.
                // Mask off the source value
                src |= 0xFFF0;
            }

            uint16_t value = src & dst;
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
            break;
        }
        case OR: {
            uint16_t src = machine_get_value(m, m->src, m->src_ext);
            uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);

            if (m->src < REGISTER_PC && m->dst >= REGISTER_PC) {
                // Moving a 4-bit value into a 16-bit register.
                // Mask off the source value
                src &= 0x000F;
            }

            uint16_t value = src | dst;
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
            break;
        }
        case XOR: {
            uint16_t src = machine_get_value(m, m->src, m->src_ext);
            uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);

            if (m->src < REGISTER_PC && m->dst >= REGISTER_PC) {
                // Moving a 4-bit value into a 16-bit register.
                // Mask off the source value
                src &= 0x000F;
            }

            uint16_t value = src ^ dst;
            machine_set_value(m, m->dst, m->dst_ext, value);
            SET_ZN(m->flags, value);
            break;
        }
        case CMP: {
            if (((m->src < REGISTER_PC || m->src >= REGISTER_CV) &&
                    m->dst >= REGISTER_PC && m->dst < REGISTER_CV) ||
                    (m->src >= REGISTER_PC && m->src < REGISTER_CV &&
                    (m->dst < REGISTER_PC || m->dst > REGISTER_CV))) {
                // If the src and dst widths don't match, we can't compare
                SET_HALT(m->flags); 
            }

            uint16_t lhs = machine_get_value(m, m->src, m->src_ext);
            uint16_t rhs = machine_get_value(m, m->dst, m->dst_ext);
            SET_CMP(m->flags, lhs, rhs);
            break;
        }
        case PSH: {
            // TODO: Push 4 values if the source is 16 bit?
            *(m->sp)++ = machine_get_value(m, m->src, m->src_ext) & 0xF;
            break;
        }
        case POP: {
            uint16_t value = *(m->sp)-- & 0xF;
            // TODO: Pop 4 values if the destination is 16 bit?
            if (m->dst >= REGISTER_PC) {
                // Moving a 4-bit value into a 16-bit register.
                // Combine the masked src and dst values
                uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);
                machine_set_value(m, m->dst, m->dst_ext, (dst & 0xFFF0) | value);
            } else {
                machine_set_value(m, m->dst, m->dst_ext, value);
            }
            break;
        }
        case JMP: {
            if ((m->flags & (1 << (m->dst & 7))) == (((m->dst & 8) >> 3) << (m->dst & 7))) {
                m->pc = m->memory->data + m->dst_ext;
            }
            break;
        }
        case JSR: {
            if ((m->flags & (1 << (m->dst & 7))) == (((m->dst & 8) >> 3) << (m->dst & 7))) {
                uint16_t pc = m->pc - m->memory->data;
                *(m->sp)++ = pc & 0xF;
                *(m->sp)++ = (pc >> 4) & 0xF;
                *(m->sp)++ = (pc >> 8) & 0xF;
                *(m->sp)++ = (pc >> 12) & 0xF;                
                m->pc = m->memory->data + m->dst_ext;
            }
            break;
        }
        case MOV: {
            uint16_t value = 0;

            if ((m->src < REGISTER_PC || m->src >= REGISTER_CV) && 
                    m->dst >= REGISTER_PC && m->dst < REGISTER_CV) {
                // Moving a 4-bit value into a 16-bit register.
                // Combine the masked src and dst values
                value = machine_get_value(m, m->src, m->src_ext);
                uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);
                machine_set_value(m, m->dst, m->dst_ext, (dst & 0xFFF0) | (value & 0xF));
            } else {
                value = machine_get_value(m, m->src, m->src_ext);
                machine_set_value(m, m->dst, m->dst_ext, value);
            }
            break;
        }
    }
}

void machine_free (machine *m) {
    memory_free(m->memory);
    free(m);
}
