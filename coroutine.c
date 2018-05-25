#include <ucontext.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <assert.h>
#include "coroutine.h"
#include "run_stacks.h"
#include "array.h"
#include "list.h"

#ifndef SET_COROUTINE_STACK_SIZE
#define COROUTINE_STACK_SIZE  64 * 1024
#else
#define COROUTINE_STACK_SIZE SET_COROUTINE_STACK_SIZE
#endif

#ifndef SET_COROUTINE_STACK_COUNT
#define COROUTINE_STACK_COUNT 7
#else
#define COROUTINE_STACK_COUNT SET_COROUTINE_STACK_COUNT
#endif

//coroutine max call stack depth
#define SCHEDULER_COROUTINE_MAX_SIZE 1024
#define COROUTINE_SPECIFIC_CAPCITY 128

#define list_to_coroutine(ls) list_to_struct(ls, coroutine, call_link);

__thread scheduler* g_scheduler = NULL;

const char* _status_strs[] = { "Ready", "Running", "Suspend", "Dead" };

#define STATUS(s) ((s < COROUTINE_READY || s > COROUTINE_DEAD)? "Invalid status" :_status_strs[s])

struct _coroutine {
    stack_st* stack;
    char* stack_sp;

    cofunc_t func;
    void* iparam;
    void* oparam;    
    cocb_t cb;
    status_t status;

    ucontext_t context;
    
    char* snapshot;
    size_t snapshot_size;
    size_t snapshot_capcity;

    array_t specs;
    volatile int hook;
    char inner;
    list_t call_link;
};

struct _scheduler {
    stack_pool_st* spool;
    coroutine* co_main;
    list_t call_list;
};

void save_coroutine_stack(coroutine*);
void coroutine_die();

void coroutine_enable_hook(coroutine *c) {
    c->hook = 1;
}

void coroutine_disable_hook(coroutine *c) {
    c->hook = 0;
}

int coroutine_hooked(coroutine *c) {
    return c->hook == 1;
}

void coroutine_set_callback(coroutine *c, cocb_t cb) {
    if(c) c->cb = cb;
}

void give_up_stack(coroutine *c) {
    if(!c || !c->stack) return;
    c->stack->occupy = NULL;
    c->stack->shared = 1;
    c->stack = NULL;
}

int coroutine_create_specific_key() {
    coroutine* curr = current_coroutine();
    int idx = array_size(&(curr->specs));
    array_insert(&(curr->specs), idx, NULL);
    return idx;
}

int coroutine_set_specific(int key, void* value) {
    coroutine* curr = current_coroutine();
    return array_put(&(curr->specs), key, value);
}

void* coroutine_get_specific(int key) {
    coroutine* curr = current_coroutine();
    return array_get(&(curr->specs), key);
}

scheduler* current_scheduler() {
    if(g_scheduler) return g_scheduler;
    return create_scheduler(SCHEDULER_COROUTINE_MAX_SIZE, COROUTINE_STACK_COUNT, COROUTINE_STACK_SIZE);
}

coroutine* current_coroutine() {
    return list_to_coroutine(g_scheduler->call_list.prev);
}

coroutine* main_coroutine() {
    return current_scheduler()->co_main;
}

coroutine* previous_coroutine() {
    coroutine* curr = current_coroutine();
    if(main_coroutine() == curr || !curr) return NULL;
    return list_to_coroutine(curr->call_link.prev);
}

void free_coroutine(coroutine* c) {
    if(!c) return;
    give_up_stack(c);
    if(!list_empty(&(c->call_link))) list_delete(&(c->call_link));
    free(c->snapshot);
    array_destory(&(c->specs));
    free(c);
}

//can only be called on main coroutine
void free_scheduler(scheduler* s) {
    if(!s) return;
    free_coroutine(s->co_main);
    free_stack_pool(s->spool);
}

void push_coroutine(coroutine* c) {
    if(c) {
        if(!list_empty(&(c->call_link))) list_delete(&(c->call_link));
        list_add_before(&(g_scheduler->call_list), &(c->call_link));
    }
}

