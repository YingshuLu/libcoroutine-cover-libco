#ifndef LIB_DEQUE_H_
#define LIB_DEQUE_H_

#include "list.h"

typedef list_t deque_t;

#define deque_init(d) list_init(d)
#define deque_destory(d) do { deque_pop_back(d); }while(!deque_empty(d))
#define deque_empty(d) list_empty(d)

void deque_push_front(deque_t* d, void* elm); 
void deque_push_back(deque_t* d, void* elm);
void* deque_pop_front(deque_t *d); 
void* deque_pop_back(deque_t *d);

#endif
