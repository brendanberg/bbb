# bbb Assembly

The _bbb_ Assembly Language is a programmer-friendly way to write programs that target the _bbb_ CPU architecture. Structurally, its statements mimic the corresponding machine code quite closely while providing ergonomic features for structuring control flow and memory layout.

The _bbb_ Assembler converts a source file or set of files into a binary executable that can be loaded into and executed by a _bbb_ system.

## Comments

```
(comments are program annotations for human readers of the code)
```

In bbb assembly, characters between open and close parentheses are ignored.

Comments may not be nested and may not contain newline characters.

## Assembler directives

Assembler directives are used to control the structure of the generated image.

### `#data`

```
#data 0000 0020 1000 00A0
```

The `#data` directive is used to specify literal data to be included in a specific location in an image. The directive is followed by one or more hex digits, optionally separated by spaces for grouping, and terminates at the end of the line.

### `#org`

```
#org 00A0
```

The `#org` directive specifies the starting address for the next instruction or data. The directive is followed by four hex digits indicating the address.

A program may have more than one `#org` directive, and it is the responsibility of the programmer to ensure they don't overlap.

### `#inc`

```
#inc filename.bbb
```

The `#inc` directive includes a _bbb_ assembly file at the location of the directive. The contents of the file are parsed and assembled as if the text of the file is expanded in place.

## Labels

```
LOOP:
```

Labels are names that can be attached to specific locations in memory. They consist of a letter followed by one or more alphanumeric characters, underscores, or hyphens. Label names must be immediately followed by a colon character.

Labels may be referenced in `JMP` or `JSR` instructions where the memory address is expected by prefixing the label name with a period.

## Instructions

```
MOV %a @F000
```

Instructions are machine opcodes, usually followed one or two operands. (The `NOP` opcode is an exception that takes no operands.) A list of opcodes and a discussion of what they do can be found in [doc/opcodes][opcodes]

[opcodes]: ./opcodes.md

## Operands

```
JMP NZ .END
```

For most opcodes, the operands specify the source and destination values of the instruction. Registers, constants, and memory locations may be specified as source values, while registers and memory locations may be used as destinations. (It is illegal to assign to a constant.)

Jump instructions (`JMP` and `JSR`) take a condition expression and either a memory address or a reference to a label.

**Registers**: Registers are specified by a percent sign followed by the register abbreviation in lower case. For examaple, the general-purpose `A` register would be specified as `%a`, the stack pointer would be `%sp`, and the lower half of the status register would be `%s0`.

**Constants**: Numeric constants may be written as either decimal or hex values: `0xB`, `11`.

**Memory-direct Addresses**: Memory may be addressed directly by specifying an address in hex prefixed with an at symbol: `@A0F1`.

**Memory-indexed Addresses**: Memory offsets from a base address stored in the index register can be specified by indicating an asterisk followed by the offset: `*00F1`.

**References**: References to labels are written with the label name following a period: `.LOOP`

**Condition Expressions**: In `JMP` and `JSR` instructions, the branch condition can be expressed by indicating the letter of the status flag to test against (`N`, `Z`, `C`, `O`, `I`, `H`, `F`, `T`), prefixed with an `N` if the status should be negated. For example, to indicate a branch should be followed if the carry bit is _not_ set, the condition would be written `NC`.

A handful of example programs can be found in the [examples][examples] directory.

[examples]: ../examples/
