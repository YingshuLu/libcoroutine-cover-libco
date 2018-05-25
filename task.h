#ifndef TASK_DEFINES_H_
#define TASK_DEFINES_H_

#include "coroutine.h"

typedef coroutine task_t;
task_t* new_task(cofunc_t f, void* ip, void* op, cocb_t cb );
int reuse_task(task_t* tp, cofunc_t f, void* ip, void* op, cocb_t cb);

#define co_self() current_coroutine()
#define co_resume(tp) coroutine_resume(tp) 
#define co_yield()      coroutine_yield()
#define co_cancel(tp) coroutine_cancel(tp)


#define co_alloc_key() coroutine_create_specific_key()
#define co_set_value(key, value) coroutine_set_specific(key, value)
#define co_get_value(key) coroutine_get_specific(key)
#define delete_task(tp) free_coroutine(tp)

//called in thread
#define co_env_init() current_scheduler()
#define co_env_destory() free_scheduler(current_scheduler())

#endif
