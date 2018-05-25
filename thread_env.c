#include "task.h"
#include "event.h"
#include "inner_fd.h"
#include "thread_env.h"

static __thread epoll_t* g_epoll = NULL;

epoll_t *init_thread_env() {
    if(!g_epoll) { 
        g_epoll = new_epoll(MAX_EVENTS_SIZE, EPOLL_WAIT_TIMEOUT);
        init_epoll_timer(g_epoll, 60, 1); 
        co_env_init();
    }
    return g_epoll;
}

epoll_t* current_thread_epoll() {
   if(!g_epoll) g_epoll = init_thread_env();
   return g_epoll;
}

//should call in main co-task
void destory_thread_env() {
   close_all_inner_fd();
   if(g_epoll) delete_epoll(current_thread_epoll());
   co_env_destory();
}

