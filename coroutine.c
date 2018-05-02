#include <ucontext.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <assert.h>
#include "coroutine.h"
#include "stack_pool.h"

#ifndef SET_COROUTINE_STACK_SIZE
#define COROUTINE_STACK_SIZE  128 * 1024
#else
#define COROUTINE_STACK_SIZE SET_COROUTINE_STACK_SIZE
#endif

#ifndef SET_COROUTINE_STACK_COUNT
#define COROUTINE_STACK_COUNT 7
#else
#define COROUTINE_STACK_COUNT SET_COROUTINE_STACK_COUNT
#endif

#ifndef SHARE_STACK_MODE 
#define SCHEDULER_STACK_POOL_MODE STACK_POOL_ON_ARRAY
#else
#define SCHEDULER_STACK_POOL_MODE SHARE_STACK_MODE
#endif

__thread scheduler* g_scheduler = NULL;

//coroutine max call stack depth
#define SCHEDULER_COROUTINE_MAX_SIZE 1024

//define stack pool mode


const char* _status_strs[] = { "Ready", "Running", "Suspend", "Dead" };

#define STATUS(s) ((s < COROUTINE_READY || s > COROUTINE_DEAD)? "Invalid status" :_status_strs[s])

struct _coroutine {
    stack_st* stack;
    char* stack_sp;

    cofunc_t func;
    void* iparam;
    void* oparam;    
    status_t status;
    
    char* snapshot;
    size_t snapshot_size;
    size_t snapshot_capcity;

    ucontext_t context;
    char inner;
};

struct _scheduler {
    stack_pool_st* spool;
    coroutine** co_array;
    size_t co_capcity;
    size_t co_size;
    coroutine* co_main;
};

void save_coroutine_stack(coroutine*);
void coroutine_die();

scheduler* current_scheduler() {
    if(g_scheduler) return g_scheduler;
    return create_scheduler(SCHEDULER_COROUTINE_MAX_SIZE, COROUTINE_STACK_COUNT, COROUTINE_STACK_SIZE);
}

coroutine* current_coroutine() {
    assert(current_scheduler()->co_size > 0);
    return current_scheduler()->co_array[current_scheduler()->co_size - 1];
}

coroutine* main_coroutine() {
    return current_scheduler()->co_main;
}

coroutine* previous_coroutine() {
    coroutine* curr = current_coroutine();
    if(main_coroutine() == curr || !curr) return NULL;
    return current_scheduler()->co_array[current_scheduler()->co_size - 2];
}

//make sure coroutine has been dead before free it
//unless current coroutine is main, and trying to destory scheduler
void free_coroutine(coroutine* c) {
    if(!c) return;
    free(c->snapshot);
    free(c);
}

//can only be called on main coroutine
void free_scheduler(scheduler* s) {
    if(!s) return;
    free_coroutine(s->co_main);
    free_stack_pool(s->spool);
    free(s->co_array);
}

void push_coroutine(coroutine* c) {
    assert(g_scheduler->co_size <= g_scheduler->co_capcity);
    if(c) g_scheduler->co_array[g_scheduler->co_size++] = c;
}

coroutine* pop_coroutine() {
    if(g_scheduler->co_size > 1) return g_scheduler->co_array[--(g_scheduler->co_size)];
    return NULL;
}

void _co_inner_func(void) {    
    coroutine* c = current_coroutine();
    assert(c);

    c->status = COROUTINE_RUNNING;
    c->func(c->iparam, c->oparam);
    
    //DBG_LOG("coroutine: %p go die\n", c);
    coroutine_die();
    return;
}

coroutine* create_coroutine(scheduler* shed, cofunc_t f, void* ip, void* op) {
    coroutine* c = (coroutine*) malloc(sizeof(coroutine));
    return reset_coroutine(shed, c, f, ip, op);
}

coroutine* reset_coroutine(scheduler* shed, coroutine* c, cofunc_t f, void* ip, void* op) {
    if(!c) return NULL;
    c->func = f;
    c->iparam = ip;
    c->oparam = op;

    c->status = COROUTINE_READY;
    c->snapshot = NULL;
    c->snapshot_size = 0;
    c->snapshot_capcity = 0;
    
    c->stack = NULL;
    if(shed) c->stack = alloc_stack(shed->spool);
    DBG_LOG("coroutine: %p, stack: %p\n", c, c->stack);
    c->stack_sp = NULL;

    c->inner = 0;
    return c;
}

scheduler* create_scheduler(size_t cocap, size_t stack_count, size_t stack_size) {
    if(g_scheduler) return g_scheduler;

    g_scheduler = (scheduler*) malloc(sizeof(scheduler));
    bzero(g_scheduler, sizeof(g_scheduler));
    
    g_scheduler->co_array = (coroutine**) malloc(sizeof(coroutine*) * cocap);
    g_scheduler->co_capcity = cocap;
    g_scheduler->co_size = 0;

    g_scheduler->spool = create_stack_pool(SCHEDULER_STACK_POOL_MODE, stack_count, stack_size);

    //main coroutine use ioslate stack memory
    coroutine* co_main = create_coroutine(NULL, NULL, NULL, NULL);
    co_main->stack = alloc_isolate_stack(g_scheduler->spool, 1);

    g_scheduler->co_main = co_main;
    push_coroutine(co_main);
    return g_scheduler;
}

