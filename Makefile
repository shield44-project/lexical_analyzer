CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Iinclude
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := lexical_analyser

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

run: $(BIN)
	./$(BIN) examples/sample.c

clean:
	rm -f $(BIN) lexical_analyzer src/*.o *_run.txt
