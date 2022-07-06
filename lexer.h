#ifndef HEADER_LEXER
#define HEADER_LEXER
#include <stdio.h>

typedef enum {
    Increase,
    Decrease,
    OpenBracket,
    CloseBracket,
    ShiftRight,
    ShiftLeft,
    Ampersand,
    Exclamation,
    None
} token_et;

int tokenize(token_et **ptr_arg, FILE *file, size_t *amount);

#endif
