CC=gcc
CCFLAGS=-Wall -Wextra -std=c11 -pedantic
EXE=pf

all: main.o
	$(CC) -o $(EXE) $(CCFLAGS) main.o

shell: all
	./$(EXE)

main.o: main.c
	$(CC) $(CCFLAGS) -c main.c

debug:
	$(CC) -o $(EXE) $(CCFLAGS) -ggdb main.c

clean:
	rm *.o $(EXE)
