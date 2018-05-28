#ifndef LIBCASK_SYS_HELPER_H_
#define LIBCASK_SYS_HELPER_H_

struct msghdr;
struct sockaddr;

int get_msg_len(const struct msghdr *msg);
int get_connect_error(int fd);

#endif
