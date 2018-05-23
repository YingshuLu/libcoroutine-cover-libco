#define _GNU_SOURCE

#include <dlfcn.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "event.h"
#include "inner_fd.h"
#include "sys_hook.h"
#include "task.h"
#include "thread_env.h"

typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*connect_pfn_t)(int sockfd, const struct sockaddr *address, socklen_t address_len);
typedef int (*accept_pfn_t)(int sockfd, struct sockaddr *address, socklen_t *address_len);
typedef int (*read_pfn_t)(int fd, void *buffer, size_t n);
typedef int (*write_pfn_t)(int fd, const void *buffer, size_t n);
typedef int (*close_pfn_t)(int fd);

static socket_pfn_t  hook_socket_pfn  = NULL; 
static connect_pfn_t hook_connect_pfn = NULL; 
static accept_pfn_t  hook_accept_pfn  = NULL; 
static read_pfn_t    hook_read_pfn    = NULL; 
static write_pfn_t   hook_write_pfn   = NULL; 
static close_pfn_t   hook_close_pfn   = NULL; 

#define HOOK_SYS_CALL(func) { if(!hook_##func##_pfn) hook_##func##_pfn = (func##_pfn_t)dlsym(RTLD_NEXT, #func); }

bool co_hooked() {
    coroutine_hooked(co_self());
}

int socket(int domain, int type, int protocol) {
    HOOK_SYS_CALL(socket);
    int sockfd = hook_socket_pfn(domain, type, protocol);
    if(sockfd < 0 || !co_hooked()) return sockfd;
    new_inner_fd(sockfd);
    return sockfd;
}

int connect(int sockfd, const struct sockaddr *address, socklen_t address_len) {
    HOOK_SYS_CALL(connect);
    if(!co_hooked()) return hook_connect_pfn(sockfd, address, address_len);
    inner_fd *isfd = get_inner_fd(sockfd);
    if(!isfd || !(O_NONBLOCK & isfd->flags)) return hook_connect_pfn(sockfd, address, address_len);
    int events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
    int ret = poll(get_thread_epoll(), sockfd, events);
    if(ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_connect_pfn(sockfd, address, address_len);
}

int accept(int sockfd, struct sockaddr *address, socklen_t *address_len) {
    HOOK_SYS_CALL(accept);
    if(!co_hooked()) return hook_accept_pfn(sockfd, address, address_len);
    inner_fd *isfd = get_inner_fd(sockfd);
    if(!isfd || !(O_NONBLOCK & isfd->flags)) return hook_accept_pfn(sockfd, address, address_len);
    int events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    int ret = poll(get_thread_epoll(), sockfd, events);
    if(ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_accept_pfn(sockfd, address, address_len);
}

int read(int fd, void *buffer, size_t n) {
    HOOK_SYS_CALL(read);
    if(!co_hooked()) return hook_read_pfn(fd, buffer, n);
    inner_fd *ifd = get_inner_fd(fd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_read_pfn(fd, buffer, n);
    DBG_LOG("co hook read\n");
    int events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    int ret = poll(get_thread_epoll(), fd, events);
    if (ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_read_pfn(fd, buffer, n);
}

int write(int fd, const void *buffer, size_t n) {
    HOOK_SYS_CALL(write);
    if(!co_hooked()) return hook_write_pfn(fd, buffer, n);
    inner_fd *ifd = get_inner_fd(fd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_write_pfn(fd, buffer, n);
    int events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
    int ret = poll(get_thread_epoll(), fd, events);
    if (ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_write_pfn(fd, buffer, n);
}

int close(int fd) {
    HOOK_SYS_CALL(close);
    if(!co_hooked()) return hook_close_pfn(fd);
    inner_fd *ifd = get_inner_fd(fd);
    if(ifd) delete_inner_fd(fd);
    return hook_close_pfn(fd);
}

void co_enable_hook() {
    coroutine_enable_hook(co_self());
}

void co_disable_hook() {
    coroutine_disable_hook(co_self());
}
