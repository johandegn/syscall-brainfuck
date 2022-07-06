CC = gcc
FLAGS = -O3


all: main compact

main: main.c lexer.c lexer.h parser.c parser.h elf64.c elf64.h optimize.c optimize.h util.c util.h
	$(CC) $(FLAGS) -o main main.c lexer.c parser.c elf64.c optimize.c util.c

compact: compact.c
	$(CC) $(FLAGS) -o compact compact.c
