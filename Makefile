CC = gcc
FLAGS = -O3


main: main.c
	$(CC) $(FLAGS) -o main main.c

compact: compact.c
	$(CC) $(FLAGS) -o compact compact.c

all: main compact
