CC?=gcc
CFLAGS=-Wall -O3 -march=native -g
LDFLAGS=$(CFLAGS)

OBJ=$(patsubst %.c,%.o,$(wildcard *.c))

all: main
main: $(OBJ)

clean:
	rm $(OBJ) main

test: main
	./main input/mini.mnt

# si un .h ou le makefile change tout recompiler :
$(OBJ): $(wildcard *.h) Makefile
