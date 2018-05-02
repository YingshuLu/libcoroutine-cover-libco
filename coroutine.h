#ifndef COROUTINE_H_
#define COROUTINE_H_

#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>

#define DBG_LOG printf

typedef enum {
    COROUTINE_READY = 0,
    COROUTINE_RUNNING,
    COROUTINE_SUSPEND,
    COROUTINE_DEAD
} status_t;

typedef void(*cofunc_t)(void*, void*);
typedef struct _scheduler scheduler;
typedef struct _coroutine coroutine;

coroutine* create_coroutine(scheduler* shed, cofunc_t f, void* ip, void* op);
coroutine* reset_coroutine(scheduler* s, coroutine* c, cofunc_t f, void* ip, void* op);
void free_coroutine(coroutine* c);

status_t coroutine_status(coroutine* c);
void coroutine_resume(coroutine* c);
void coroutine_yield();

scheduler* current_scheduler();
scheduler* create_scheduler(size_t cocap, size_t stack_count, size_t stack_size);
void free_scheduler(scheduler* s);

#endif
