#include "thread_env.h"
#include "task_pool.h"

task_pool_t *g_pool = NULL;

void foo(void *ip, void *op) {
    printf("foo first\n");
    printf("value : %d\n", *((int *)ip));
    printf("ovalue : %d\n", *((int *)op));
    int key = co_alloc_key();
    printf("key : %d\n", key);
    co_yield();
    printf("foo second\n");
}

void goo(void *ip, void *op) {
    printf("goo first\n");
    co_yield();
    printf("goo second\n");
}

void callback() {
    printf(">>callback, %p\n", co_self());
}

int main() {
    g_pool = new_task_pool(-1);
    init_thread_env();

    int value = 125;
    int ov = 189;
    task_t *t = get_task_from_pool(g_pool, foo, (void *)&value, (void *)&ov, callback);
    co_resume(t);
    co_resume(t);

    t = get_task_from_pool(g_pool, goo, NULL, NULL, NULL);

    co_resume(t);
    co_resume(t);

    event_loop(get_thread_epoll());
    destory_thread_env();
    delete_task_pool(g_pool);
    return 0;
}
