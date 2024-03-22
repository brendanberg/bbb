#include "cpu.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "io.h"
#include "memory.h"

uint16_t prev_keymap = 0;

#define READ_NEXT(x) (*(x)++)
#define READ_QUARTET(x) ((uint16_t)(*(x)++ << 12 | *(x)++ << 8 | *(x)++ << 4 | *(x)++))

#define SET_HALT(x) ((x) |= FLAG_H)
#define SET_CMP(x, lhs, rhs) \
    ((x) = (uint8_t)((x) & 0xFC) | (uint8_t)(((lhs) == (rhs)) << 1) | ((lhs) > (rhs)))
// TODO: Does the lhs > rhs comparison need to be aware of signedness...
#define SET_ZN(x, val) \
    ((x) = (uint8_t)((x) & 0xFC) | (uint8_t)(((val) ? 0 : 1) << 1) | ((uint8_t)(val) < 0 ? 1 : 0))
#define SET_N4(x, val) ((x) = (uint8_t)((x) & 0xFE) | ((val) & 0x08 ? 1 : 0))
#define SET_N16(x, val) ((x) = (uint8_t)((x) & 0xFE) | ((val) & 0x8000 ? 1 : 0))
#define SET_O(x, val) ((x) = (uint8_t)((x) & ~FLAG_O) | ((val) ? FLAG_O : 0))
#define SET_C(x, val) ((x) = (uint8_t)((x) & ~FLAG_C) | ((val) ? FLAG_C : 0))
// General purpose registers are 4-bit. Special registers are 16-bit
static inline void machine_instr_fetch(machine *m);
static inline void machine_instr_decode(machine *m);
static inline void machine_instr_execute(machine *m);

static inline void machine_interrupt_check(machine *m);

static inline uint16_t machine_get_value(machine *m, Register src, uint16_t src_ext) {
    switch (src) {
    case REGISTER_CV:
        return src_ext;
    case REGISTER_MD:
        return memory_read(m->memory, src_ext);
    case REGISTER_MX:
        return memory_read_indexed(m->memory, m->ix, src_ext);
    case REGISTER_PC:
        return (unsigned short)(m->pc - m->memory->data);
    case REGISTER_SP:
        return (unsigned short)(m->sp - m->memory->data);
    case REGISTER_IV:
        return (unsigned short)(m->iv - m->memory->data);
    case REGISTER_IX:
        return (unsigned short)(m->ix - m->memory->data);
    case REGISTER_TA:
        return (unsigned short)(m->ta - m->memory->data);
    case REGISTER_S0:
        return m->flags & MASK_S0;
    case REGISTER_S1:
        return m->flags >> 4;
    default:
        return m->registers[src];
    }
}

static inline void machine_set_value(machine *m, Register dst, uint16_t dst_ext, uint16_t value) {
    switch (dst) {
    case REGISTER_CV:
        m->flags |= FLAG_H;
        break;
    case REGISTER_MD:
        memory_write(m->memory, dst_ext, value & 0xF);
        break;
    case REGISTER_MX:
        memory_write_indexed(m->memory, m->ix, dst_ext, value & 0xF);
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
        m->flags = (m->flags & 0xB0) | (value & 0x0F) | 0x80;
        break;
    case REGISTER_S1:
        m->flags = (m->flags & 0x8F) | (uint8_t)((value & 0x03) << 4) | FLAG_1;
        break;
    default:
        m->registers[dst] = value & 0xF;
        break;
    }
}

machine *machine_init(size_t size) {
    machine *m = malloc(sizeof(machine));
    m->memory = memory_init(size);
    machine_reset(m);
    kbio_setup();
    printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
    // fprintf(stderr, "Starting up...\n");
    return m;
}

#define RENDER(t, c, e) ((t) ? printf((c), (t)) : printf((e), (t)))
#define E(x) "\e[" #x
#define E385(x) "\e[38:5:" #x
#define E0(x) "\e[0;" #x

