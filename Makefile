CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Iinclude
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := lexical_analyser
GUI_BIN := lexical_gui

.PHONY: all clean run gui

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

run: $(BIN)
	./$(BIN) examples/sample.c

gui: $(BIN) gui.c
	$(CC) $(CFLAGS) -o $(GUI_BIN) gui.c
	./$(GUI_BIN)

clean:
	rm -f $(BIN) $(GUI_BIN) lexical_analyzer src/*.o *_run.txt
