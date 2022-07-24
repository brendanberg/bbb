#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "memory.h"


enum {
    NOP,  // Do nothing
    INC,  // Increment contents of register or memory
    DEC,  // Decrement contents of register or memory
    ADD,  // Add contents of <src> and <dst> (with carry) and store in <dst>
    SUB,  // Subtract contents of <src> from <dst> (with borrow) and store in <dst>
    ROLC, // Rotate contents of <dst> left through the carry status bit
    RORC, // Rotate contents of <dst> right through the carry status bit
    AND,  // Logical AND contents of <src> and <dst> and store in <dst>
    OR,   // Logical OR contents of <src> and <dst> and store in <dst>
    XOR,  // Logical XOR contents of <src> and <dst> and store in <dst>
    CMP,  // Compare contents of <src> and <dst> and update C and Z status bits
    PSH,  // Push contents of <src> onto the stack
    POP,  // Pop top of stack and store in <dst>
    JMP,  // Jump to location
    JSR,  // Jump to subroutine
    MOV   // Move the contents of <src> to <dst>
};

// General purpose registers are 4-bit. Special registers are 16-bit

void machine_init (machine *m, size_t size) {
    m = malloc(sizeof machine);
    m->registers = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    memory_init(m->memory, size);
}

int machine_load () {}
int store () {}

void machine_instr_fetch (machine *p) {}
void machine_instr_decode (machine *p) {}
void machine_instr_execute (machine *p) {}

void machine_free (machine *m) {
    memory_free(m->memory);
    free(m);
}
