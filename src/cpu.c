#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


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

static uint16_t registers [16];
// General purpose registers are 4-bit. Special registers are 16-bit

typedef struct machine {
    uint16_t registers [16];

} machine;


int load () {}
int store () {}

void instr_decode (machine *p) {}
void instr_fetch (machine *p) {}
void instr_execute (machine *p) {}

int main (int argc, char *argsv[]) {
    return 0;
}
