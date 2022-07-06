#ifndef HEADER_ELF64
#define HEADER_ELF64

#include <stdio.h>
#include "parser.h"

void build(FILE *file, size_t initial_mem, instruction_st *ins_ptr, size_t iamount);

#endif
