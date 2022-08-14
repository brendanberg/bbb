#ifndef BBB_CPU_H
#define BBB_CPU_H

#include <stdint.h>
#include "memory.h"


typedef enum {
    STATE_RUN,
    STATE_HALT
} State;

typedef enum {
    REGISTER_A,  // General purpose register A
    REGISTER_B,  // General purpose register B
    REGISTER_C,  // General purpose register C
    REGISTER_D,  // General purpose register D
    REGISTER_E,  // General purpose register E
    REGISTER_F,  // General purpose register F
    REGISTER_S0, // Status register 0 (O, C, Z, N)
    REGISTER_S1, // Status register 1 (1, 0, H, I)
    REGISTER_PC, // Program counter
    REGISTER_SP, // Stack pointer
    REGISTER_IV, // Interrupt vector 
    REGISTER_IX, // Index register
    REGISTER_TA, // Temporary address
    REGISTER_CV, // Virtual register for a constant value
    REGISTER_MD, // Virtual register for memory direct addressing
    REGISTER_MX  // Virtual register for memory indexed addressing
} Register;

typedef enum {
    NOP,  // Do nothing
    INC,  // Increment contents of register or memory
    DEC,  // Decrement contents of register or memory
    ADD,  // Add contents of <src> and <dst> (with carry) and store in <dst>
    SUB,  // Subtract contents of <src> from <dst> (with borrow) and store in <dst>
    RLC,  // Rotate contents of <dst> left through the carry status bit
    RRC,  // Rotate contents of <dst> right through the carry status bit
    AND,  // Logical AND contents of <src> and <dst> and store in <dst>
    OR,   // Logical OR contents of <src> and <dst> and store in <dst>
    XOR,  // Logical XOR contents of <src> and <dst> and store in <dst>
    CMP,  // Compare contents of <src> and <dst> and update C and Z status bits
    PSH,  // Push contents of <src> onto the stack
    POP,  // Pop top of stack and store in <dst>
    JMP,  // Jump to location
    JSR,  // Jump to subroutine
    MOV   // Move the contents of <src> to <dst>
} Opcode;

typedef struct machine {
    uint8_t status;
    uint8_t registers [8];
    memory *memory;
    uint8_t flags;
    // ------
    uint8_t *pc;
    uint8_t *sp;
    uint8_t *iv;
    uint8_t *ix;
    uint8_t *ta;
    uint8_t instr;
    Register src;
    Register dst;
    uint16_t src_ext;
    uint16_t dst_ext;
} machine;

void machine_init (machine **mach, size_t size);

void machine_show (machine *mach);
static inline void machine_io (machine *mach);

void machine_start (machine *mach);
void machine_pause (machine *mach);
void machine_halt (machine *mach);
void machine_reset (machine *mach);

void machine_free (machine *mach);
void machine_run (machine *mach);

#endif
