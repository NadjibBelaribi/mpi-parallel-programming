CC=mpicc
# CFLAGS=-Wall -O3 -march=native -g -fopenmp
CFLAGS=-Wall -O1 -g -fopenmp

LDFLAGS=$(CFLAGS)

OBJ=$(patsubst %.c,%.o,$(wildcard *.c))

all: main
main: $(OBJ)

clean:
	rm $(OBJ) main

test: main
	mpirun -n 4 ./main input/small.mnt

memory:
		mpiexec -np 2 valgrind  --track-origins=yes  ./main input/mini.mnt
# si un .h ou le makefile change tout recompiler :
$(OBJ): $(wildcard *.h) Makefile
