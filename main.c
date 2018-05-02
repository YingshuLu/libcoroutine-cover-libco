#include "coroutine.h"
#include <stdio.h>

void too(void* v, void* r);
void ff(void* v, void* r) {
    printf(" 4th coroutine\n");
    coroutine* c = create_coroutine(current_scheduler(), too, NULL, NULL);
    coroutine_resume(c);
    coroutine_yield();
}

void hh(void* v, void* r ) {
        printf("switch to  3th coroutine\n");
        coroutine* c = create_coroutine(current_scheduler(), ff, NULL, NULL);
        coroutine_resume(c);
        coroutine_yield();
}

void foo(void* n, void* m) {
        printf("value: 1\n");
        printf("value: 2\n");
        coroutine_yield();
        printf("value: 3\n");
        coroutine* c = create_coroutine(current_scheduler(), hh, NULL, NULL);
        coroutine_resume(c);

}

void goo(void* n, void* m) {
        printf("value: x\n");
        printf("value: y\n");
        coroutine_yield();
        printf("value: z\n");
}

void too(void* n, void* m) {
        printf("value: a\n");
        printf("value: b\n");
        coroutine_yield();
        printf("value: c\n");
}

int main() {
    scheduler* shed = current_scheduler();
    
    coroutine* c1 = create_coroutine(shed, foo, NULL, NULL);
    coroutine* c2 = create_coroutine(shed, goo, NULL, NULL);
    coroutine* c3 = create_coroutine(shed, too, NULL, NULL);
    printf("c1 addr: %p\n", c1);
    printf("c2 addr: %p\n", c1);
    printf("c3 addr: %p\n", c1);

    coroutine_resume(c1);
    coroutine_resume(c2);
    coroutine_resume(c3);
    
    printf("on main coroutine\n");

    coroutine_resume(c1);
    coroutine_resume(c2);
    coroutine_resume(c3);

    int count = 100000;
    coroutine** array = (coroutine**) malloc(sizeof(coroutine*) * count);
    int i = 0;
    for(; i < count; i++) 
      array[i] = create_coroutine(shed, foo, NULL, NULL);

    for(i=0; i < count; i++)
      coroutine_resume(array[i]);

    for(i=0; i < count; i++)
      coroutine_resume(array[i]);

    free_scheduler(shed);
    return 0;
}
