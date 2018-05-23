#include "task.h"

task_t* new_task(cofunc_t f, void* ip, void* op, cocb_t cb ) {
    task_t* tp = create_coroutine(current_scheduler(), f, ip, op);
    if(cb) coroutine_set_callback(tp, cb);
    return tp;
}

int reuse_task(task_t* tp, cofunc_t f, void* ip, void* op, cocb_t cb) {
    if (!tp) return -1;
    reset_coroutine(current_scheduler(), tp, f, ip, op);
    coroutine_set_callback(tp, cb);
    return 0;
}

