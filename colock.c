#include "task.h"
#include "types.h"
#include "queue.h"
#include "colock.h"

struct _co_lock {
    volatile bool locked;
    task_t* owner;
    queue_t que;    
};

typedef struct _co_lock colock_t;

void colock_init(colock_t *lock) {
    if(!lock) return;
    lock->locked = false;
    queue_init(&(lock->que));
    lock->owner = NULL;
}

void colock_destory(colock_t *lock) {
    if(!lock) return;
    lock->locked = false;
    lock->owner = NULL;
    queue_destory(&(lock->que));
}

int co_lock(colock_t *lock) {
    while(1) {
        if(!lock->locked) {
            lock->locked = true;
            lock->owner = co_self();
            return 0;
        }
        //reenter-lock
        if(lock->owner == co_self()) return 0;
        queue_push(&(lock->que), (void *)co_self());
        co_yield();
    }
    return -1;
}

int co_unlock(colock_t *lock) {
    if(co_self() != lock->owner || !lock->locked) return -1;
    lock->locked = false;
    lock->owner = NULL;
    if(!queue_empty(&(lock->que))) {
        task_t *t = (task_t *)queue_pop(&(lock->que));
        scheduler_after(co_self(), t);
        //co_resume(t);
    }
    return 0;
}