coroutine* pop_coroutine() {
    //can not pop co_main
    if(list_next(g_scheduler->call_list.next) != &(g_scheduler->call_list)) {
      coroutine* c = list_to_coroutine(g_scheduler->call_list.prev);
      list_delete(&(c->call_link));
      return c;
    }
    return NULL;
}

void _co_inner_func(void) {    
    coroutine* c = current_coroutine();
    assert(c);

    c->status = COROUTINE_RUNNING;
    c->func(c->iparam, c->oparam);
    
    DBG_LOG("coroutine: %p go die\n", c);
    coroutine_die();
    return;
}

coroutine* create_coroutine(scheduler* shed, cofunc_t f, void* ip, void* op) {
    coroutine* c = (coroutine*) malloc(sizeof(coroutine));
    list_init(&(c->call_link));
    return reset_coroutine(shed, c, f, ip, op);
}

coroutine* reset_coroutine(scheduler* shed, coroutine* c, cofunc_t f, void* ip, void* op) {
    if(!c) return NULL;
    c->func = f;
    c->iparam = ip;
    c->oparam = op;

    c->status = COROUTINE_READY;
    if(c->snapshot) free(c->snapshot);
    c->snapshot = NULL;
    c->snapshot_size = 0;
    c->snapshot_capcity = 0;
    c->cb = NULL;

    give_up_stack(c); 

    c->stack_sp = NULL;
    c->inner = 0;
    c->hook = 0;
    array_destory(&(c->specs));
    if(!list_empty(&(c->call_link))) list_delete(&(c->call_link));
    return c;
}

scheduler* create_scheduler(size_t cocap, size_t stack_count, size_t stack_size) {
    if(g_scheduler) return g_scheduler;

    g_scheduler = (scheduler*) malloc(sizeof(scheduler));
    g_scheduler->spool = create_stack_pool(stack_count, stack_size);

    //main coroutine use ioslate stack memory
    coroutine* co_main = create_coroutine(NULL, NULL, NULL, NULL);
    co_main->stack = alloc_isolate_stack(g_scheduler->spool, 1);
    g_scheduler->co_main = co_main;

    list_init(&(g_scheduler->call_list));
    push_coroutine(co_main);
        
    return g_scheduler;
}

void save_coroutine_stack(coroutine* c) {
    assert(c);
    if(c->snapshot_capcity < (c->stack->stack_bp - c->stack_sp) || !c->snapshot) {
        free(c->snapshot);
        //expand 2 times capcity
        c->snapshot_capcity = 2 * (c->stack->stack_bp - c->stack_sp);
        assert(c->snapshot_capcity < COROUTINE_STACK_SIZE);
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
    coroutine* curr = current_coroutine();
    //re-alloc a different stack
    stack_st* stk, *ostk;
    int cnt = 3;
    do {
        stk = alloc_shared_stack(shed->spool);
        DBG_LOG("find next stack_st in pool, left count %d\n", cnt);
    } while(stk == curr->stack && --cnt);

    if( !stk || stk == curr->stack) {
        stk = alloc_isolate_stack(shed->spool, curr->stack->size);
        DBG_LOG("create new stack_st: %p\n", stk);
    }

    ostk = c->stack;
    if(ostk && !ostk->shared) ostk->shared = 1;
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
    proxy->inner = 1;
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
            if(!c->stack || c->stack == curr->stack) realloc_coroutine_stack(c);
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
    if(current_coroutine()->cb) current_coroutine()->cb();
    coroutine* pendding = previous_coroutine();
    coroutine* curr = pop_coroutine();

    //find alive pendding coroutine
    while(pendding) {
        if(pendding->status != COROUTINE_DEAD) break;
        pendding = previous_coroutine();
        pop_coroutine();
    }

    curr->status = COROUTINE_DEAD;
    give_up_stack(curr);
    if(!list_empty(&(curr->call_link))) list_delete(&(curr->call_link));

    //only free inner coroutine
    if(curr->inner) free_coroutine(curr);
    setcontext(&pendding->context);
}

void scheduler_after(coroutine *t1, coroutine *t2) {
    if(!t1 || !t2) return;
    list_delete(&(t2->call_link));
    if(!list_empty(&(t1->call_link))) list_add_before(&(t1->call_link), &(t2->call_link));
    else coroutine_resume(t2);
}
