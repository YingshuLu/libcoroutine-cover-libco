#include <stdlib.h>
#include "stack_pool.h"

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
    stack_array_st* sh_array;
    stack_list_st* sh_list;
    stack_list_st* is_list;
    pool_mode_t mode;
};

stack_st* create_stack(size_t size) {
    stack_st* stk = (stack_st*) malloc(sizeof(stack_st));
    stk->buffer = (char*) malloc(size);
    stk->size = size;
    stk->stack_bp = stk->buffer + stk->size - 1;
    stk->occupy = NULL;
    stk->next = NULL;
    stk->shared = 0;
    return stk;
}

void free_stack(stack_st* stk) {
    if(!stk) return;
    free(stk->buffer);
    free(stk);
}

stack_list_st* create_stack_list(size_t list_size, size_t stack_size) {
    stack_list_st* slist = (stack_list_st*) malloc(sizeof(stack_list_st));
    slist->size = list_size;
    slist->end = NULL;
    slist->cur = NULL;
    slist->beg = list_size? create_stack(stack_size) : NULL;

    if(!slist->beg) return slist;
    size_t lsize = list_size - 1; 
    stack_st* node = slist->beg;
    node->shared = 1;
    int i = 0;

    for(; i <= lsize; i++) {
       node->next = create_stack(stack_size); 
       node->next->shared = 1;
       if(i == lsize - 1) slist->end = node->next;
       node = node->next;
    }

    if(!slist->end) slist->end = slist->beg;
    return slist;
}

void stack_list_append(stack_list_st* list, stack_st* stk) {
    if(!list || !stk) return;
    if(!list->size) {
        list->beg = stk;
        list->end = stk;
    }
    else {
        list->end->next = stk;
        list->end = stk;
    }
    list->size++;
    printf("stack list size : %zu\n", list->size);
    return;
}

stack_st* alloc_stack_from_list(stack_list_st* list) {
    if(!list || !list->beg) return NULL;
    if(list->cur) list->cur = list->cur->next;
    if(!list->cur) list->cur = list->beg;
    return list->cur;
}

void free_stack_list(stack_list_st* list) {
    if(!list) return;
    stack_st* node = list->beg;
    stack_st* prev = NULL;

    while(!node) {
       prev = node;
       node = prev->next;
       free_stack(prev);
    }
}

stack_array_st* create_stack_array(size_t count, size_t size) {
    if(!count || !size) return NULL;
    stack_array_st* sp = (stack_array_st*) malloc(sizeof(stack_array_st));
    sp->alloc_id = 0;
    sp->sh_stacks = (stack_st**) malloc(sizeof(stack_array_st*) * count);
    sp->sh_size = count; 
    
    int i = 0;
    for(; i < sp->sh_size; i++) {
        sp->sh_stacks[i] = create_stack(size);
        sp->sh_stacks[i]->shared = 1;
    }
    return sp;
}

stack_st* alloc_stack_from_array(stack_array_st* array) {
    if(!array || !array->sh_size) return NULL;
    return array->sh_stacks[array->alloc_id++ % array->sh_size];
}

void free_stack_array(stack_array_st* sp) {
    if(!sp) return;
    int i = 0;
    for(; i < sp->sh_size; i++)
      free_stack(sp->sh_stacks[i]);
    free(sp);
}

stack_pool_st* create_stack_pool(pool_mode_t mode, size_t count, size_t stack_size) {
    stack_pool_st* pool = (stack_pool_st*) malloc(sizeof(stack_pool_st));
    
    pool->is_list = NULL;
    switch(mode) {
        case STACK_POOL_ON_LIST:
            pool->sh_list = create_stack_list(count, stack_size);
            printf("create stack list: %p\n", pool->sh_list);
            pool->sh_array = NULL;
            pool->mode = mode;
            break;

        case STACK_POOL_ON_ARRAY:
            pool->mode = mode;
        default:
            pool->sh_array = create_stack_array(count, stack_size);
            pool->sh_list = NULL;
    }

    return pool;
}

stack_st* alloc_isolate_stack(stack_pool_st* pool, size_t stack_size) {
    if(!pool) return NULL;
    if(!pool->is_list) pool->is_list = create_stack_list(0, stack_size);

    stack_st* stk = pool->is_list->beg;
    //find no occupied & enough stack
    while(stk) {
        if(stk->size >= stack_size && stk->shared) break;
        stk = stk->next;
    }

    //not found
    if(!stk) {
        stk = create_stack(stack_size);
        stack_list_append(pool->is_list, stk);
    }

    // monopolize this stack
    stk->shared = 0;
    return stk;
}

stack_st* alloc_stack(stack_pool_st* pool) {
    stack_st* stk = NULL;
    if(pool) {
        switch(pool->mode) {
            case STACK_POOL_ON_LIST:
                stk = alloc_stack_from_list(pool->sh_list);
                break;
            case STACK_POOL_ON_ARRAY:
            default:
                stk = alloc_stack_from_array(pool->sh_array);
        }
    }
    if(!stk) stk->shared = 1;
    return stk;
}

void free_stack_pool(stack_pool_st* pool) {
    if(!pool) return;

    free_stack_array(pool->sh_array);
    free_stack_list(pool->sh_list);
    free_stack_list(pool->is_list);
}