void machine_show(machine *m) {
    // Return to home
    printf(E(11A) E(?25l) "\r\n" E385(13m));
    printf("╔═════════════╤════════╤══════════════════════════════╗\r\n");
    printf("║" E385(6m) " A B C D E F " E385(13m));
    printf("│" E385(6m) " HIOCZN " E385(13m));
    printf("│" E385(6m) " PROG  STAK  INTR  INDX  TEMP " E385(13m) "║\r\n");
    printf("║" E385(5m) " ");

    RENDER(m->registers[REGISTER_A], E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(m->registers[REGISTER_B], E(1m) E385(5m) "%X " E0(35m), E385(11m) "%X ");
    RENDER(m->registers[REGISTER_C], E(1m) E385(5m) "%X " E0(35m), E385(11m) "%X ");
    RENDER(m->registers[REGISTER_D], E(1m) E385(5m) "%X " E0(35m), E385(11m) "%X ");
    RENDER(m->registers[REGISTER_E], E(1m) E385(5m) "%X " E0(35m), E385(11m) "%X ");
    RENDER(m->registers[REGISTER_F], E(1m) E385(5m) "%X " E0(35m), E385(11m) "%X ");

    printf(E385(13m) "│" E385(4m) " ");

    printf(m->flags & 0x20 ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & 0x10 ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & 0x08 ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & 0x04 ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & 0x02 ? E385(5m) "✺" E385(4m) : E385(11m) "◌");
    printf(m->flags & 0x01 ? E385(5m) "✺" E385(4m) : E385(11m) "◌");

    printf(" " E385(13m) "│" E385(5m));

    RENDER((uint16_t)(m->pc - m->memory->data), " " E(1m) E385(5m) "%04X" E0(35m) " ",
           " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->sp - m->memory->data), " " E(1m) E385(5m) "%04X" E0(35m) " ",
           " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->iv - m->memory->data), " " E(1m) E385(5m) "%04X" E0(35m) " ",
           " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->ix - m->memory->data), " " E(1m) E385(5m) "%04X" E0(35m) " ",
           " " E385(11m) "%04X ");
    RENDER((uint16_t)(m->ta - m->memory->data), " " E(1m) E385(5m) "%04X" E0(35m) " ",
           " " E385(11m) "%04X ");

    printf(E385(13m) "║\r\n");
    printf("╠═════════╤═══╧════════╧══════════════════════════════╣\r\n");

    printf("║ ");
    // printf("%-1X ", m->memory->data[0xF000]);
    RENDER(m->memory->data[0xF000], E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF001), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF002), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF003), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    printf(E385(13m) "│                                           ║\r\n");
    printf("║ ");
    RENDER(memory_read(m->memory, 0xF004), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF005), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF006), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF007), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    printf(E385(13m) "│                                           ║\r\n");
    printf("║ ");
    RENDER(memory_read(m->memory, 0xF008), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF009), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF00A), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF00B), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    printf(E385(13m) "│                                           ║\r\n");
    printf("║ ");
    RENDER(memory_read(m->memory, 0xF00C), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF00D), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF00E), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    RENDER(memory_read(m->memory, 0xF00F), E(1m) E385(5m) "%01X " E0(35m), E385(11m) "%01X ");
    printf(E385(13m) "│                                           ║\r\n");

    printf("╚═════════╧═══════════════════════════════════════════╝\r\n");
    printf(E(0m) "\r\n" E(?25h));
}

static inline void machine_io(machine *m) {
    // Get the currently pressed keys. If they changed since last time and the
    // interrupt flag is not asserted, set the keyboard value in the
    // memory-mapped IO segment and assert the interrupt flag.
    char meta = 0;
    uint16_t keymap = kbio_get_keymap(&meta);

    if (meta == 'q') {
        m->flags |= FLAG_H;  // Set halt flag
        return;
    }
    //  && (m->flags & FLAG_I) == 0
    if (keymap != prev_keymap && !m->int_mask) {
        m->memory->data[0xFFF0] = keymap & 0x000F;
        m->memory->data[0xFFF1] = (keymap & 0x00F0) >> 4;
        m->memory->data[0xFFF2] = (keymap & 0x0F00) >> 8;
        m->memory->data[0xFFF3] = (keymap & 0xF000) >> 12;

        prev_keymap = keymap;
        m->flags |= FLAG_I;  // Set interrupt flag
    }
}

