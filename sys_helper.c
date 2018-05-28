#include <sys/types.h>
#include <sys/socket.h>
#include "sys_hook.h"

int get_msg_len(const struct msghdr *msg) {
    if(!msg) return -1;
    int len, i;
    len = i = 0;
    for(; i < msg->msg_iovlen; i++) {
        len += msg->msg_iov[i].iov_len;
    }
    return len;
}

int get_connect_error(int fd) {
    int error = 0;
    socklen_t error_len = sizeof(error);
    
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_len);
    return error;
}
