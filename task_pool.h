#ifndef TASK_POOL_H_
#define TASK_POOL_H_

#include "task.h"

typedef struct _task_pool task_pool_t;
task_pool_t* new_task_pool(int num);
void delete_task_pool(task_pool_t *tp);
task_t* get_task_from_pool(task_pool_t *pool, cofunc_t f, void *ip, void *op, cocb_t cb);

#endif
