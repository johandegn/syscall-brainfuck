#include <stdbool.h>
#include <stdlib.h>
#include "util.h"

mult_lst_st *create_empty() {
    return NULL;
}

bool is_empty(mult_lst_st *lst) {
    return lst == NULL;
}

void add(mult_lst_st **lst, size_t offset, size_t amount) {
    mult_lst_st *elm = malloc(sizeof(mult_lst_st));
    elm->next = *lst;
    elm->offset = offset;
    elm->amount = amount;
    *lst = elm;
}

void delete(mult_lst_st **lst) {
    mult_lst_st *next = (*lst)->next;
    free(*lst);
    *lst = next;
}

void delete_all(mult_lst_st **lst) {
    while (!is_empty(*lst))
        delete(lst);
}
