CC = gcc
FLAGS = -O3


all: main compact

main: main.c
	$(CC) $(FLAGS) -o main main.c

compact: compact.c
	$(CC) $(FLAGS) -o compact compact.c
