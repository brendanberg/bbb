# bbb Opcodes

The _bbb_ architecture has 16 opcodes, described in the following table.

| Hex | Mnemonic | Description                                                         |
| --- | -------- | ------------------------------------------------------------------- |
| 0x0 | NOP      | Do nothing                                                          |
| 0x1 | INC      | Increment the contents of register or memory                        |
| 0x2 | DEC      | Decrement the contents of register or memory                        |
| 0x3 | RLC      | Rotate contents of <dst> left through the carry status bit          |
| 0x4 | RRC      | Rotate contents of <dst> right through the carry status bit         |
| 0x5 | PSH      | Push contents of <src> onto the stack                               |
| 0x6 | POP      | Pop top of stack and store in <dst>                                 |
| 0x7 | MOV      | Place the contents of <src> in <dst>                                |
| 0x8 | ADD      | Add contents of <src> and <dst> (with carry) and store in <dst>     |
| 0x9 | SUB      | Subtract contents of <src> from <dst> (with borrow); store in <dst> |
| 0xA | AND      | Logical AND contents of <src> and <dst> and store in <dst>          |
| 0xB | OR       | Logical OR contents of <src> and <dst> and store in <dst>           |
| 0xC | XOR      | Logical XOR contents of <src> and <dst> and store in <dst>          |
| 0xD | CMP      | Compare contents of <src> and <dst> and update C and Z status bits  |
| 0xE | JMP      | Jump to location specified by <dst>                                 |
| 0xF | JSR      | Jump to subroutine specified by <dst>                               |

## Opcodes and Operands

All opcodes are 4 bits. Opcodes are followed by different numbers of operands depending on the type of instruction. _Source_ and _destination_ operands are either 4 or 12 bits.

A `NOP` instruction takes no operands.

The `INC`, `DEC`, `RLC`, `RRC`, `PSH`, and `POP` instructions expect to be followed by a register or memory location operand.

The `MOV` instruction expects to be followed by source and destination operands. The destination may be either a register name or memory location. The source may be a register name, memory location, or a constant.

The arithmetic and logic instructions (`ADD`, `SUB`, `AND`, `OR`, `XOR`, and `CMP`) similarly expect to be followed by source and destination operands, with the same semantics as the `MOV` instruction.

The `JMP` and `JSR` instructions take a status pattern followed by a destination memory location.

## Opcode Definitions

### `NOP`

Do nothing for once clock cycle.

- **Operands:** None.
- **Flags:** No change.

### `INC <dst>`

Increment the value in the specified register or memory location.

- **Operands:** `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Updates overflow, zero, and negative flags as appropriate

### `DEC`

Decrement the value in the specified register or memory location.

- **Operands:** `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Updates overflow, zero, and negative flags as appropriate

### `RLC`

Rotate the bits of the value in the specified register or memory location left through the carry status bit.

- **Operands:** `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Sets the carry flag to `<dst>`'s most significant bit. Updates overflow, zero, and negative flags as appropriate

### `RRC`

Rotate the bits of the value in the specified register or memory location right through the carry status bit.

- **Operands:** `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Sets the carry flag to `<dst>`'s least significant bit. Updates overflow, zero, and negative flags as appropriate

### `PSH`

### `POP`

### `MOV`

### `ADD`

### `SUB`

### `AND`

### `OR`

### `XOR`

### `CMP`

### `JMP`

### `JSR`
