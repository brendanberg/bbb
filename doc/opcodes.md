# bbb Opcodes

The _bbb_ architecture has 16 opcodes, described in the following table.

| Hex   | Mnemonic | Description                                                         |
| ----- | -------- | ------------------------------------------------------------------- |
| `0x0` | NOP      | Do nothing                                                          |
| `0x1` | INC      | Increment the contents of register or memory                        |
| `0x2` | DEC      | Decrement the contents of register or memory                        |
| `0x3` | RLC      | Rotate contents of <dst> left through the carry status bit          |
| `0x4` | RRC      | Rotate contents of <dst> right through the carry status bit         |
| `0x5` | PSH      | Push contents of <src> onto the stack                               |
| `0x6` | POP      | Pop top of stack and store in <dst>                                 |
| `0x7` | MOV      | Place the contents of <src> in <dst>                                |
| `0x8` | ADD      | Add contents of <src> and <dst> (with carry) and store in <dst>     |
| `0x9` | SUB      | Subtract contents of <src> from <dst> (with borrow); store in <dst> |
| `0xA` | AND      | Logical AND contents of <src> and <dst> and store in <dst>          |
| `0xB` | OR       | Logical OR contents of <src> and <dst> and store in <dst>           |
| `0xC` | XOR      | Logical XOR contents of <src> and <dst> and store in <dst>          |
| `0xD` | CMP      | Compare contents of <src> and <dst> and update C and Z status bits  |
| `0xE` | JMP      | Jump to location specified by <dst>                                 |
| `0xF` | JSR      | Jump to subroutine specified by <dst>                               |

## Opcodes and Operands

All opcodes are 4 bits. Opcodes are followed by different numbers of operands depending on the type of instruction. _Source_ and _destination_ operands are either 4 or 12 bits.

A `NOP` instruction takes no operands.

The `INC`, `DEC`, `RLC`, `RRC`, `PSH`, and `POP` instructions expect to be followed by a register or memory location operand.

The `MOV` instruction expects to be followed by source and destination operands. The destination may be either a register name or memory location. The source may be a register name, memory location, or a constant.

The arithmetic and logic instructions (`ADD`, `SUB`, `AND`, `OR`, `XOR`, and `CMP`) similarly expect to be followed by source and destination operands, with the same semantics as the `MOV` instruction.

The `JMP` and `JSR` instructions take a status pattern followed by a destination memory location.

## Status Flags

There are two status registers, S0 and S1. The `S0` register contains flags to describe results of arithmetic and logic operations. They indicate overflow, carry, zero and negative values. The `S1` register contains two constant bits to compare against (one and zero), plus halt and interrupt flags.

## Opcode Definitions

### `NOP`

Do nothing for once clock cycle.

- **Operands:** None
- **Flags:** No change

### `INC <dst>`

Increment the value in the specified register or memory location.

- **Operands:**  
  `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Updates overflow, zero, and negative flags as appropriate

### `DEC <dst>`

Decrement the value in the specified register or memory location.

- **Operands:**  
  `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Updates overflow, zero, and negative flags as appropriate

### `RLC <dst>`

Rotate the bits of the value in the specified register or memory location left through the carry status bit.

