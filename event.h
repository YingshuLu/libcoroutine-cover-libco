#ifndef EVENT_H_
#define EVENT_H_

#include <stddef.h>
typedef struct _epoll_st epoll_t;

epoll_t* new_epoll(size_t maxevents, int timeout);
void init_epoll_timer(epoll_t *ep, size_t sec_size, size_t interval);
void delete_epoll(epoll_t *ep);

int add_events(epoll_t *ep, int fd, int events);
int delete_events(epoll_t *ep, int fd, int events);
int modify_events(epoll_t *ep, int fd, int events);

int event_poll(epoll_t* ep, int fd, int events);
void stop_event_loop(epoll_t* ep);
int event_loop(epoll_t *ep);
    
#endif