void save_coroutine_stack(coroutine* c) {
    assert(c);
    if(c->snapshot_capcity < (c->stack->stack_bp - c->stack_sp) || !c->snapshot) {
        free(c->snapshot);
        //expand 2 times capcity
        c->snapshot_capcity = 2*(c->stack->stack_bp - c->stack_sp);
        c->snapshot = (char*) malloc(c->snapshot_capcity);
    }
    c->snapshot_size = c->stack->stack_bp - c->stack_sp;
    if(c->snapshot_size) memcpy(c->snapshot, c->stack_sp, c->snapshot_size);
    return;
}

void recover_coroutine_stack(coroutine* c) {
    assert(c);
    if(c->snapshot > 0) memcpy(c->stack->stack_bp - c->snapshot_size, c->snapshot, c->snapshot_size);            
}

//alloc a different stack for ready coroutine
stack_st* realloc_coroutine_stack(coroutine* c) {
    assert(c && c->status == COROUTINE_READY);

    scheduler* shed = current_scheduler();
    //re-alloc a different stack
    stack_st* stk, *ostk;
    int cnt = 3;
    do {
        stk = alloc_stack(shed->spool);
        DBG_LOG("find next stack_st in pool\n");
    } while(stk == c->stack && --cnt);
    

    if(stk == c->stack) {
        stk = alloc_isolate_stack(shed->spool, c->stack->size);
        DBG_LOG("create new stack_st\n");
    }

    ostk = c->stack;
    if(!ostk->shared) ostk->shared = 1;
    c->stack = stk;
    return stk;
}

void _proxy_func_(void* c, void* p) {
    coroutine* curr = c? (coroutine*) c : NULL;
    coroutine* pendding = p? (coroutine*) p : NULL;
    if(pendding) coroutine_resume(pendding);
}

void coroutine_proxy(coroutine* curr, coroutine* pendding) {
    assert(pendding);
    coroutine* proxy = create_coroutine(NULL, _proxy_func_, (void*) curr, (void*) pendding);
    proxy->stack = alloc_isolate_stack(current_scheduler()->spool, 256);
    proxy->inner = 1;
    if(proxy->stack == pendding->stack) { realloc_coroutine_stack(proxy); }
    coroutine_resume(proxy);
}

void coroutine_swap(coroutine* curr, coroutine* pendding) {
    assert(curr && pendding);
    //record curr stack_sp, swap means curr suspend, pendding resume
    char c;
    curr->stack_sp = &c;
    coroutine* occupy = pendding->stack->occupy;
    if(occupy && occupy != pendding) save_coroutine_stack(occupy);
    if(occupy != pendding && pendding->status == COROUTINE_SUSPEND) recover_coroutine_stack(pendding);
    pendding->stack->occupy = pendding;
    swapcontext(&curr->context, &pendding->context);
    return;
}

void coroutine_resume(coroutine* c) {
    coroutine* curr = current_coroutine();
    //not allow to resume current coroutine
    if(!c || c == curr) return;
    switch(c->status) {
        case COROUTINE_READY: 
            getcontext(&c->context);
            if(c->stack == curr->stack) realloc_coroutine_stack(c);
            c->context.uc_stack.ss_sp = c->stack->buffer;
            c->context.uc_stack.ss_size = c->stack->size;
            makecontext(&c->context, _co_inner_func, 0);
            break;
        case COROUTINE_SUSPEND:
            if(c->stack == curr->stack) { 
                coroutine_proxy(curr, c);
                DBG_LOG("[Message] proxy come back, no need to resume pendding, return\n");
                return;
            }
            break;
        case COROUTINE_DEAD:
            DBG_LOG("[Warnning] dead coroutine: %p,  status: %s, return\n", c, STATUS(c->status));
            return;

        default:
            DBG_LOG("[Error] invalid coroutine status: %s, assert\n", STATUS(c->status));
            assert(0);
    }

    push_coroutine(c);
    coroutine_swap(curr, c);
    return;
}

void coroutine_yield() {
    coroutine* pendding = previous_coroutine();
    coroutine* curr = pop_coroutine();

    //find alive pendding coroutine
    while(pendding) {
        if(pendding->status != COROUTINE_DEAD) break;
        pendding = previous_coroutine();
        pop_coroutine();
    }

    curr->status = COROUTINE_SUSPEND;
    coroutine_swap(curr, pendding);
}

void coroutine_die() {
    coroutine* pendding = previous_coroutine();
    coroutine* curr = pop_coroutine();

    //find alive pendding coroutine
    while(pendding) {
        if(pendding->status != COROUTINE_DEAD) break;
        pendding = previous_coroutine();
        pop_coroutine();
    }

    curr->status = COROUTINE_DEAD;
    curr->stack->occupy = NULL;
    //give up stack owner 
    curr->stack->shared = 1;

#ifdef AUTO_FREE_COROUTINE
    free_coroutine(curr);
#endif

    //free inner coroutine
    if(curr->inner) free_coroutine(curr);
    setcontext(&pendding->context);
}
