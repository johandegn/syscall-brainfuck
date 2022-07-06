#ifndef HEADER_PARSER
#define HEADER_PARSER

#include <stdlib.h>
#include "lexer.h"
#include "util.h"

typedef enum {
    ChangeVal,
    ChangePtr,
    BeginLoop,
    EndLoop,
    MemAddr,
    SysCall,
    Set,
    CondSetZero,
    Multiply // TODO - implement runtime optimization
} ins_type_et;

typedef struct {
    ins_type_et type;
    union {
        size_t arg;
        mult_lst_st *ptr;
    };
} instruction_st;

int parse(instruction_st **ptr_arg, token_et *tokens, size_t tamount, size_t *iamount);

#endif
