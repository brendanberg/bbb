#include "cpu.h"
#include "io.h"
#include "memory.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define READ_NEXT(x) (*(x)++)
#define READ_QUARTET(x)                                                        \
    (*(x) << 12 | *((x) + 1) << 8 | *((x) + 2) << 4 | *((x) + 3));             \
    (x) += 4

#define SET_HALT(x) ((x) |= FLAG_HALT)
#define SET_CMP(x, lhs, rhs)                                                   \
    ((x) = ((x) & 0xFC) | (((lhs) == (rhs)) << 1) | ((lhs) > (rhs)))
#define SET_ZN(x, val)                                                         \
    ((x) = ((x) & 0xFC) | ((val) ? 0 : 1) << 1 | (((val) & 0x8000) >> 15))

// General purpose registers are 4-bit. Special registers are 16-bit
extern inline void machine_instr_fetch(machine *m);
extern inline void machine_instr_decode(machine *m);
extern inline void machine_instr_execute(machine *m);

extern inline void machine_interrupt_check(machine *m);
void machine_call_update(machine *m);
void machine_call_teardown(machine *m);

static inline uint16_t machine_get_value(machine *m, Register src,
                                         uint16_t src_ext) {
    switch (src) {
    case REGISTER_CV:
        return src_ext;
    case REGISTER_MD:
        return memory_read(m->memory, src_ext);
    case REGISTER_MX:
        return memory_read_indexed(m->memory, m->ix, src_ext);
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
        return m->flags &
               (FLAG_OVERFLOW | FLAG_CARRY | FLAG_ZERO | FLAG_NEGATIVE);
    case REGISTER_S1:
        return m->flags >> 4;
    default:
        return m->registers[src];
    }
}

