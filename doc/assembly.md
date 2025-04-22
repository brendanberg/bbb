# bbb Assembly

## Comments

In bbb assembly, characters between open and close parentheses are ignored.

## Assembler directives

Assembler directives are used to ....

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

## Comments

```
(comments are any characters between open and close parentheses)
```

Comments can be used to annotate code. They must not contain new lines.

## Labels

```
LOOP:
```

Labels are names that can be attached to specific locations in memory. They consist of a letter one or more

(define literal data to write at the current memory offset)
(the #data directive is followed by one or more hex digits,)
(optionally separated by spaces for grouping, and terminates)
(with a new line.)
