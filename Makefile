CC=gcc
CFLAGS=-Wall -Wextra -Iinclude
LIBS=-lSDL2 -lSDL2_ttf -lm
SRC=src/synthesizer.c
BIN=build/synthesizer

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LIBS)

clean:
	rm -rf build/
