#include "defines.h"

void foo(void* ip, void* op) {
    printf("[%p] first enter coroutine\n", co_self());
    int key1 = co_alloc_key();
    int* v = malloc(sizeof(int));
    *v = 24;
    co_set_value(key1, v);
    co_yield();
    printf("[%p] second enter coroutine\n", co_self());
    int* p = co_get_value(key1);
    printf("get key/value: %d:%d\n", key1, *p);
    
}

void goo(void* ip, void* op) {
    printf("in goo routine\n");
}

void callback() {
   printf("callback task\n");
}

int main () {
    co_env_init();
    task_t* tp = new_task(foo, NULL, NULL, NULL);
    co_resume(tp);
    co_resume(tp);
    int ret = reuse_task(tp, goo, NULL, NULL, callback);
    printf("reuse goo routine\n");
    co_resume(tp);
    delete_task(tp);
    co_env_destory();
    return 0; 
}
