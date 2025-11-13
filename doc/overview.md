# bbb Architecture Overview

```
Opcodes                                     Registers
┌─────┬─────┬─────┬─────┐                   ┌─────┬─────┬─────┬─────┐
│ NOP │ INC │ DEC │ ADD │                   │  A  │  B  │  C  │  D  │
├─────┼─────┼─────┼─────┤                   ├─────┼─────┼─────┼─────┤
│ SUB │ RLC │ RRC │ AND │                   │  E  │  F  │ S0  │ S1  │
├─────┼─────┼─────┼─────┤                   ├─────┼─────┼─────┼─────┤
│ OR  │ XOR │ CMP │ PSH │                   │ TA  │ IX  │ IV  │ SP  │
├─────┼─────┼─────┼─────┤                   ├─────┼─────┴─────┴─────┘
│ POP │ JMP │ JSR │ MOV │                   │ PC  │ ┌─────┬─────┬─────┐
└─────┴─────┴─────┴─────┘                   └─────┘ │ CV  │ MD  │ MX  │
                                               │    └─────┴─────┴─────┘
                                               │             │
   ┌───────────────────────────────────────────┘             │
   │                                                         │
   │                                           ┌─────────────┘
   ▼                                           ▼

Physical Registers                          Virtual Registers
┌───────────────────────┐                   ┌─ ─── ─── ─── ─── ─── ─┐
│  A    B    C    D     │                   │  CV (Constant Value)  │
│ ┌────┬────┬────┬────┐ │                     ┌────┬──────────────┐
│ │    │    │    │    │ │                   │ │    │    |    |    │ │
│ └────┴────┴────┴────┘ │                   │ └────┴──────────────┘ │
│                       │
│  E    F    S1   S0    │                   │  MD (Memory Direct)   │
│ ┌────┬────┬────┬────┐ │                   │ ┌───────────────────┐ │
│ │    │    │TFHI│OCNZ│ │                     │    |    |    |    │
│ └────┴────┴────┴────┘ │                   │ └───────────────────┘ │
│                       │                   │                       │
│  TA (Temporary Addr.) │                      MX (Memory Indexed)
│ ┌───────────────────┐ │                   │ ┌───────────────────┐ │
│ │    |    |    |    │ │                   │ │    |    |    |    │ │
│ └───────────────────┘ │                     └───────────────────┘
│                       │                   │                       │
│  IX (Index)           │                   └─ ─── ─── ─── ─── ─── ─┘
│ ┌───────────────────┐ │
│ │    |    |    |    │ │                   Virtual registers indicate that the
│ └───────────────────┘ │                   instruction is extended with addi-
│                       │                   tional bytes of data containing
│  IV (Inter. Vector)   │                   either a constant value or a memory
│ ┌───────────────────┐ │                   address.
│ │    |    |    |    │ │
│ └───────────────────┘ │                   The constant value virtual register
│                       │                   is only valid as the source argu-
│  SP (Stack Pointer)   │                   ment to an opcode. The number of
│ ┌───────────────────┐ │                   words to read is determined by the
│ │    |    |    |    │ │                   size of the destination register.
│ └───────────────────┘ │
│                       │                   The memory direct register indi-
│  PC (Program Counter) │                   cates that the instruction's exten-
│ ┌───────────────────┐ │                   ded source or destination value is
│ │    |    |    |    │ │                   a memory address whose contents
│ └───────────────────┘ │                   should be read or written.
│                       │
└───────────────────────┘                   The memory indexed register indi-
                                            cates that the extended source or
General-purpose registers A through         destination value is an offset to
F are four-bit words that can be            be added to the index register and
used as the source or destination           used as the memory location to be
of ALU operations.                          read or written.

The status registers S0 and S1 con-
tain bit flags used to indicate CPU
state. The S0 flags indicate prop-
erties of the most recent ALU oper-
ation, while the S1 flags contain
the constants True and False, as
well as halt and interrupt state
flags.

S0                  S1
┌───┬───────────┐   ┌───┬───────────┐
│ N │ Negative  │   │ I │ Interrupt │
├───┼───────────┤   ├───┼───────────┤
│ Z │ Zero      │   │ H │ Halt      │
├───┼───────────┤   ├───┼───────────┤
│ C │ Carry     │   │ F │ False     │
├───┼───────────┤   ├───┼───────────┤
│ O │ Overflow  │   │ T │ True      │
└───┴───────────┘   └───┴───────────┘

The memory address registers TA, IX,
IV, SP, and PC are sixteen bits wide
and are not connected to the ALU.

The TA (Temporary Address) register is
a sixteen bit wide register that can
hold a memory address.

The IX (Index) register holds the
memory base address for memory-indexed
addressing.

The IV (Interrupt Vector) register
holds the starting memory address of
the interrupt handling routine.

The SP (Stack Pointer) register holds
the address of the top of the program's
call stack.

The PC (Program Counter) register holds
the memory address of the next value to
be read from memory in the CPU's fetch,
decode, and execute cycle.

```
