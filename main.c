#include "thread_env.h"

int main() {
    init_thread_env();

    event_loop(get_thread_epoll());
    destory_thread_env();
    return 0;
}