static inline void machine_set_value(machine *m, Register dst, uint16_t dst_ext,
                                     uint16_t value) {
    switch (dst) {
    case REGISTER_CV:
        m->flags |= FLAG_HALT;
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
        m->flags = (m->flags & MASK_REGISTER_S1) | (value & MASK_REGISTER_S0);
        break;
    case REGISTER_S1:
        m->flags = (m->flags & MASK_REGISTER_S0) | FLAG_TRUE |
                   ((value << 4) & (FLAG_HALT | FLAG_INTERRUPT));
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
    return m;
}

void machine_start(machine *m) {
    if (m->event_setup != NULL) {
        m->event_setup(m);
    }

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
    m->flags = FLAG_TRUE;

    for (uint8_t i = 0; i < CPU_REGISTER_COUNT; i++) {
        m->registers[i] = 0;
    }
}

void machine_run(machine *m) {
    machine_call_update(m);
    while (!(m->flags & FLAG_HALT)) {
        machine_instr_fetch(m);
        machine_instr_decode(m);
        machine_instr_execute(m);
        machine_call_update(m);
        machine_interrupt_check(m);
    }
    machine_call_update(m);
}

void machine_call_update(machine *m) {
    if (m->event_update != NULL) {
        m->event_update(m);
    }
}

void machine_call_teardown(machine *m) {
    if (m->event_teardown != NULL) {
        m->event_teardown(m);
    }
}

inline void machine_interrupt_check(machine *m) {
    if (!(m->flags & FLAG_INTERRUPT) || m->int_mask) {
        return;
    } else {
        m->int_mask = true;
        uint16_t dest = m->iv - m->memory->data;

        uint16_t pc = m->pc - m->memory->data;
        *(m->sp)++ = pc & 0xF;
        *(m->sp)++ = (pc >> 4) & 0xF;
        *(m->sp)++ = (pc >> 8) & 0xF;
        *(m->sp)++ = (pc >> 12) & 0xF;
        m->pc = m->memory->data + dest;
    }
}

extern inline void machine_instr_fetch(machine *m) {
    m->instr = READ_NEXT(m->pc);
}

extern inline void machine_instr_decode(machine *m) {
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
            // exit(1);
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

extern inline void machine_instr_execute(machine *m) {
    // There are cases where the instruction decode step will set the halt flag
    // and we want execution to stop completely
    if (m->flags & FLAG_HALT) {
        return;
    }

    switch (m->instr) {
    case NOP: {
        break;
    }
    case INC: {
        uint16_t value = machine_get_value(m, m->dst, m->dst_ext) + 1;
        machine_set_value(m, m->dst, m->dst_ext, value);

        if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
            value |= (value & 0x8) << 12;
        }

        SET_ZN(m->flags, value);
        break;
    }
    case DEC: {
        uint16_t value = machine_get_value(m, m->dst, m->dst_ext) - 1;
        machine_set_value(m, m->dst, m->dst_ext, value);

        if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
            value |= (value & 0x8) << 12;
        }

        SET_ZN(m->flags, value);
        break;
    }
    case ADD: {
        // Set overflow and carry based on the target register width
        uint16_t lhs = machine_get_value(m, m->src, m->src_ext);
        uint16_t rhs = machine_get_value(m, m->dst, m->dst_ext);
        uint16_t value = lhs + rhs + ((m->flags & FLAG_CARRY) >> 2);

        if (m->src >= REGISTER_PC && m->src < REGISTER_CV) {
            SET_HALT(m->flags);
            break;
        }

        machine_set_value(m, m->dst, m->dst_ext, value);

        uint8_t flags = 0;

        if ((m->dst < REGISTER_PC || m->dst > REGISTER_CV) &&
            ((lhs & 0x8) == (rhs & 0x8)) && ((lhs & 0x8) != (value & 0x8))) {
            flags |= FLAG_OVERFLOW;
        }

        if (value & 0x10) {
            flags |= FLAG_CARRY;
        }

        m->flags = (m->flags & 0xF0) | flags;

        if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
            value |= (value & 0x8) << 12;
        }

        SET_ZN(m->flags, value);

        break;
    }
    case SUB: {
        uint16_t rhs = machine_get_value(m, m->src, m->src_ext);
        uint16_t lhs = machine_get_value(m, m->dst, m->dst_ext);
        uint16_t value = lhs - rhs - ((m->flags & FLAG_CARRY) >> 2);

        machine_set_value(m, m->dst, m->dst_ext, value);

        uint8_t flags = 0;

        if ((m->dst < REGISTER_PC || m->dst > REGISTER_CV) &&
            ((lhs & 0x8) == (rhs & 0x8)) && ((lhs & 0x8) != (value & 0x8))) {
            flags |= FLAG_OVERFLOW;
        }

        if (value & 0x10) {
            flags |= FLAG_CARRY;
        }

        m->flags = (m->flags & 0xF0) | flags;

        if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
            value = (value & 0x7FFF) | (value & 0x8) << 12;
        }

        SET_ZN(m->flags, value);

        break;
    }
    case RLC: {
        uint16_t value = machine_get_value(m, m->dst, m->dst_ext);
        uint8_t carry = m->flags & FLAG_CARRY;
        uint8_t flags = m->flags & ~FLAG_CARRY;
        uint16_t mask;

        if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
            m->flags = flags | ((value & 0x8) >> 1);
            mask = 0x000F;
        } else {
            m->flags = flags | ((value & 0x8000) >> 13);
            mask = 0xFFFF;
        }

        value = ((value << 1) | (carry >> 2)) & mask;
        machine_set_value(m, m->dst, m->dst_ext, value);
        SET_ZN(m->flags, value);
        break;
    }
    case RRC: {
        uint16_t value = machine_get_value(m, m->dst, m->dst_ext);
        uint16_t carry = m->flags & FLAG_CARRY;
        uint8_t flags = m->flags & ~FLAG_CARRY;
        uint16_t mask;

        m->flags = flags | ((value & 0x1) << 2);

        if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
            mask = 0x000F;
            carry = carry << 1;
        } else {
            mask = 0xFFFF;
            carry <<= 13;
        }

        value = ((value >> 1) | carry) & mask;
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
        if (((m->src < REGISTER_PC || m->src > REGISTER_CV) &&
             m->dst >= REGISTER_PC && m->dst < REGISTER_CV) ||
            (m->src >= REGISTER_PC && m->src < REGISTER_CV &&
             (m->dst < REGISTER_PC || m->dst > REGISTER_CV))) {
            // If the src and dst widths don't match, we can't compare
            SET_HALT(m->flags);
            break;
        }

        uint16_t lhs = machine_get_value(m, m->src, m->src_ext);
        uint16_t rhs = machine_get_value(m, m->dst, m->dst_ext);
        SET_CMP(m->flags, lhs, rhs);
        break;
    }
    case PSH: {
        // TODO: Push 4 values if the source is 16 bit?
        uint16_t value = machine_get_value(m, m->src, m->src_ext);

        if (m->src < REGISTER_PC || m->src > REGISTER_TA) {
            *(m->sp)++ = value & 0xF;
        } else {
            *(m->sp)++ = (value >> 12) & 0xF;
            *(m->sp)++ = (value >> 8) & 0xF;
            *(m->sp)++ = (value >> 4) & 0xF;
            *(m->sp)++ = (value >> 0) & 0xF;
        }

        break;
    }
    case POP: {
        uint16_t value;

        if (m->dst < REGISTER_PC || m->dst > REGISTER_CV) {
            value = *--(m->sp);
        } else {
            value = *--(m->sp);
            value |= *--(m->sp) << 4;
            value |= *--(m->sp) << 8;
            value |= *--(m->sp) << 12;
        }

        machine_set_value(m, m->dst, m->dst_ext, value);

        if (m->dst == REGISTER_PC && m->int_mask &&
            ~m->flags & FLAG_INTERRUPT) {
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

        if ((m->flags & (1 << (m->dst & 7))) ==
            (((m->dst & 8) >> 3) << (m->dst & 7))) {
            m->pc = m->memory->data + m->dst_ext;
        }
        break;
    }
    case JSR: {
        if ((m->flags & (1 << (m->dst & 7))) ==
            (((m->dst & 8) >> 3) << (m->dst & 7))) {
            uint16_t pc = m->pc - m->memory->data;
            *(m->sp)++ = (pc >> 12) & 0xF;
            *(m->sp)++ = (pc >> 8) & 0xF;
            *(m->sp)++ = (pc >> 4) & 0xF;
            *(m->sp)++ = (pc >> 0) & 0xF;
            m->pc = m->memory->data + m->dst_ext;
        }
        break;
    }
    case MOV: {
        uint16_t value = 0;

        if ((m->src < REGISTER_PC || m->src > REGISTER_CV) &&
            m->dst >= REGISTER_PC && m->dst < REGISTER_CV) {
            // Moving a 4-bit value into a 16-bit register.
            // Combine the masked src and dst values
            value = machine_get_value(m, m->src, m->src_ext);
            uint16_t dst = machine_get_value(m, m->dst, m->dst_ext);
            machine_set_value(m, m->dst, m->dst_ext,
                              (dst & 0xFFF0) | (value & 0xF));
        } else {
            value = machine_get_value(m, m->src, m->src_ext);
            machine_set_value(m, m->dst, m->dst_ext, value);
        }
        break;
    }
    }
}

void machine_free(machine *m) {
    machine_call_teardown(m);
    memory_free(m->memory);
    free(m);
}
