#include "parser.h"
#include "lexer.h"

int join_loop_ends(instruction_st *instructions, size_t iamount, size_t max_nesting) {
    // Create bracket stack
    size_t *bracket_stack = (size_t *) malloc(max_nesting * sizeof(size_t));
    size_t *stack_ptr = bracket_stack;
    if (bracket_stack == NULL)
        return 1;

    size_t label_counter = 0;
    // Match loop begin with end
    instruction_st *ins_ptr = instructions;
    while (ins_ptr < instructions + iamount) {
        if (ins_ptr->type == BeginLoop) {
            *stack_ptr = label_counter;
            ins_ptr->arg = label_counter;
            ++label_counter;
            ++stack_ptr;
        }
        else if (ins_ptr->type == EndLoop) {
            --stack_ptr;
            ins_ptr->arg = *stack_ptr;
        }
        ++ins_ptr;
    }

    free(bracket_stack);
    return 0;
}

typedef struct {
    size_t inc;
    size_t dec;
} count_st;

count_st count_occurences(token_et *ptr, token_et *end, token_et inc, token_et dec) {
    count_st res;
    res.inc = res.dec = 0;
    while (ptr < end) {
        if (*ptr == inc)
            ++res.inc;
        else if (*ptr == dec)
            ++res.dec;
        else
            break;
        ++ptr;
    }
    return res;
}

int parse(instruction_st **ptr_arg, token_et *tokens, size_t tamount, size_t *iamount) {
    size_t size = 100;
    size_t used = 0;
    
    instruction_st *instructions = (instruction_st *) malloc(size * sizeof(instruction_st));
    instruction_st *ins_ptr = instructions;
    if (instructions == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

    size_t loop_nest_lvl = 0;
    size_t max_nest_lvl = 0;

    token_et *token_ptr = tokens;
    token_et *token_end = token_ptr + tamount;

    count_st count;
    while (token_ptr < token_end) {
        // If more space needed
        if (used == size) {
            size += size;
            instructions = realloc(instructions, size * sizeof(instruction_st));
            if (instructions == NULL) {
                printf("Memory error!\n");
                exit(3);
            }
            ins_ptr = instructions + used; // could have been moved
        }

        switch (*token_ptr) {
        case Increase:
        case Decrease:
            count = count_occurences(token_ptr, token_end, Increase, Decrease);
            token_ptr += count.inc + count.dec - 1;
            ins_ptr->type = ChangeVal;
            ins_ptr->arg = count.inc - count.dec;
            break;
        case ShiftRight:
        case ShiftLeft:
            count = count_occurences(token_ptr, token_end, ShiftRight, ShiftLeft);
            token_ptr += count.inc + count.dec - 1;
            ins_ptr->type = ChangePtr;
            ins_ptr->arg = count.inc - count.dec;
            break;
        case OpenBracket:
            ins_ptr->type = BeginLoop;
            ++loop_nest_lvl;
            max_nest_lvl = max_nest_lvl > loop_nest_lvl ? max_nest_lvl : loop_nest_lvl;
            break;
        case CloseBracket:
            ins_ptr->type = EndLoop;
            if (--loop_nest_lvl == -1) { // usinged int, so i use == -1
                return 1;
            }
            break;
        case Ampersand:
            ins_ptr->type = MemAddr;
            break;
        case Exclamation:
            ins_ptr->type = SysCall;
            break;
        default:
            printf("Bad Token. Address: %ld. Begin: %ld. Diff: %ld\n", token_ptr, tokens, token_ptr - tokens);
            exit(4);
        }

        ++token_ptr;
        ++used;
        ++ins_ptr;
    }

    *iamount = used;
    if (loop_nest_lvl != 0)
        return 1;

    if (join_loop_ends(instructions, *iamount, max_nest_lvl)) {
        return 2;
    }

    // Bail out fast
    if (used == 0) {
        *ptr_arg = instructions;
        return 0;
    }

    // Resize to free extra memory
    instructions = realloc(instructions, used * sizeof(instruction_st));
    if (instructions == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

    *ptr_arg = instructions;
    return 0;
}
