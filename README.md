# Lexical Analyzer and Syntax Analyzer

This is a simple C project that performs lexical analysis and basic syntax
analysis for C source files. It is intentionally kept small and easy to
understand.

## Features

- Detects keywords, identifiers, numbers, strings, characters, comments,
  preprocessor lines, operators, and punctuators.
- Checks basic syntax such as matching brackets, missing semicolons in common
  statements, and correct parentheses after `if`, `for`, `while`, and `switch`.
- Prints a colored token table and simple syntax error messages.
- Keeps every `.c` file at 100 lines or less.

## Build

```sh
make
```

## Run

```sh
./lexical_analyzer examples/sample.c
```

## Files

```text
include/   one header file
src/       small source files
examples/  sample input program
```

Use `make linecheck` to verify the 100-line limit for `.c` files.
