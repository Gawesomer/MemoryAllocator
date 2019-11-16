CC	 = gcc
CFLAGS	 = -g
CPPFLAGS = -Wall -pedantic

ARFLAGS  = rvU

.PHONEY: all clean

all: alloc.a main

alloc.a: alloc.a(alloc.o)

main: main.o alloc.a

clean: 
	rm -f *.o *.a main
