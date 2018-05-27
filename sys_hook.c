#define _GNU_SOURCE

#include <dlfcn.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdarg.h>
#include "event.h"
#include "inner_fd.h"
#include "sys_hook.h"
#include "task.h"
#include "thread_env.h"

struct sockaddr;
struct msghdr;
typedef int socklen_t;

typedef int (*fcntl_pfn_t)(int fd, int cnd, ...);
typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*listen_pfn_t)(int sockfd, int backlog);

typedef int (*connect_pfn_t)(int sockfd, const struct sockaddr *address, socklen_t address_len);
typedef int (*accept_pfn_t)(int sockfd, struct sockaddr *address, socklen_t *address_len);
typedef int (*read_pfn_t)(int fd, void *buffer, size_t n);
typedef int (*write_pfn_t)(int fd, const void *buffer, size_t n);
typedef int (*close_pfn_t)(int fd);

typedef int (*recv_pfn_t)(int sockfd, void *buf, size_t len, int flags);
typedef int (*recvfrom_pfn_t)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrelen);
typedef int (*recvmsg_pfn_t)(int sockfd, struct msghdr* msg, int flags);

typedef int (*send_pfn_t)(int sockfd, const void *buf, size_t len, int flags);
typedef int (*sendto_pfn_t)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrelen);
typedef int (*sendmsg_pfn_t)(int sockfd, const struct msghdr *msg, int flags);

static fcntl_pfn_t     hook_fcntl_pfn    = NULL;
static socket_pfn_t    hook_socket_pfn   = NULL; 
static listen_pfn_t    hook_listen_pfn   = NULL;
static connect_pfn_t   hook_connect_pfn  = NULL; 
static accept_pfn_t    hook_accept_pfn   = NULL; 
static read_pfn_t      hook_read_pfn     = NULL; 
static write_pfn_t     hook_write_pfn    = NULL; 
static close_pfn_t     hook_close_pfn    = NULL; 
static recv_pfn_t      hook_recv_pfn     = NULL;
static recvfrom_pfn_t  hook_recvfrom_pfn = NULL;
static recvmsg_pfn_t   hook_recvmsg_pfn  = NULL;
static send_pfn_t      hook_send_pfn     = NULL;
static sendto_pfn_t    hook_sendto_pfn   = NULL;
static sendmsg_pfn_t   hook_sendmsg_pfn  = NULL;

#define HOOK_SYS_CALL(func) { if(!hook_##func##_pfn) hook_##func##_pfn = (func##_pfn_t)dlsym(RTLD_NEXT, #func); }

bool co_hooked() {
    coroutine_hooked(co_self());
}

int fcntl(int fd, int cmd, ...) {
	HOOK_SYS_CALL(fcntl);
	if( fd < 0 ){
        errno = EBADF;
		return -1;
	}

	va_list arg_list;
	va_start( arg_list,cmd );

	int ret = -1;
	inner_fd *ifd = get_inner_fd(fd);
	switch( cmd ) {
        case F_DUPFD_CLOEXEC:
		case F_DUPFD: {
			int param = va_arg(arg_list, int);
			ret = hook_fcntl_pfn(fd, cmd, param );

            if(ret > 0 && co_hooked() && ifd && (ifd->flags & O_NONBLOCK)) {
                new_inner_fd(ret);
            }
			break;
		}
		case F_GETFD: {
			ret = hook_fcntl_pfn(fd, cmd);
			break;
		}
		case F_SETFD: {
			int param = va_arg(arg_list, int);
			ret = hook_fcntl_pfn( fd, cmd, param );
			break;
		}
		case F_GETFL: {
            if(!co_hooked() || !ifd) ret = hook_fcntl_pfn(fd, cmd);
            else ret = ifd->flags;
			break;
		}
		case F_SETFL: {
			int flags = va_arg(arg_list, int);
            if(!co_hooked()) {
			    ret = hook_fcntl_pfn(fd, cmd, flags);
                break;
            }

            if(!ifd && (flags & O_NONBLOCK)) {
                ifd = new_inner_fd(fd);
            }

            if(ifd && ifd->flags == flags) {
                ret = 0;
                break;
            }
			ret = hook_fcntl_pfn(fd, cmd, flags);
			if(0 == ret && ifd) ifd->flags = flags;
			break;
		}
		case F_GETOWN: {
			ret = hook_fcntl_pfn(fd, cmd);
			break;
		}
		case F_SETOWN: {
			int param = va_arg(arg_list, int);
			ret = hook_fcntl_pfn(fd, cmd, param);
			break;
		}
		case F_GETLK: {
			struct flock *param = va_arg(arg_list, struct flock *);
			ret = hook_fcntl_pfn(fd, cmd, param);
			break;
		}
		case F_SETLK: {
			struct flock *param = va_arg(arg_list, struct flock *);
			ret = hook_fcntl_pfn(fd, cmd, param);
			break;
		}
		case F_SETLKW: {
			struct flock *param = va_arg(arg_list, struct flock *);
			ret = hook_fcntl_pfn(fd, cmd, param);
			break;
		}
	}
	va_end(arg_list);
	return ret;
}

