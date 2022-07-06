// TODO - parse arguments with argp by gnu: https://www.gnu.org/software/libc/manual/html_node/Argp.html
#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lexer.h"
#include "parser.h"
#include "elf64.h"
#include "optimize.h"

#define INITIAL_MEM 30000

// ---- argp ----
const char *argp_program_version = "System Call Brainfuck Compiler 0.1";
const char *argp_program_bug_address = 0;
static char doc[] = "System Call Brainfuck compiler";
static char args_doc[] = "INFILE";
static struct argp_option options[] = {
  {"optimize", 'O', 0, 0, "Optimize program" },
  {"output", 'o', "OUTFILE", 0, "Output to FILE instead of out.s" },
  { 0 }
};

//static struct argp argp = { options, parse_opt, args_doc, doc };
static struct argp argp = { options, 0, args_doc, doc };


int call(char *path, char *args[]) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        execvp(path, args);

        // If we get here, execvp has failed.
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        return 1;
    }

    // Parent process
    do {
        wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return WEXITSTATUS(status);
}

int main(int argc, char **argv) {
    //argp_parse (&argp, argc, argv, 0, 0, &arguments);
    //argp_parse (&argp, argc, argv, 0, 0, 0);

    if (argc != 2) {
        printf("Specify a file path as a single argument!\n");
        exit(1);
    }


    FILE *in_file = fopen(argv[1], "r");

    if (in_file == NULL) {
        printf("File not found!\n");
        exit(2);
    }

    size_t token_amount;
    token_et *tokens;
    if (tokenize(&tokens, in_file, &token_amount) != 0) {
        exit(1); // TODO - write message
    }

    if (fclose(in_file) == -1) {
        printf("Could not close source file!\n");
        exit(2);
    }

    size_t iamount;
    instruction_st *instructions;
    int code = parse(&instructions, tokens, token_amount, &iamount);
    if (code != 0) {
        if (code == 1)
            printf("Mismatched Parentheses!\n");
        if (code == 2)
            printf("Memory error!\n");
        exit(1); // TODO - write messages
    }

    free(tokens);

    optimize(&instructions, &iamount); // TODO - return code

    FILE *out_file = fopen("out.s", "w");
    if (out_file == NULL) {
        printf("File error!\n");
        exit(2); // TODO - make exit free mem.
    }

    build(out_file, INITIAL_MEM, instructions, iamount);

    if (fclose(out_file) == -1) {
        printf("Could not close destination file!\n");
        exit(2);
    }

    free(instructions);
    
    char *assemble_args[] = {"/bin/nasm", "-f", "elf64", "-o", "out.o", "out.s", NULL};
    if (call(assemble_args[0], assemble_args) != 0) {
        printf("Assembler error!\n");
        exit(1);
    }

    char *link_args[] = {"/bin/ld", "out.o", NULL};
    if (call(link_args[0], link_args) != 0) {
        printf("Linker error!\n");
        exit(1);
    }

    return 0;
}
