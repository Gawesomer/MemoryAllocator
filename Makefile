CC	 = gcc
CFLAGS	 = -g
CPPFLAGS = -Wall -pedantic

ARFLAGS  = rvU

.PHONEY: all clean

all: main

main: main.o alloc.o

clean: 
	rm -f *.o *.a main
