#ifndef HEADER_UTIL
#define HEADER_UTIL

#include <stdbool.h>
#include <stdlib.h>

typedef struct mult_lst {
    struct mult_lst *next;
    size_t offset;
    size_t amount;
} mult_lst_st;

mult_lst_st *create_empty();

bool is_empty(mult_lst_st *lst);

void add(mult_lst_st **lst, size_t offset, size_t amount);

void delete(mult_lst_st **lst);

void delete_all(mult_lst_st **lst);

#endif
