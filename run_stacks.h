#ifndef STACK_POOL_H_
#define STACK_POOL_H_

#include <stddef.h>
#include <stdio.h>
#include "coroutine.h"
#include "list.h"

//typedef struct _coroutine coroutine;

typedef enum {
    STACK_POOL_ON_ARRAY = 0,
    STACK_POOL_ON_LIST
} pool_mode_t;

struct _stack_st{
    char* buffer;
    size_t size;
    char* stack_bp;
    coroutine* occupy;
    struct _stack_st* next;
    list_t link;
    char shared;
};

typedef struct _stack_st      stack_st;
typedef struct _stack_pool_st stack_pool_st;

stack_pool_st* create_stack_pool(size_t count, size_t stack_size);
stack_st* alloc_isolate_stack(stack_pool_st* pool, size_t stack_size);
stack_st* alloc_shared_stack(stack_pool_st* pool);
void free_stack_pool(stack_pool_st* pool);

#endif
