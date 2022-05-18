// TODO - parse arguments with argp by gnu: https://www.gnu.org/software/libc/manual/html_node/Argp.html
#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

token_et * tokenize(FILE *file, size_t *amount) {
    size_t size = 100;
    size_t used = 0;
    token_et *ptr = (token_et *) malloc(size * sizeof(token_et));
    if (ptr == NULL) {
        printf("Memory error!\n");
        exit(3);
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
    if (used == 0)
        return ptr;

    // Resize to free extra memory
    ptr = realloc(ptr, used * sizeof(token_et));

    if (ptr == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

    return ptr;
}

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
    size_t value;
    size_t value2; // FIXME - do something better instead
} instruction_st;

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

void join_loop_ends(instruction_st *instructions, size_t iamount, size_t max_nesting){
    // Create bracket stack
    size_t *bracket_stack = (size_t *) malloc(max_nesting * sizeof(size_t));
    size_t *stack_ptr = bracket_stack;
    if (bracket_stack == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

    size_t label_counter = 0;
    // Match loop begin with end
    instruction_st *ins_ptr = instructions;
    while (ins_ptr < instructions + iamount) {
        if (ins_ptr->type == BeginLoop) {
            *stack_ptr = label_counter;
            ins_ptr->value = label_counter;
            ++label_counter;
            ++stack_ptr;
        }
        else if (ins_ptr->type == EndLoop) {
            --stack_ptr;
            ins_ptr->value = *stack_ptr;
        }
        ++ins_ptr;
    }

    free(bracket_stack);
}

instruction_st * parse(token_et *tokens, size_t tamount, size_t *iamount) {
    size_t size = 100;
    size_t used = 0;
    
    instruction_st *instructions = (instruction_st *) malloc(size * sizeof(instruction_st));
    instruction_st *ins_ptr = instructions;
    if (instructions == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

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
            ins_ptr->value = count.inc - count.dec;
            break;
        case ShiftRight:
        case ShiftLeft:
            count = count_occurences(token_ptr, token_end, ShiftRight, ShiftLeft);
            token_ptr += count.inc + count.dec - 1;
            ins_ptr->type = ChangePtr;
            ins_ptr->value = count.inc - count.dec;
            break;
        case OpenBracket:
            ins_ptr->type = BeginLoop;
            break;
        case CloseBracket:
            ins_ptr->type = EndLoop;
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

    // Bail out fast
    if (used == 0)
        return instructions;

    // Resize to free extra memory
    instructions = realloc(instructions, used * sizeof(instruction_st));
    if (instructions == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

    return instructions;
}

size_t brackets_match(token_et *ptr, size_t amount) {
    size_t balance = 0;
    size_t max = 0;
    for (size_t i = 0; i < amount; i++) {
        if (ptr[i] == OpenBracket)
            max = max > ++balance ? max : balance;
        else if (ptr[i] == CloseBracket)
            if (--balance < 0)
                return -1;
    }
    if (balance > 0)
        return -1;
    return max;
}

bool optimize_change_ptr(instruction_st **head, instruction_st **tail, size_t remaining) {
    // Remove if zero
    if ((*head)->value == 0) {
        ++*head;
        return true;
    }

    if (remaining < 1)
        return false;

    // Merge if possible
    if ((*head)->type == (*head+1)->type) {
        (*head)->value += (*head+1)->value;
        *(*tail)++ = **head;
        *head += 2; // one extra
        return true;
    }

    return false;
}

bool optimize_change_val(instruction_st **head, instruction_st **tail, size_t remaining) {
    // Remove if zero
    if ((*head)->value == 0) {
        ++*head;
        return true;
    }

    if (remaining < 1)
        return false;
    
    // Remove if overriden 
    if ((*head+1)->type == Set) {
        ++*head;
        return true;
    }

    // Merge if possible
    if ((*head)->type == (*head+1)->type) {
        (*head)->value += (*head+1)->value;
        *(*tail)++ = **head;
        *head += 2; // one extra
        return true;
    }

    return false;
}

bool optimize_loop(instruction_st **head, instruction_st **tail, size_t remaining) {
    if (remaining < 2)
        return false;
    
    // Check for simple loop
    if ((*head+1)->type == ChangeVal && (*head+2)->type == EndLoop) {
        // Infinite loop
        if ((*head+1)->value == 0) {
            return false;
        }

        // Set 0
        if ((*head+1)->value == 1 || (*head+1)->value == -1) {
            (*tail)->type = Set;
            (*tail)->value = 0;
        }
        // Conditional set to 0
        else {
            (*tail)->type = CondSetZero;
            (*tail)->value = (*head+1)->value;
        }
        *head += 3;
        ++*tail;
        return true;
    }

    if (remaining < 5)
        return false;

    // Multiplication
    if ((*head+5)->type == EndLoop) { // FIXME - generalize
        if ((*head+1)->type == ChangePtr 
            && (*head+2)->type == ChangeVal 
            && (*head+3)->type == ChangePtr 
            && (*head+4)->type == ChangeVal
            && ((*head+1)->value == - (*head+3)->value)
            && ((*head+4)->value == -1)) {
            (*tail)->type = Multiply;
            (*tail)->value = (*head+2)->value;
            (*tail)->value2 = (*head+1)->value;
            *head += 6;
            ++*tail;
            return true;
        }
    }

    return false;
}

bool optimize_set(instruction_st **head, instruction_st **tail, size_t remaining) {
    if (remaining < 1)
        return false;

    if ((*head+1)->type == ChangeVal) {
        (*head)->value += (*head+1)->value;
        *(*tail)++ = **head;
        *head += 2;
        return true;
    }

    return false;
}

// Returns true if any optimizations was done
bool optimize_runthrough(instruction_st **head, instruction_st **tail, instruction_st **end) {
    bool result = false;
    bool optimized = false;
    size_t remaining;
    while (*head < *end) {
        result = result || optimized;
        optimized = false;
        remaining = *end - *head - 1;
        switch ((*head)->type) {
        case ChangeVal:
            if (optimized = optimize_change_val(head, tail, remaining))
                continue;
            break;
        case ChangePtr:
            if (optimized = optimize_change_ptr(head, tail, remaining))
                continue;
            break;
        case BeginLoop:
            if (optimized = optimize_loop(head, tail, remaining))
                continue;
            break;
        case Set:
            if (optimized = optimize_set(head, tail, remaining))
                continue;
            break;
        default:
            break;
        }
        *(*tail)++ = *(*head)++;
    }
    return result;
}

instruction_st * optimize_instructions(instruction_st *instructions, size_t *iamount) {
    instruction_st *ins_ptr_head, *ins_ptr_tail, *ins_end;
    bool optimized;
    do {
        ins_ptr_head = ins_ptr_tail = instructions;
        ins_end = instructions + *iamount;
        
        optimized = optimize_runthrough(&ins_ptr_head, &ins_ptr_tail, &ins_end);

        *iamount = ins_ptr_tail - instructions;
    } while (optimized);

    if (*iamount == 0)
        return instructions;
    
    // Resize to free extra memory
    instructions = realloc(instructions, *iamount * sizeof(instruction_st));
    if (instructions == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

    return instructions;
}

void build(FILE *file, instruction_st *ins_ptr, size_t iamount) {
    instruction_st *ins_end = ins_ptr + iamount;

    fprintf(file, "\tglobal _start\n");
    fprintf(file, "\tsection .text\n");
    fprintf(file, "\n_start:\n");
    fprintf(file, "\tsub rsp, 8\n"); // FIXME - missing stack allignment?? should this be 16??
    fprintf(file, "\tmov rbx, rsp\n");
    fprintf(file, "\tmov r10, rsp\n");
    fprintf(file, "\tlea rsp, [rsp-%lu]\n", INITIAL_MEM * 8);
    fprintf(file, "\tmov qword rsi, 0\n\n");

    fprintf(file, "BEGIN:\n");
    fprintf(file, "\tcmp rsi, %lu\n", INITIAL_MEM);
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
            fprintf(file, "\tadd qword [rbx], %ld\n", ins_ptr->value);
            break;
        case ChangePtr:
            fprintf(file, "\tsub rbx, %ld\n", ins_ptr->value * 8);
            break;
        case BeginLoop:
            fprintf(file, "B%lu:\n", ins_ptr->value);
            fprintf(file, "\tcmp qword [rbx], 0\n");
            fprintf(file, "\tje E%lu\n", ins_ptr->value);
            break;
        case EndLoop:
            fprintf(file, "\tjmp B%lu\n", ins_ptr->value);
            fprintf(file, "E%lu:\n", ins_ptr->value);
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
            fprintf(file, "\tmov qword [rbx], %lu\n", ins_ptr->value);
            break;
        case CondSetZero:
            size_t ctz = __builtin_ctz(ins_ptr->value); // Argument is not zero (would have been filtered out in parser)
            fprintf(file, "\tcmp qword [rbx], 0\n");
            fprintf(file, "\tje SZ%lu ; if zero, skip\n", csz_counter);

            fprintf(file, "\tbsf rdi, [rbx]\n");
            fprintf(file, "\tcmp qword rdi, %lu\n", ctz);
            fprintf(file, "\tjge SZ%lu \n", csz_counter);

            fprintf(file, "CSZ_LOOP%lu:\n", csz_counter);
            fprintf(file, "\tadd qword [rbx], %ld\n", ins_ptr->value);
            fprintf(file, "\tjmp CSZ_LOOP%lu\n", csz_counter);

            fprintf(file, "SZ%lu:\n", csz_counter++);
            fprintf(file, "\tmov qword [rbx], 0\n");
            break;
        case Multiply: // FIXME - signed vs unsigned multiplication..?
            fprintf(file, "\tmov rax, %lu\n", ins_ptr->value);
            fprintf(file, "\timul rax, [rbx]\n");
            fprintf(file, "\tlea rdi, [rbx+%lu]\n", -ins_ptr->value2 * 8);
            fprintf(file, "\tadd [rdi], rax\n");

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
    tokens = tokenize(in_file, &token_amount);

    if (fclose(in_file) == -1) {
        printf("Could not close source file!\n");
        exit(2);
    }

    size_t max_nesting = brackets_match(tokens, token_amount);
    if (max_nesting == -1) {
        printf("Mismatched Parentheses");
        exit(5);
    }

    size_t iamount;
    instruction_st *instructions;
    instructions = parse(tokens, token_amount, &iamount);

    free(tokens);

    instructions = optimize_instructions(instructions, &iamount);
    join_loop_ends(instructions, iamount, max_nesting);

    FILE *out_file = fopen("out.s", "w");
    if (out_file == NULL) {
        printf("File error!\n");
        exit(2);
    }

    build(out_file, instructions, iamount);

    if (fclose(out_file) == -1) {
        printf("Could not close destination file!\n");
        exit(2);
    }

    free(instructions);

    return 0;
}
