#include <sys/epoll.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "task.h"
#include "inner_fd.h"
#include "event.h"
#include "time_wheel.h"
#include "list.h"
#include "sys_hook.h"

struct _epoll_st { 
    size_t maxevents;
    int timeout;
    int fd;
    bool loop;
    struct epoll_event *events;
    time_wheel_t* timer;
};

void _timeout_task(void *ip, void *op) {
    epoll_t *ep = (epoll_t *)ip;
    time_wheel_t *tw = ep->timer;
    inner_fd *ifd = NULL;
    const int buf_size = 8;
    char *buf[8] = {0};
    int ret = 0;

    co_enable_hook();

    while(1) {
        ret = read(tw->fd, buf, buf_size);
        //error, disable timer
        if(ret != buf_size) {
            close(tw->fd);
            dperror(ret);
            break;
        }
        
        list_t *timeout_list = wheel_timeout_list(ep->timer);
        list_t *ls = list_next(timeout_list);
        list_t *next = NULL;
        while(!list_empty(ls)) {
            ifd = list_to_inner_fd(ls);
            ifd->betimeout = true;
            next = list_next(ls);
            wheel_delete_element(ls);
            ls = next;
        }
    }
}

epoll_t* new_epoll(size_t maxevents, int timeout) {
    int epfd = epoll_create(1);
    if(epfd < 0) {
        dperror(epfd);
        return NULL;
    }
    epoll_t* ep = (epoll_t *)malloc(sizeof(epoll_t));
    ep->maxevents = maxevents == 0? 1 : maxevents;
    ep->timeout = timeout < 0? 0 : timeout; //epoll_wait can be blocked
    ep->fd = epfd;
    ep->events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * ep->maxevents); 
    ep->loop = 1;
    return ep;
}

void init_epoll_timer(epoll_t *ep, size_t sec_size, size_t interval) {
    assert(ep && ep->fd >= 0);
    time_wheel_t *tw = new_time_wheel(sec_size, interval);
    ep->timer = tw;
    inner_fd *ifd = new_inner_fd(tw->fd);
    ifd->flags |= O_NONBLOCK;
    ifd->timeout = -1;
    ifd->task = new_task(_timeout_task, (void *)ep, NULL, NULL);
    co_resume(ifd->task);
}

void delete_epoll(epoll_t *ep) {
    if(!ep) return;
    close(ep->fd);
    if(ep->timer) {
        inner_fd *ifd = get_inner_fd(ep->timer->fd);
        if(ifd) delete_task(ifd->task);
        delete_time_wheel(ep->timer);
    }
    free(ep->events);
    free(ep);
}

int cntl_events(epoll_t *ep, int op, int fd, int events) {
    if(!ep || !is_fd_valid(fd)) {
        errno = EBADF;
        return -1;  
    }
    struct epoll_event evst;
    evst.events = events;
    evst.data.fd = fd;
    return epoll_ctl(ep->fd, op, fd, &evst);
}

int add_events(epoll_t *ep, int fd, int events) {
    return cntl_events(ep, EPOLL_CTL_ADD, fd, events);
}

int delete_events(epoll_t *ep, int fd, int events) {
    return cntl_events(ep, EPOLL_CTL_DEL, fd, events);
}

int modify_events(epoll_t *ep, int fd, int events) {
    return cntl_events(ep, EPOLL_CTL_MOD, fd, events);
}

int poll(epoll_t* ep, int fd, int events) {
    inner_fd* ifd = get_inner_fd(fd);
    if(!ifd) return -1;

    if(add_events(ep, fd, events) != 0) {
        dperror(-1);
        return -1;      
    }

    ifd->task = co_self();
    co_yield();
    
    if(delete_events(ep, fd, events) != 0) {
        dperror(-1);
        return -1;
    }

    if(ifd->betimeout) {
        DBG_LOG("fd: %d timeout\n", ifd->fd);   
        return -1;
    }
    return 0;
}

void stop_event_loop(epoll_t* ep) {
    if(ep) ep->loop = 0;
}

//should called in thread mian task
int event_loop(epoll_t *ep) {
    if(!ep) {
        errno = EBADF;
        goto EVENT_ERROR;
    }
    
    int num, i;
    inner_fd* ifd = NULL;
    while(ep->loop) {
        num = epoll_wait(ep->fd, ep->events, ep->maxevents, ep->timeout);
        switch(num) {
            case -1:
                goto EVENT_ERROR;
                break;

            case 0:
                break;

            default: {
               DBG_LOG("epoll wait %d event(s)\n", num);
               for(i = 0; i < num; i++) {
                 ifd = get_inner_fd(ep->events[i].data.fd);
                 if(!ifd->betimeout) wheel_update_element(ep->timer, &(ifd->link), ifd->timeout);
                 if(ifd && ifd->task) co_resume(ifd->task);
               }

            }
        }
    }

    if(!(ep->loop)) return 0;
EVENT_ERROR:
    dperror(-1);
    return -1;

}
