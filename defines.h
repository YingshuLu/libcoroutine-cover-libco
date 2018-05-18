#ifndef TASK_DEFINES_H_
#define TASK_DEFINES_H_

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>
#include "coroutine.h"

/*coroutine defines*/
typedef coroutine task_t;

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

#define co_self() current_coroutine()
#define co_resume(tp) coroutine_resume(tp) 

//called in coroutine
#define co_yield()      coroutine_yield()
#define co_alloc_key() coroutine_create_specific_key()
#define co_set_value(key, value) coroutine_set_specific(key, value)
#define co_get_value(key) coroutine_get_specific(key)

//#define co_enable_hook() coroutine_enable_hook(co_self())
//#define co_disable_hook() coroutine_disable_hook(co_self())
//#define co_hooked() coroutine_hooked(co_self())

#define delete_task(tp) free_coroutine(tp)
//called in thread
#define co_env_init() current_scheduler()
#define co_env_destory() free_scheduler(current_scheduler())
/*coroutine defines*/

typedef int bool;
#define true  1
#define false 0

#define MAX_ERROR_BUFFER_SIZE 1024
__thread char *_error_buffer = NULL;
void dperror(int ret) {
    if(!_error_buffer) _error_buffer = (char *)malloc(MAX_ERROR_BUFFER_SIZE);
    strerror_r(errno, _error_buffer, MAX_ERROR_BUFFER_SIZE);
    printf("[Error] return value: %d, errno: %d, %s <%s>@<%s:%d>\n", ret, errno, _error_buffer, __func__, basename(__FILE__), __LINE__);
}

void warnf(const char *buf) {
    printf("[warnning]");
    printf("%s",buf);
}
#endif
