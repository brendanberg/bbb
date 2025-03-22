# bbb Assembly

## Comments

In bbb assembly, characters between open and close parentheses are ignored.

## Assembler directives

Assembler directives are used to ....

### #data

```
#data 0000 0020 1000 00A0
```

The `#data` directive is used to specify literal data to be included in a specific location in an image.

(comments are any characters between open and close parentheses)

#data 0000

(define literal data to write at the current memory offset)
(the #data directive is followed by one or more hex digits,)
(optionally separated by spaces for grouping, and terminates)
(with a new line.)
