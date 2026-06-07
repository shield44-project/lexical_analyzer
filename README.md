# Lexical Analyser and Syntax Analyser

A small C project that performs lexical analysis and basic syntax analysis for
C source files. The code is intentionally structured so it is easy to read,
explain, and rewrite for coursework or decision-making AI demonstrations.

## Features

- Detects keywords, identifiers, numbers, strings, characters, comments,
  preprocessor lines, operators, and punctuators.
- Checks basic syntax such as matching brackets, common missing semicolons, and
  parentheses after `if`, `for`, `while`, and `switch`.
- Prints an ASCII logo banner, colored token table, syntax result, and summary.
- Asks whether the same analyser output should be saved to a text file.
- Names saved reports from the input source file, such as `sample_run.txt`.
- Includes a simple C-based browser GUI for pasting or loading code and viewing
  terminal-style analyzer output, with a separate Keywords view.

## Build

```sh
make
```

## Run

```sh
./lexical_analyser examples/sample.c
```

or:

```sh
make run
```

When prompted, type `yes` to save the token and syntax report, or `no` to only
show it in the terminal.

## GUI

```sh
make gui
```

In the GUI, paste code directly into the editor or use **Load File** to open a
source file. Press **Analyze Code** to populate the terminal-style output.
Use **Terminal** for the full report and **Keywords** for keyword-only output.
The command starts a local C server and opens the interface in your browser.

## Files

```text
include/   shared analyser declarations
src/       lexer, syntax checker, display, keywords, and main program
examples/  sample input program
```
