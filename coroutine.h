#ifndef COROUTINE_H_
#define COROUTINE_H_

#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef CO_DEBUG
#define DBG_LOG printf
#else
#define DBG_LOG //
#endif

typedef enum {
    COROUTINE_READY = 0,
    COROUTINE_RUNNING,
    COROUTINE_SUSPEND,
    COROUTINE_DEAD
} status_t;

typedef void(*cofunc_t)(void*, void*);
typedef void(*cocb_t)();
typedef struct _scheduler scheduler;
typedef struct _coroutine coroutine;

coroutine* current_coroutine();
coroutine* create_coroutine(scheduler* shed, cofunc_t f, void* ip, void* op);
coroutine* reset_coroutine(scheduler* shed, coroutine* co, cofunc_t f, void* ip, void* op);
void free_coroutine(coroutine* c);

void coroutine_resume(coroutine* c);
void coroutine_yield();
int coroutine_cancel(coroutine* c);

void coroutine_enable_hook(coroutine *co);
void coroutine_disable_hook(coroutine *co); 
int coroutine_hooked(coroutine *co);

void coroutine_set_callback(coroutine* c, cocb_t cb);
int coroutine_create_specific_key();
int coroutine_set_specific(int key, void* value);
void* coroutine_get_specific(int key);

scheduler* current_scheduler();
scheduler* create_scheduler(size_t cocap, size_t stack_count, size_t stack_size);
void free_scheduler(scheduler* s);
void scheduler_after(coroutine *t1, coroutine *t2);

#endif
