#ifndef THREAD_ENV_H_
#define THREAD_ENV_H_

#include "event.h"

#define MAX_EVENTS_SIZE 10240
#define EPOLL_WAIT_TIMEOUT 60*1000 

epoll_t *init_thread_env();
epoll_t* current_thread_epoll();
void destory_thread_env();
  
#endif