void machine_start(machine *m) {
    uint16_t pc = READ_QUARTET(m->pc);
    m->sp += READ_QUARTET(m->pc);
    m->iv += READ_QUARTET(m->pc);
    m->ix += READ_QUARTET(m->pc);
    m->ta += READ_QUARTET(m->pc);
    m->pc = m->memory->data + pc;
    m->status = STATE_RUN;
}

void machine_pause(machine *m);
void machine_halt(machine *m);

void machine_reset(machine *m) {
    m->status = STATE_HALT;
    m->pc = m->sp = m->iv = m->ix = m->ta = m->memory->data;
    m->flags = 0x80;

    for (uint8_t i = 0; i < 6; i++) {
        m->registers[i] = 0;
    }
}

void machine_run(machine *m) {
    while (!(m->flags & 0x20)) {
        machine_instr_fetch(m);
        machine_instr_decode(m);
        machine_instr_execute(m);
        machine_show(m);
        machine_io(m);
        machine_interrupt_check(m);
    }
}

static inline void machine_interrupt_check(machine *m) {
    if (!(m->flags & 0x10) || m->int_mask) {
        return;
    } else {
        m->int_mask = true;
        // uint16_t dest = (uint16_t)(m->iv - m->memory->data);

        uint16_t pc = (uint16_t)(m->pc - m->memory->data);
        *(m->sp)++ = pc & 0xF;
        *(m->sp)++ = (pc >> 4) & 0xF;
        *(m->sp)++ = (pc >> 8) & 0xF;
        *(m->sp)++ = (pc >> 12) & 0xF;
        m->pc = m->iv;  // m->memory->data + dest;
    }
}

static inline void machine_instr_fetch(machine *m) { m->instr = READ_NEXT(m->pc); }

static inline void machine_instr_decode(machine *m) {
    switch (m->instr) {
    case NOP: {
        break;
    }
    case ADD:
    case SUB:
    case AND:
    case OR:
    case XOR:
    case CMP:
    case MOV: {
        m->src = (Register)READ_NEXT(m->pc);
        m->dst = (Register)READ_NEXT(m->pc);

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
        default:
            break;
        }

        switch (m->dst) {
        case REGISTER_CV: {
            // Read-only registers are not valid destinations; HALT.
            exit(1);
            SET_HALT(m->flags);
            break;
        }
        case REGISTER_MD:
        case REGISTER_MX: {
            m->dst_ext = READ_QUARTET(m->pc);
            break;
        }
        default:
            break;
        }

        break;
    }

    case INC:
    case DEC:
    case RLC:
    case RRC:
    case POP: {
        m->dst = (Register)READ_NEXT(m->pc);

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
        default:
            break;
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
        default:
            break;
        }

        break;
    }
    }
}

