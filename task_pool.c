#include "task_pool.h"
#include "types.h"
#include "list.h"

typedef struct {
    list_t link;
    task_t *task;
    void* pa[2];
    void* pb[4];
} task_node_st;

#define list_to_task_node(ls) list_to_struct(ls, task_node_st, link)
#define list_to_task(ls) (list_to_task_node(ls)->task)

struct _task_pool {
    //ready, running & suspend task
    list_t run_list;
    //dead task
    list_t recycle_list;
    int limit;
};

task_node_st* new_task_node() {
    task_node_st *tn = (task_node_st *)malloc(sizeof(task_node_st));
    tn->task = NULL;
    list_init(&(tn->link));
    return tn;
}

void delete_task_node(task_node_st *tn) {
    if(tn) {
        list_delete(&(tn->link));
        delete_task(tn->task);
        free(tn);
    }
}

task_pool_t* new_task_pool(int limit) {
    task_pool_t *pool = (task_pool_t *)malloc(sizeof(task_pool_t));
    pool->limit = limit;
    list_init(&(pool->run_list));
    list_init(&(pool->recycle_list));
    return pool;
}

void delete_task_pool(task_pool_t *pool) {
    if(!pool) return;
    list_t* ls[2] = {0};
    ls[0] = &(pool->run_list);
    ls[1] = &(pool->recycle_list);
    
    list_t *item, *next;
    task_node_st* tn;
    int i = 0;
    for(; i < sizeof(ls) / sizeof(list_t*); i++) {
       item = list_next(ls[i]);
       while(!list_empty(item)) {
           next = list_next(item);
           tn = list_to_task_node(item);
           delete_task_node(tn);
           item = next;
       }
    }
}

void _task_func(void *ip, void *op) {
    void **pa = (void **)ip;
    task_pool_t *pool = (task_pool_t *)(pa[0]);
    task_node_st *tn = (task_node_st *)(pa[1]);
    
    void **pb = (void **)op;
    cofunc_t f = (cofunc_t)(pb[0]);
    void *in = pb[1];
    void *out = pb[2];
    cocb_t cb = (cocb_t)pb[3];
    
    co_set_value(co_alloc_key(), (void *)pool);
    co_set_value(co_alloc_key(), (void *)tn);
    co_set_value(co_alloc_key(), (void *)cb);
    
    f(in, out);
}

void _task_callback() {
    task_t *t = co_self();
    int k1, k2, k3;
    k1 = 0;
    k2 = 1;
    k3 = 2;

    task_pool_t *pool = (task_pool_t *) co_get_value(k1);
    task_node_st *tn = (task_node_st *) co_get_value(k2);
    cocb_t cb = (cocb_t) co_get_value(k3);
    
    if(!list_empty(&(tn->link))) list_delete(&(tn->link));
    list_add_before(&(pool->recycle_list), &(tn->link));
    if(cb) cb();
}

task_t *get_task_from_pool(task_pool_t *pool, cofunc_t f, void *ip, void *op, cocb_t cb) {
    if(!pool) return NULL;

    task_node_st *tn = NULL;
    list_t *ls = list_next(&(pool->recycle_list));
    //printf("pool recycle_list: %p, next list: %p\n",&(pool->recycle_list), ls);
    bool reused = false;
    if(!list_empty(ls)) {
        tn = list_to_task_node(ls);
        DBG_LOG("reuse coroutine\n");
        list_delete(ls);
        reused = true;
    }
    else { 
        tn = new_task_node(); 
        DBG_LOG("new coroutine\n");
    }
    DBG_LOG("addr callback: %p\n", cb);

    tn->pa[0] = (void *)pool;
    tn->pa[1] = (void *)tn;

    tn->pb[0] = (void *)f;
    tn->pb[1] = ip;
    tn->pb[2] = op;
    tn->pb[3] = (void *)cb;
        
    if(reused) reuse_task(tn->task, _task_func, (void *) &(tn->pa), (void *) &(tn->pb), _task_callback);
    else tn->task = new_task(_task_func, (void *) &(tn->pa), (void *) &(tn->pb), _task_callback);
    list_add(&(pool->run_list), &(tn->link));
    return tn->task;
}
