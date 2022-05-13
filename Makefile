CC = gcc
FLAGS = -O3

all: main.c
	$(CC) $(FLAGS) -o main main.c