static inline void machine_instr_execute(machine *m) {
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
        uint16_t src = machine_get_value(m, m->src, m->src_ext);
        uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);
        uint16_t value = src + dst;

        if (m->flags & 0x04) {
            value += 1;
        }
        machine_set_value(m, m->dst, m->dst_ext, value);
        SET_O(m->flags, (src & 0x8 && dst & 0x8) ^ (src & 0x10 && dst & 0x10));
        SET_C(m->flags, value & 0x10);
        SET_ZN(m->flags, value);
        break;
    }
    case SUB: {
        uint16_t src = machine_get_value(m, m->src, m->src_ext);
        uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);
        uint16_t value = dst - src;
        machine_set_value(m, m->dst, m->dst_ext, value);
        // TODO IS THIS RIGHT?
        SET_O(m->flags, (src & 0x8 && dst & 0x8) ^ (src & 0x10 && dst & 0x10));
        SET_C(m->flags, value & 0x10);
        SET_ZN(m->flags, value);
        break;
    }
    case RLC: {
        uint16_t value = machine_get_value(m, m->dst, m->dst_ext);
        uint8_t flags = m->flags;
        // TODO: WTF is going on here?
        m->flags = (uint8_t)((value & 0xA000) | (flags & ~FLAG_C));
        value = (uint16_t)(value << 1) | ((flags & FLAG_C) >> 2);
        machine_set_value(m, m->dst, m->dst_ext, value);
        SET_ZN(m->flags, value);
        break;
    }
    case RRC: {
        uint16_t value = machine_get_value(m, m->dst, m->dst_ext);
        uint8_t flags = m->flags;
        // TODO: WTF again?
        m->flags = (value & 0x1) | (flags & ~FLAG_C);
        value = (value >> 1) | (uint16_t)((flags & FLAG_C) << 13);
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
        if (((m->src < REGISTER_PC || m->src > REGISTER_CV) && m->dst >= REGISTER_PC &&
             m->dst < REGISTER_CV) ||
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
        uint16_t value = machine_get_value(m, m->src, m->src_ext);
        *(m->sp)++ = value & 0xF;

        if (m->src >= REGISTER_PC && m->src <= REGISTER_TA) {
            *(m->sp)++ = (value >> 4) & 0xF;
            *(m->sp)++ = (value >> 8) & 0xF;
            *(m->sp)++ = (value >> 12) & 0xF;
        }
        break;
    }
    case POP: {
        uint16_t value = *(--m->sp) & 0xF;

        if (m->dst >= REGISTER_PC && m->dst <= REGISTER_TA) {
            value = value << 4 | *(--m->sp) & 0xF;
            value = value << 4 | *(--m->sp) & 0xF;
            value = value << 4 | *(--m->sp) & 0xF;
        }
        machine_set_value(m, m->dst, m->dst_ext, value);

        if (m->int_mask && ~m->flags & FLAG_I) {
            // The interrupt mask has been cleared by software
            // so we can unmask the interrupt
            m->int_mask = false;
        }
        break;
    }
    case JMP: {
        // The jump specifier value is a 4-bit value in the form F I I I, where
        // F is the desired flag value and I I I is the offset into the S
        // register.
        //
        // For example, to jump if the zero flag is set, the jump specifier
        // value would be 1001, meaning execution will continue at the target
        // address if bit 1 of the S register is 1.
        //
        // As another example, to jump if the interrupt flag is not set, the
        // jump specifier value would be 0 1 0 0, indicating that the 4th status
        // bit should be 0.
        //
        // To unconditionally jump, use the jump specifier 1111, meaning
        // execution will continue at the target address if the 7th bit of the S
        // register is 1.

        if ((m->flags & (1 << (m->dst & 7))) == (((m->dst & 8) >> 3) << (m->dst & 7))) {
            m->pc = m->memory->data + m->dst_ext;
        }
        break;
    }
    case JSR: {
        if ((m->flags & (1 << (m->dst & 7))) == (((m->dst & 8) >> 3) << (m->dst & 7))) {
            uint16_t pc = (uint16_t)(m->pc - m->memory->data);
            *(m->sp)++ = pc & 0xF;
            *(m->sp)++ = (pc >> 4) & 0xF;
            *(m->sp)++ = (pc >> 8) & 0xF;
            *(m->sp)++ = (pc >> 12) & 0xF;
            m->pc = m->memory->data + m->dst_ext;
        }
        break;
    }
    case MOV: {
        uint16_t value = machine_get_value(m, m->src, m->src_ext);

        if (m->dst >= REGISTER_PC && m->dst < REGISTER_CV) {
            // The destination is a 16-bit register
            uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);

            if (m->src < REGISTER_PC || m->src > REGISTER_CV) {
                // Moving a 4-bit value into a 16-bit register.
                // Combine the masked src and dst values
                value = (dst & 0xFFF0) | (value & 0xF);
            }

            machine_set_value(m, m->dst, m->dst_ext, value);
            //     // Moving
        } else {
            // fprintf(stderr, "machine_get_value(m, %X, %X) = %d\n", m->src, m->src_ext, value);
            machine_set_value(m, m->dst, m->dst_ext, value);
        }
        break;
    }
    }
}

void machine_free(machine *m) {
    memory_free(m->memory);
    free(m);
}
