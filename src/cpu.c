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


void machine_init (machine **m, size_t size) {
    *m = malloc(sizeof(machine));
    memory_init(&((*m)->memory), size);
    machine_reset(*m);
}

void machine_start (machine *m) {
    m->status = STATE_RUN;
}

void machine_pause (machine *m);
void machine_halt (machine *m);

void machine_reset (machine *m) {
    m->status = STATE_HALT;
    m->pc = m->memory->data;
    
    for (uint8_t i = 0; i < 16; i++) {
        m->registers[i] = 0;
    }
}

void machine_run (machine *m) {
    //while (1) {
        machine_instr_fetch(m);

        machine_instr_decode(m);
        machine_instr_execute(m);
    //}
}

static inline void machine_instr_fetch (machine *m) {
    m->instr = *m->pc++;
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
            m->src = (Register) *m->pc++;
            m->dst = (Register) *m->pc++;

            switch (m->src) {
                case REGISTER_CV:
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->src_ext = *m->pc++;
                    m->src_ext |= *m->pc++ << 4;
                    m->src_ext |= *m->pc++ << 8;
                    m->src_ext |= *m->pc++ << 12;
                }
                default: break;
            }
            
            switch (m->dst) {
                case REGISTER_CV:
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->dst_ext = *m->pc++;
                    m->dst_ext |= *m->pc++ << 4;
                    m->dst_ext |= *m->pc++ << 8;
                    m->dst_ext |= *m->pc++ << 12;
                }
                default: break;
            }

            break;
        }

        case INC:
        case DEC:
        case ROLC:
        case RORC:
        case PSH:
        case JMP:
        case JSR: {
            m->dst = (Register) *m->pc++;

            switch (m->dst) {
                case REGISTER_CV:
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->dst_ext = *m->pc++;
                    m->dst_ext |= *m->pc++ << 4;
                    m->dst_ext |= *m->pc++ << 8;
                    m->dst_ext |= *m->pc++ << 12;
                }
                default: break;
            }

            break;
        }

        case POP: {
            m->src = *m->pc++;

            switch (m->src) {
                case REGISTER_CV:
                case REGISTER_MD:
                case REGISTER_MX: {
                    m->src_ext = *m->pc++;
                    m->src_ext |= *m->pc++ << 4;
                    m->src_ext |= *m->pc++ << 8;
                    m->src_ext |= *m->pc++ << 12;
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
            switch (m->dst) {
                case REGISTER_CV: {
                    // This is UB. Halt?
                    break;
                }
                case REGISTER_MD: {
                    m->memory->data[m->dst_ext]++;
                    break;
                }
                case REGISTER_MX:
                break;
                default: {
                    uint16_t *value = m->registers + m->dst;
                    (*value)++;
                }
            }
            break;
        }
        case DEC: {
            switch (m->dst) {
                case REGISTER_CV: {
                    break;
                }
                case REGISTER_MD: {
                    m->memory->data[m->dst_ext]--;
                    break;
                }
                default: {
                    uint16_t *value = m->registers + m->dst;
                    (*value)--;
                }
            }
            break;
        }
        case ADD: {
            uint16_t source;
            switch (m->src) {
                case REGISTER_CV: {
                    source = m->src_ext;
                    break;
                }
                case REGISTER_MD: {
                    source = m->memory->data[m->src_ext];
                    break;
                }
                case REGISTER_MX: {
                    //source = m->memory->data[m->pc + m->src_ext];
                    break;
                }
                default: {
                    source = m->registers[m->src];
                }
            }

            switch (m->dst) {
                case REGISTER_CV: {
                    break;
                }
                case REGISTER_MD: {
                    m->memory->data[m->dst_ext] += source;
                    break;
                }
                case REGISTER_MX: {
                    //m->memory->data[m->pc + m->dst_ext] += source;
                    break;
                }
                default: {
                    m->registers[m->dst] += source;
                }
            }
        }
    }
}

void machine_free (machine *m) {
    memory_free(m->memory);
    free(m);
}