- **Operands:**  
  `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Sets the carry flag to `<dst>`'s most significant bit. Updates overflow, zero, and negative flags as appropriate

### `RRC <dst>`

Rotate the bits of the value in the specified register or memory location right through the carry status bit.

- **Operands:**  
  `<dst>` – General purpose register, program counter, stack pointer, interrupt vector, index register, temporary address, or memory location
- **Flags:** Sets the carry flag to `<dst>`'s least significant bit. Updates overflow, zero, and negative flags as appropriate

### `PSH <src>`

Place the value in the specified register or memory location into the memory location referenced in the stack pointer register.

- **Operands:**  
  `<src>` – General purpose register, program counter, constant, or memory location
- **Flags:** No change

### `POP <dst>`

Place the value in the memory location referenced by the stack pointer register into the specified destination register or memory location.

- **Operands:**  
  `<dst>` – General purpose register, program counter, or memory location
- **Flags:** No change

### `MOV <src> <dst>`

Read the value in the source register or memory location and place it in the destination register or memory location.

- **Operands:**  
  `<src>` – General purpose register, program counter, stack pointer, interrupt vector, index reigister, temporary address, constant or memory location  
  `<dst>` – General purpose register, PC, SP, IV, IX, TA, CV, MD, MX
- **Flags:** No change

### `ADD <src> <dst>`

Add contents of source and destination registers and store in the destination register.

- **Operands:**  
  `<src>` – General purpose register, CV, MD, MX  
  `<dst>` – General purpose register, MD, MX
- **Flags:**  
  `O` – Sets overflow if...  
  `C` – Sets carry flag if ...  
  `Z` – Sets zero flag if the sum is 0  
  `N` – Sets negative flag if the most significant bit is set

### `SUB <src> <dst>`

- **Operands:**  
  `<src>` – General purpose register, CV, MD, MX  
  `<dst>` – General purpose register, MD, MX
- **Flags:**  
  `O` – Sets overflow if...  
  `C` – Sets carry flag if ...  
  `Z` – Sets zero flag if the sum is 0  
  `N` – Sets negative flag if the most significant bit is set

### `AND <src> <dst>`

- **Operands:**  
  `<src>` – General purpose register, CV, MD, MX  
  `<dst>` – General purpose register, MD, MX
- **Flags:**  
  `O` – No change  
  `C` – No change  
  `Z` – Sets zero flag if the sum is 0  
  `N` – Sets negative flag if the most significant bit is set

### `OR <src> <dst>`

- **Operands:**  
  `<src>` – General purpose register, CV, MD, MX  
  `<dst>` – General purpose register, MD, MX
- **Flags:**  
  `O` – No change  
  `C` – No change  
  `Z` – Sets zero flag if the sum is 0  
  `N` – Sets negative flag if the most significant bit is set

### `XOR <src> <dst>`

- **Operands:**  
  `<src>` – General purpose register, CV, MD, MX  
  `<dst>` – General purpose register, MD, MX
- **Flags:**  
  `O` – No change  
  `C` – No change  
  `Z` – Sets zero flag if the sum is 0  
  `N` – Sets negative flag if the most significant bit is set

### `CMP <src> <dst>`

Compare the values in source and destination locations and set the zero and negative flags to indicate the relationship.

If the values are equal, the zero flag will be set. In all other cases, the zero flag will be unset. If source is greater than or equal to destination, the negative flag will be unset. If source is less than destination, the negative flag will be set.

| Z   | N   | Condition                                      |
| --- | --- | ---------------------------------------------- |
| 0   | 0   | Source is greater than or equal to destination |
| 1   | 0   | Source equals destination                      |
| 0   | 1   | Source is less than destination                |

- **Operands:**  
  `<src>` – General purpose register, CV, MD, MX  
  `<dst>` – General purpose register, MD, MX
- **Flags:**  
  `O` – No change  
  `C` – No change  
  `Z` – Sets zero flag if the sum is 0  
  `N` – Sets negative flag if the most significant bit is set

### `JMP <cond> <addr>`

Conditionally jump to the memory location indicated by the destination operand.

The jump condition is a 4-bit value that defines which flag to test and the desired flag value. The three least-significant bits indicate which status flag will be used in the condition. The most significant bit is the value to test the flag against.

For example, a `JMP` instruction with a condition operand of `1` (binary `0001`) will follow the branch if the zero flag (bit one of the status register pair) is not set. When combined with a `CMP` instruction, this allows us to construct the equivalent of a branch if not equal instruction.

This mechanism also allows the `JMP` instruction to perform an unconditional jump, since testing against the constant zero and constant one flags will always produce the same result. Therefore `JMP 15 <addr>` and `JMP 6 <addr>` are both unconditional jumps, while `JMP 14 <addr>` and `JMP 7 <addr>` are both unconditional fall throughs.

> [!NOTE]
> The **bbb** assembler provides mnemonics for jump conditions that are easier to remember. For example, insetad of having to work out that to jump if carry is set, the condition operand should be `10` (binary `1010`), the programmer may write `C=1`. Likewise, jump if not zero would be written `Z=0`.
> This also simplifies unconditional jumps and fall throughs, since it's possible to write `0=0`, `0=1`, `1=0`, and `1=1`.

| Example | Description                                                               |
| ------- | ------------------------------------------------------------------------- |
| `0 000` | Jump if the negative (N) flag (status bit 0) is not set                   |
| `1 000` | Jump if the negative (N) flag (status bit 0) is set                       |
| `0 001` | Jump if the zero (Z) flag is not set                                      |
| `1 001` | Jump if the zero (Z) flag is set                                          |
| `0 010` | Jump if the carry (C) flag is not set                                     |
| `1 010` | Jump if the carry (C) flag is set                                         |
| `0 011` | Jump if the overflow (O) flag is not set                                  |
| `1 011` | Jump if the overflow (O) flag is set                                      |
| `0 100` | Jump if the interrupt (I) flag is not set                                 |
| `1 100` | Jump if the interrupt (I) flag is set                                     |
| `0 101` | Jump if the halt (H) flag is not set                                      |
| `1 101` | Jump if the halt (H) flag is set                                          |
| `0 110` | Jump if the constant zero (0) flag is not set (unconditional jump)        |
| `1 110` | Jump if the constant zero (0) flag is set (unconditional fall through)    |
| `0 111` | Jump if the constant one (1) flag is not set (unconditional fall through) |
| `1 111` | Jump if the constant one (1) flag is set (unconditional jump)             |

- **Operands:**  
  `<cond>` – Four bit jump specifier to indicate branch type  
  `<addr>` – Sixteen bit literal address
- **Flags:** No change

### `JSR <cond> <dst>`

Conditionally enter the subroutine at the memory location indicated by the destination operand. Jumping to a subroutine involves storing register values on the stack. Since there is no return opcode, the programmer must explicitly restore register values before jumping back to the original control flow.

The jump condition is a 4-bit value that defines which flag to test and the desired flag value. The three least-significant bits indicate which status flag will be used in the condition. The most significant bit is the value to test the flag against.

- **Operands:**  
  `<cond>` – Four bit jump specifier to indicate branch type  
  `<addr>` – Sixteen bit literal address
- **Flags:**  No change
