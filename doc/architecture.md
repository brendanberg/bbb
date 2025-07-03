# bbb Architecture

This document describes the overall organization of the _bbb_ architecture, including bus layout, registers, control signals, state machine, and initialization procedure.

The _bbb_ CPU is a bare-bones architecture intended for learning and experimentation. It has 16 variable-width instructions, a 4-bit data path, and a 16-bit address path.

## Registers

The _bbb_ CPU has six four-bit general purpose registers, a pair of four-bit status registers, and five address registers. There are three additional "virtual" registers used to indicate constant values or memory access.

The general purpose regisers, `A` through `F`, are four bits wide and may be used as inputs to and outputs from the ALU.

ALU operations on general purpose registers update the `S0` status flag register.

The five address registers are 16 bits wide, and consist of the program counter (`PC`), stack pointer (`SP`), interrupt vector (`IV`), memory base index (`IX`), and temporary address (`TA`).

The address registers may be the source or destination in increment, decrement, push, pop, move, add, subtract, compare, and jump operations.

The three "virtual" registers are to indicate an operand is either an constant (immediate) value, or a memory read or write.

If the constant value (`CV`) register is specified in a source operand, it indicates that the 4 or 16 bits following the instruction are a literal value to be used. The number of words to be read depend on the type of destination register. If the destination register is general purpose, one word is read from memory and placed in the destination. If the destination is an address register, four words are read sequentially from memory and placed into the destination. If the destination is a memory virtual register, one word is read from the program counter location and written to the specified address.

For example, the machine code below adds the constant value 9 to the value currently in register B and stores the result in B:

```
+------+------+------+------+
| 0011 | 1101 | 0001 | 1001 |
+------+------+------+------+
  ADD    CV     B      0x9
```

And the next example shows how a programmer would store the memory address 0x002A in the program counter:

```
+------+------+------+------+------+------+------+
| 0111 | 1101 | 1000 | 0000 | 0000 | 0010 | 1010 |
+------+------+------+------+------+------+------+
  MOV    CV     PC     0x0    0x0    0x2    0xA
```

The memory direct (`MD`) virtual register specifies that the source or destination operand is to be a value found at the address specified by the four words that follow. Similarly, the memory indexed (`MX`) register specifies that the operand is to be added to the value at the index register before reading or writing the value.

The machine code below illustrates reading a value from memory address 0xCAFE and placing it in register D:

```
+------+------+------+------+------+------+------+
| 0111 | 1110 | 0011 | 1100 | 1010 | 1111 | 1110 |
+------+------+------+------+------+------+------+
  MOV    MD     D      0xC    0xA    0xF    0xE
```

In this further example, the constant value 6 is placed into the memory address 24 words past the address referenced in the memory base index (`IX`) register.

```
+------+------+------+------+------+------+
| 0111 | 1101 | 1111 | 0110 | 0001 | 1000 |
+------+------+------+------+------+------+
  MOV    CV     MX     0x6    0x1    0x8
```

## Initialization procedure

When the _bbb_ CPU first powers on, it loads the first five memory locations starting from 0x0000 into the PC, SP, IR, IX, and TA registers. The CPU then transitions into the `Running` state and begins execution with the fetch, decode, execute pipeline starting at the location loaded into the PC register.
