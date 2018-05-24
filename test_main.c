#include "task.h"

task_t* g_task = NULL;
void goo(void* ip, void* op);
void too(void* n, void* m) {
    printf("in too, delete global task\n");
    reuse_task(g_task, goo, NULL, NULL, NULL);
    co_resume(g_task);
}

void foo(void* ip, void* op) {
    printf("[%p] first enter coroutine\n", co_self());
    int key1 = co_alloc_key();
    int key2 = co_alloc_key();
    int key3 = co_alloc_key();
    int* v = malloc(sizeof(int));
    *v = 24;
    int* v1 = malloc(sizeof(int));
    *v1 = 129;

    int* v2 = malloc(sizeof(int));
    *v2 = 256;


    co_set_value(key1, v);
    co_set_value(key2, v1);
    co_set_value(key3, v2);
    co_yield();
    printf("[%p] second enter coroutine\n", co_self());
    int* p = co_get_value(key1);
    printf("get key/value: %d:%d\n", key1, *p);
    printf("get key/value: %d:%d\n", key2, *((int *)co_get_value(key2)));
    printf("get key/value: %d:%d\n", key3, *((int *)co_get_value(key3)));
    
    task_t* tp = new_task(too, NULL, NULL, NULL);
    co_resume(tp);
    printf("old task, never be here\n");
}

void goo(void* ip, void* op) {
    printf("new task, in goo routine\n");
}

void callback() {
   printf("callback task\n");
}


int main () {
    co_env_init();
    g_task = new_task(foo, NULL, NULL, NULL);
    task_t* tp = g_task;
    co_resume(tp);
    co_resume(tp);
    /*
    int ret = reuse_task(tp, goo, NULL, NULL, callback);
    printf("reuse goo routine\n");
    co_resume(tp);
    delete_task(tp);
    */
    co_env_destory();
    return 0; 
}
