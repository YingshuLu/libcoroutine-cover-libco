#ifndef LIB_STACK_H_
#define LIB_STACK_H_

#include "array.h"

typedef array_t stack_t;
#define stack_init(s) array_init(s)
#define stack_push(s, e) array_insert(s, array_size(s), elm)
#define stack_pop(s) array_del(s, array_size(s) - 1)
#define stack_empty(s) (array_size(s) == 0)

#endif
