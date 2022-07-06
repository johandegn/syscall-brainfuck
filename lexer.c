#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

token_et get_token(char c) {
    switch (c) {
    case '+':
        return Increase;
    case '-':
        return Decrease;
    case '[':
        return OpenBracket;
    case ']':
        return CloseBracket;
    case '>':
        return ShiftRight;
    case '<':
        return ShiftLeft;
    case '&':
        return Ampersand;
    case '!':
        return Exclamation;
    default:
        return None;
    }
}

int tokenize(token_et **ptr_arg, FILE *file, size_t *amount) {
    size_t size = 100;
    size_t used = 0;
    token_et *ptr;
    ptr = (token_et *) malloc(size * sizeof(token_et));
    if (ptr == NULL) {
        printf("Memory error!\n");
        exit(3); // return error code instead
    }

    char c;
    token_et token;
    while ((c = fgetc(file)) != EOF) {
        token = get_token(c);
        if (token == None)
            continue;
        
        // If more space needed
        if (used == size) {
            size += size;
            ptr = realloc(ptr, size * sizeof(token_et));
            if (ptr == NULL) {
                printf("Memory error!\n");
                exit(3);
            }
        }

        ptr[used++] = token;
    }
    
    *amount = used;
    // Bail out fast
    if (used == 0) {
        *ptr_arg = ptr;
        return 0;
    }

    // Resize to free extra memory
    ptr = realloc(ptr, used * sizeof(token_et));

    if (ptr == NULL) {
        printf("Memory error!\n");
        exit(3);
    }
    
    *ptr_arg = ptr;
    return 0;
}
