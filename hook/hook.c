#define _GNU_SOURCE
#include <dlfcn.h>
#include "hook.h"

typedef void* (*malloc_pfn_t)(size_t size);

static malloc_pfn_t hook_malloc_pfn = 0;

#define HOOK_SYS_CALL(func) { hook_##func##_pfn = (func##_pfn_t) dlsym(RTLD_NEXT, #func); }

volatile int hooked = 0;

void* malloc(size_t size) {
    HOOK_SYS_CALL(malloc);
    if(is_hooked()) {
        disable_hook();
        printf("hooked malloc\n");
        enable_hook();
    }
    return hook_malloc_pfn(size);
}

void enable_hook() { hooked = 1; }
void disable_hook() { hooked = 0; }
int is_hooked() { return hooked; }


