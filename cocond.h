#ifndef CO_COND_H_
#define CO_COND_H_

#include "queue.h"
#include "task.h"

struct _cocond_st {
    queue_t que;
};

int co_cond_wait(cocond_t *cond) {
    if(!cond) return -1;
    queue_push(&(cond->que), (void *)co_self());
    co_yield();
    return 0;
}
int co_cond_notify(cocond_t *cond) {
    if(!cond) || queue_empty(&(cond->que)) return -1;
    task_t *t = (task_t *)queue_pop(&(cond->que));
    co_resume(t);
    return 0;

}
int co_cond_notify_all(cocond_t *cond) {
    while(cond && !queue_empty(&(con->que))) {
        co_cond_notify(cond);
    }
    return 0;
}


#endif
