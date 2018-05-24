#include <stdlib.h>
#include "array.h"
#include "list.h"
#include "run_stacks.h"

typedef struct {
    stack_st* beg;
    stack_st* end;
    stack_st* cur;
    size_t size;
} stack_list_st;

typedef struct  {
    stack_st** sh_stacks;
    size_t sh_size;
    size_t alloc_id;
} stack_array_st;

struct _stack_pool_st {
    array_t sh_array;
    list_t is_list;
    int alloc_id;
};

#define list_to_stack(ls) list_to_struct(ls, stack_st, link)

stack_st* create_stack(size_t size) {
    stack_st* stk = (stack_st*) malloc(sizeof(stack_st));
    stk->buffer = (char*) malloc(size);
    stk->size = size;
    stk->stack_bp = stk->buffer + stk->size - 1;
    stk->occupy = NULL;
    stk->next = NULL;
    stk->shared = 0;
    list_init(&(stk->link));
    return stk;
}

void free_stack(stack_st* stk) {
    if(!stk) return;
    free(stk->buffer);
    free(stk);
}

int init_stack_array(array_t *array, size_t count, size_t st_size) {
    if(!count || !st_size) return -1;
    int i = 0;
    for(; i < count; i++)
        array_put(array, create_stack(st_size));
    return 0;
}

void free_stack_array(array_t* array) {
    if(!array) return;
    int i = array->size - 1;
    void *stack = NULL;
    for(; i >= 0; i--) {
      stack = array_del(array, i);
      if(stack) free(stack);
    }
}

void free_stack_list(list_t* list) {
    if(!list) return;
    list_t *ls = list_next(list);
    list_t *next = NULL;
    stack_st *stk = NULL;
    while(!list_empty(ls)) {
        next = list_next(ls);
        list_delete(ls); 
        free(list_to_stack(ls));
        ls = next;
    }
}

stack_pool_st* create_stack_pool(size_t count, size_t stack_size) {
    stack_pool_st* pool = (stack_pool_st*) malloc(sizeof(stack_pool_st));
    pool->alloc_id = 0;
    list_init(&(pool->is_list));
    int ret = init_stack_array(&(pool->sh_array), count, stack_size);
    if(ret) {
        free_stack_pool(pool);
        pool = NULL;
    }
    return pool;
}

stack_st* alloc_shared_stack(stack_pool_st* pool) {
    if(!pool || !array_size(&(pool->sh_array))) return NULL;
    return (stack_st *)(array_get(&(pool->sh_array), (pool->alloc_id)++ % array_size(&(pool->sh_array))));
}

stack_st* alloc_isolate_stack(stack_pool_st* pool, size_t stack_size) {
    if(!pool) return NULL;
    
    stack_st *stk = NULL;
    list_t *ls = list_next(&(pool->is_list));
    //find no occupied & enough stack
    while(ls != &(pool->is_list)) {
        stk = (stack_st *)list_to_stack(ls);
        if(stk->size >= stack_size && stk->shared) break;
        ls = list_next(ls);
    }

    //not found
    if(!stk) {
        stk = create_stack(stack_size);
        list_add_before(&(pool->is_list), &(stk->link));
    }

    // monopolize this stack
    stk->shared = 0;
    return stk;
}

void free_stack_pool(stack_pool_st* pool) {
    if(!pool) return;
    free_stack_array(&(pool->sh_array));
    free_stack_list(&(pool->is_list));
    free(pool);
}
