#include "thread_env.h"
#include "task_pool.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "types.h"
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sys_hook.h"

task_pool_t *g_pool = NULL;
void client(void* ip, void *op) {
    co_enable_hook();
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sock_addr;
    socklen_t addr_len = sizeof(sock_addr);
    bzero(&sock_addr, addr_len);

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(8848);
    sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    int ret = connect(sockfd, (struct sockaddr *)&sock_addr, addr_len);
    if(ret) {
        dperror(ret);
        return;
    }

    char* request = "GET / HTTP/1.1\r\nHost: www.tigerso.com\r\n\r\n";
    size_t len = strlen(request);
    while(len) {
        ret = write(sockfd, request, strlen(request));
        if(ret < 0) {
            dperror(ret);
            close(sockfd);
            return;
        }
        len -= ret;
    }
    DBG_LOG("send request:\n%s\n", request);

    char* response = (char *) malloc(65536);
    bzero(response, 65536);
    ret = read(sockfd, response, 65536);
    if(ret < 0) {
        dperror(ret);
        return;
    }

    DBG_LOG("recv response:\n%s\n", response);
    free(response);
    close(sockfd);
    return;
}

int main() {
    g_pool = new_task_pool(-1);
    init_thread_env();

    task_t *t = get_task_from_pool(g_pool, client, NULL, NULL, NULL);
    co_resume(t);
   
    t = get_task_from_pool(g_pool, client, NULL, NULL, NULL);
    co_resume(t);

    t = get_task_from_pool(g_pool, client, NULL, NULL, NULL);
    co_resume(t);

    event_loop(current_thread_epoll());
    destory_thread_env();
    delete_task_pool(g_pool);
    return 0;
}