int socket(int domain, int type, int protocol) {
    HOOK_SYS_CALL(socket);
    int sockfd = hook_socket_pfn(domain, type, protocol);
    DBG_LOG("socket hooked\n");
    if(sockfd < 0 || !co_hooked()) return sockfd;
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
    return sockfd;
}

int listen(int sockfd , int backlog) {
    HOOK_SYS_CALL(listen);
    inner_fd *ifd = get_inner_fd(sockfd);
    if(co_hooked() && ifd) ifd->timeout = -1;
    DBG_LOG("listen hooked\n");
    return  hook_listen_pfn(sockfd, backlog);
}

int connect(int sockfd, const struct sockaddr *address, socklen_t address_len) {
    HOOK_SYS_CALL(connect);
    if(!co_hooked()) return hook_connect_pfn(sockfd, address, address_len);
    inner_fd *isfd = get_inner_fd(sockfd);
    if(!isfd || !(O_NONBLOCK & isfd->flags)) return hook_connect_pfn(sockfd, address, address_len);

    DBG_LOG("connect hooked\n");
    int ret = hook_connect_pfn(sockfd, address, address_len);
    if(ret == -1 && (errno == EALREADY || errno == EINPROGRESS)) {
        int events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
         ret = poll(current_thread_epoll(), sockfd, events);
         if(ret != 0) {
            errno = EBADF;
            return -1;
        }
    }
    return ret;
}

int accept(int sockfd, struct sockaddr *address, socklen_t *address_len) {
    HOOK_SYS_CALL(accept);
    if(!co_hooked()) return hook_accept_pfn(sockfd, address, address_len);
    inner_fd *isfd = get_inner_fd(sockfd);
    if(!isfd || !(O_NONBLOCK & isfd->flags)) return hook_accept_pfn(sockfd, address, address_len);
    int events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    int ret = poll(current_thread_epoll(), sockfd, events);
    if(ret != 0) {
        errno = EBADF;
        return -1;
    }
    
    ret = hook_accept_pfn(sockfd, address, address_len);
    if(ret < 0) {
        errno = EBADF;
        return -1;
    }   

    fcntl(ret, F_SETFL, fcntl(ret, F_GETFL) | O_NONBLOCK);
    return ret;
}

