#include <stdbool.h>
#include "parser.h"
#include "optimize.h"
#include "util.h"

bool change_ptr_p1(instruction_st **head, instruction_st **tail, size_t remaining) {
    // Remove if zero
    if ((*head)->arg == 0) {
        ++*head;
        return true;
    }

    if (remaining < 1)
        return false;

    // Merge if possible
    if ((*head)->type == (*head+1)->type) {
        (*head)->arg += (*head+1)->arg;
        *(*tail)++ = **head;
        *head += 2; // one extra
        return true;
    }

    return false;
}

bool change_val_p1(instruction_st **head, instruction_st **tail, size_t remaining) {
    // Remove if zero
    if ((*head)->arg == 0) {
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
        (*head)->arg += (*head+1)->arg;
        *(*tail)++ = **head;
        *head += 2; // one extra
        return true;
    }

    return false;
}

bool loop_p1(instruction_st **head, instruction_st **tail, size_t remaining) {
    if (remaining < 2)
        return false;

    // Check for simple loop
    if ((*head+1)->type == ChangeVal && (*head+2)->type == EndLoop) {
        // Infinite loop
        if ((*head+1)->arg == 0) {
            return false;
        }

        // Set 0
        if ((*head+1)->arg == 1 || (*head+1)->arg == -1) {
            (*tail)->type = Set;
            (*tail)->arg = 0;
        }
        // Conditional set to 0
        else {
            (*tail)->type = CondSetZero;
            (*tail)->arg = (*head+1)->arg;
        }
        *head += 3;
        ++*tail;
        return true;
    }

    if (remaining < 5)
        return false;

    // Multiplication
    /*
    if ((*head+5)->type == EndLoop) { // FIXME - generalize
        if ((*head+1)->type == ChangePtr 
            && (*head+2)->type == ChangeVal 
            && (*head+3)->type == ChangePtr 
            && (*head+4)->type == ChangeVal
            && ((*head+1)->arg == - (*head+3)->arg)
            && ((*head+4)->arg == -1)) {
            (*tail)->type = Multiply;
            (*tail)->arg = (*head+2)->arg;
            (*tail)->value2 = (*head+1)->arg;
            *head += 6;
            ++*tail;
            return true;
        }
    }
    */

    return false;
}

bool set_p1(instruction_st **head, instruction_st **tail, size_t remaining) {
    if (remaining < 1)
        return false;

    // Add ChangeVal amount to the Set operation
    if ((*head+1)->type == ChangeVal) {
        (*head)->arg += (*head+1)->arg;
        *(*tail)++ = **head;
        *head += 2;
        return true;
    }

    return false;
}

// Returns true if any optimizations was done
bool phase1(instruction_st **head, instruction_st **tail, instruction_st **end) {
    bool result = false;
    bool optimized = false;
    size_t remaining;
    while (*head < *end) {
        result = result || optimized;
        optimized = false;
        remaining = *end - *head - 1;
        switch ((*head)->type) {
        case ChangeVal:
            if (optimized = change_val_p1(head, tail, remaining))
                continue;
            break;
        case ChangePtr:
            if (optimized = change_ptr_p1(head, tail, remaining))
                continue;
            break;
        case BeginLoop:
            if (optimized = loop_p1(head, tail, remaining))
                continue;
            break;
        case Set:
            if (optimized = set_p1(head, tail, remaining))
                continue;
            break;
        default:
            break;
        }
        *(*tail)++ = *(*head)++;
    }
    return result;
}

bool loop_p2(instruction_st **head, instruction_st **tail, size_t remaining) {
    if (remaining < 5)
        return false;

    // Multiplication
    mult_lst_st *lst = create_empty();
    add(&lst, 0, 0);
    size_t zero_val = 0;
    size_t offset = 0;
    instruction_st ins, *ptr = *head;
    while ((ins = *(++ptr), ins.type != EndLoop)) {
        // Unsuccessful multiplication
        if (ins.type != ChangePtr && ins.type != ChangeVal) {
            delete_all(&lst);
            return false;
        }

        if (ins.type == ChangePtr) {
            offset += ins.arg;
            lst->offset = offset;
        }
        else if (ins.type == ChangeVal) {
            if (offset == 0) {
                zero_val += ins.arg;
                continue;
            }
            lst->amount = ins.arg;
        }
        
        if (lst->offset != 0 && lst->amount != 0) { // TODO - clean up this mess
            /* FIXME - make work
            mult_lst_st *l = lst;
            bool existed = false;
            while (l != NULL) {
                if (l->offset == offset) {
                    l->amount += lst->amount;
                    lst->amount = 0;
                    lst->offset = 0;
                    existed = true;
                    break;
                }
                l = l->next;
            }
            if (!existed)
                add(&lst, 0, 0);
            */
           add(&lst, 0, 0);
        }
    }

    if (offset != 0 || zero_val != -1) {
        delete_all(&lst);
        return false;
    }

    // Successful multiplication
    *head = ptr + 1;
    (*tail)->type = Multiply;
    (*tail)->ptr = lst;
    (*tail)++;
    return true;
}

// Returns true if any optimizations was done
bool phase2(instruction_st **head, instruction_st **tail, instruction_st **end) {
    bool result = false;
    bool optimized = false;
    size_t remaining;
    
    while (*head < *end) {
        result = result || optimized;
        optimized = false;
        remaining = *end - *head - 1;
        switch ((*head)->type) {
        case ChangeVal:
            //if (optimized = optimize_change_val(head, tail, remaining))
            //    continue;
            break;
        case ChangePtr:
            //if (optimized = optimize_change_ptr(head, tail, remaining))
            //    continue;
            break;
        case BeginLoop:
            if (optimized = loop_p2(head, tail, remaining))
                continue;
            break;
        case Set:
            //if (optimized = optimize_set(head, tail, remaining))
            //    continue;
            break;
        default:
            break;
        }
        
        *(*tail)++ = *(*head)++;
    }
    return result;
}

int optimize(instruction_st **ptr_arg, size_t *iamount) {
    instruction_st *instructions, *ins_ptr_head, *ins_ptr_tail, *ins_end;
    instructions = *ptr_arg;
    bool was_optimized;
    do {
        ins_ptr_head = ins_ptr_tail = instructions;
        ins_end = instructions + *iamount;
        
        was_optimized = phase1(&ins_ptr_head, &ins_ptr_tail, &ins_end);

        *iamount = ins_ptr_tail - instructions;
    } while (was_optimized);

    do {
        ins_ptr_head = ins_ptr_tail = instructions;
        ins_end = instructions + *iamount;

        was_optimized = phase2(&ins_ptr_head, &ins_ptr_tail, &ins_end);

        *iamount = ins_ptr_tail - instructions;
    } while (was_optimized);


    if (*iamount == 0) {
        *ptr_arg = instructions;
        return 0;
    }
    
    // Resize to free extra memory
    instructions = realloc(instructions, *iamount * sizeof(instruction_st));
    if (instructions == NULL) {
        printf("Memory error!\n");
        exit(3);
    }

    *ptr_arg = instructions;
    return 0;
}
