CCFLAGS=-Wall -ggdb

all:
	gcc -o pf $(CCFLAGS) main.c