int read(int fd, void *buffer, size_t n) {
    HOOK_SYS_CALL(read);
    if(!co_hooked()) return hook_read_pfn(fd, buffer, n);
    inner_fd *ifd = get_inner_fd(fd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_read_pfn(fd, buffer, n);

    //DBG_LOG("read hooked\n");
    int events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    int ret = poll(current_thread_epoll(), fd, events);
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

    DBG_LOG("write hooked\n");
    int len = 0;
    int ret = hook_write_pfn(fd, buffer, n);
    if(ret == 0) {
        return ret;
    }
    else if(ret > 0) {
        len += ret;
    }
    
    int events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
    while(len < n) {
        ret = poll(current_thread_epoll(), fd, events);
        if (ret != 0) {
            errno = EBADF;
            return -1;
        }
        ret = hook_write_pfn(fd, buffer, n);
        if(ret < 0 && len == 0) {
            return ret;
        }
        len += ret;
    }

    return len;
}

int close(int fd) {
    HOOK_SYS_CALL(close);
    if(!co_hooked()) return hook_close_pfn(fd);
    DBG_LOG("close hooked\n");
    inner_fd *ifd = get_inner_fd(fd);
    if(ifd) delete_inner_fd(fd);
    return hook_close_pfn(fd);
}

int recv(int sockfd, void *buf, size_t len, int flags) {
    HOOK_SYS_CALL(recv);
    if(!co_hooked()) return hook_recv_pfn(sockfd, buf, len, flags);
    inner_fd *ifd = get_inner_fd(sockfd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_recv_pfn(sockfd, buf, len, flags);
    int events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    int ret = poll(current_thread_epoll(), sockfd, events);
    if (ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_recv_pfn(sockfd, buf, len, flags);
}

int recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    HOOK_SYS_CALL(recvfrom);
    if(!co_hooked()) return hook_recvfrom_pfn(sockfd, buf, len, flags, src_addr, addrlen);
    inner_fd *ifd = get_inner_fd(sockfd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_recvfrom_pfn(sockfd, buf, len, flags, src_addr, addrlen);
    int events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    int ret = poll(current_thread_epoll(), sockfd, events);
    if (ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_recvfrom_pfn(sockfd, buf, len, flags, src_addr, addrlen);
}

int recvmsg(int sockfd, struct msghdr *msg, int flags) {
    HOOK_SYS_CALL(recvmsg);
    if(!co_hooked()) return hook_recvmsg_pfn(sockfd, msg, flags);
    inner_fd *ifd = get_inner_fd(sockfd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_recvmsg_pfn(sockfd, msg, flags);
    int events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    int ret = poll(current_thread_epoll(), sockfd, events);
    if (ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_recvmsg_pfn(sockfd, msg, flags);
}

int send(int sockfd, const void *buf, size_t n, int flags) {
    HOOK_SYS_CALL(send);
    if(!co_hooked()) return hook_send_pfn(sockfd, buf, n, flags);
    inner_fd *ifd = get_inner_fd(sockfd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_send_pfn(sockfd, buf, n, flags);

    int len = 0;
    int ret = hook_send_pfn(sockfd, buf, n, flags);
    if(ret == 0) {
        return ret;
    }
    else if(ret > 0) {
        len += ret;
    }
    
    int events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
    while(len < n) {
        ret = poll(current_thread_epoll(), sockfd, events);
        if (ret != 0) {
            errno = EBADF;
            return -1;
        }
        ret = hook_send_pfn(sockfd, buf, n, flags);
        if(ret < 0 && len == 0) {
            return ret;
        }
        len += ret;
    }
    return len;
}

int sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    HOOK_SYS_CALL(sendto);
    if(!co_hooked()) return hook_sendto_pfn(sockfd, buf, len, flags, dest_addr, addrlen);
    inner_fd *ifd = get_inner_fd(sockfd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_sendto_pfn(sockfd, buf, len, flags, dest_addr, addrlen);
    int events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
    int ret = poll(current_thread_epoll(), sockfd, events);
    if (ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_sendto_pfn(sockfd, buf, len, flags, dest_addr, addrlen);
}

int sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    HOOK_SYS_CALL(sendmsg);
    if(!co_hooked()) return hook_sendmsg_pfn(sockfd, msg, flags);
    inner_fd *ifd = get_inner_fd(sockfd);
    if(!ifd || !(O_NONBLOCK & ifd->flags)) return hook_sendmsg_pfn(sockfd, msg, flags);
    int events = EPOLLOUT | EPOLLRDHUP | EPOLLERR;
    int ret = poll(current_thread_epoll(), sockfd, events);
    if (ret != 0) {
        errno = EBADF;
        return -1;
    }
    return hook_sendmsg_pfn(sockfd, msg, flags);
}

void co_enable_hook() {
    coroutine_enable_hook(co_self());
}

void co_disable_hook() {
    coroutine_disable_hook(co_self());
}
