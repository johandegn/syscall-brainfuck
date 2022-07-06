#include <stdio.h>
#include "elf64.h"
#include "parser.h"
#include "util.h"

void build(FILE *file, size_t initial_mem, instruction_st *ins_ptr, size_t iamount) {
    instruction_st *ins_end = ins_ptr + iamount;

    fprintf(file, "\tglobal _start\n");
    fprintf(file, "\tsection .text\n");
    fprintf(file, "\n_start:\n");
    fprintf(file, "\tsub rsp, 8\n"); // FIXME - missing stack allignment?? should this be 16??
    fprintf(file, "\tmov rbx, rsp\n");
    fprintf(file, "\tmov r10, rsp\n");
    fprintf(file, "\tlea rsp, [rsp-%lu]\n", initial_mem * 8);
    fprintf(file, "\tmov qword rsi, 0\n\n");

    fprintf(file, "BEGIN:\n");
    fprintf(file, "\tcmp rsi, %lu\n", initial_mem);
    fprintf(file, "\tje END\n");
    fprintf(file, "\tinc rsi\n");
    fprintf(file, "\tsub r10, 8\n");
    fprintf(file, "\tmov qword [r10], 0\n");
    fprintf(file, "\tjmp BEGIN\n");
    fprintf(file, "END:\n\n");

    size_t csz_counter = 0;

    while (ins_ptr < ins_end) {
        switch (ins_ptr->type) {
        case ChangeVal:
            fprintf(file, "\tadd qword [rbx], %ld\n", ins_ptr->arg);
            break;
        case ChangePtr:
            fprintf(file, "\tsub rbx, %ld\n", ins_ptr->arg * 8);
            break;
        case BeginLoop:
            fprintf(file, "B%lu:\n", ins_ptr->arg);
            fprintf(file, "\tcmp qword [rbx], 0\n");
            fprintf(file, "\tje E%lu\n", ins_ptr->arg);
            break;
        case EndLoop:
            fprintf(file, "\tjmp B%lu\n", ins_ptr->arg);
            fprintf(file, "E%lu:\n", ins_ptr->arg);
            break;
        case MemAddr:
            fprintf(file, "\tmov rdi, [rbx]\n");
            fprintf(file, "\tneg rdi\n");
            fprintf(file, "\tlea rdi, [rbx+8*rdi]\n");
            fprintf(file, "\tmov [rbx], rdi\n");
            break;
        case SysCall:
            fprintf(file, "\tmov rax, [rbx]\n");
            fprintf(file, "\tmov rdi, [rbx-8]\n");
            fprintf(file, "\tmov rsi, [rbx-16]\n");
            fprintf(file, "\tmov rdx, [rbx-24]\n");
            fprintf(file, "\tmov r10, [rbx-32]\n");
            fprintf(file, "\tmov r8, [rbx-40]\n");
            fprintf(file, "\tmov r9, [rbx-48]\n");
            fprintf(file, "\tsyscall\n");
            fprintf(file, "\tmov [rbx+8], rax\n");
            break;
        case Set:
            fprintf(file, "\tmov qword [rbx], %lu\n", ins_ptr->arg);
            break;
        case CondSetZero:
            size_t ctz = __builtin_ctz(ins_ptr->arg); // Argument is not zero (would have been filtered out in parser)
            fprintf(file, "\tcmp qword [rbx], 0\n");
            fprintf(file, "\tje SZ%lu ; if zero, skip\n", csz_counter);

            fprintf(file, "\tbsf rdi, [rbx]\n");
            fprintf(file, "\tcmp qword rdi, %lu\n", ctz);
            fprintf(file, "\tjge SZ%lu \n", csz_counter);

            fprintf(file, "CSZ_LOOP%lu:\n", csz_counter);
            fprintf(file, "\tadd qword [rbx], %ld\n", ins_ptr->arg);
            fprintf(file, "\tjmp CSZ_LOOP%lu\n", csz_counter);

            fprintf(file, "SZ%lu:\n", csz_counter++);
            fprintf(file, "\tmov qword [rbx], 0\n");
            break;
        case Multiply: // FIXME - signed vs unsigned multiplication..?
            while (!is_empty(ins_ptr->ptr)) {
                fprintf(file, "\tmov rax, %lu\n", ins_ptr->ptr->amount);
                fprintf(file, "\timul rax, [rbx]\n");
                fprintf(file, "\tlea rdi, [rbx+%lu]\n", -ins_ptr->ptr->offset * 8);
                fprintf(file, "\tadd [rdi], rax\n");

                delete(&(ins_ptr->ptr));
            }
            
            fprintf(file, "\tmov qword [rbx], 0\n");
            break;
        default:
            printf("Internal error!");
            exit(5);
            break;
        }
        ++ins_ptr;
    }

    fprintf(file, "\n\tmov rax, 60\n");
    fprintf(file, "\tmov rdi, [rbx]\n");
    fprintf(file, "\tsyscall\n");
}
