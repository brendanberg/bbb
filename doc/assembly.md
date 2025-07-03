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
