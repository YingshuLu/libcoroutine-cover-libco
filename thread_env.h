#ifndef THREAD_ENV_H_
#define THREAD_ENV_H_

#include "defines.h"
#include "event.h"
#include "inner_fd.h"

#define MAX_EVENTS_SIZE 10240
#define EPOLL_WAIT_TIMEOUT -1

__thread epoll_t* g_epoll = NULL;

epoll_t *init_thread_env() {
    if(!g_epoll) { 
        g_epoll = new_epoll(MAX_EVENTS_SIZE, EPOLL_WAIT_TIMEOUT);
        init_epoll_timer(g_epoll, 60, 1); 
        co_env_init();
    }
    return g_epoll;
}

epoll_t* get_thread_epoll() {
   if(!g_epoll) g_epoll = init_thread_env();
   return g_epoll;
}

//should call in main co-task
void destory_thread_env() {
   close_all_inner_fd();
   if(g_epoll) delete_epoll(get_thread_epoll());
   co_env_destory();
}

void co_enable_hook();
void co_disable_hook();
bool co_hooked();

#endif
