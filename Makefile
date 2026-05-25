CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Iinclude
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := lexical_analyzer

.PHONY: all clean run linecheck

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

run: $(BIN)
	./$(BIN) examples/sample.c

linecheck:
	@awk 'FNR==1{if(n>100){print f ": " n; bad=1} f=FILENAME; n=0} \
	{n++} END{if(n>100){print f ": " n; bad=1} exit bad}' src/*.c

clean:
	rm -f $(BIN) src/*.o
