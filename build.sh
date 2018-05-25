#!/bin/sh

op=$1
define="CO_INFO"
[ "$op" == "debug" ] && define="CO_DEBUG"

result=`gcc -D "$define" -g -o test thread_main.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c -I./ -ldl`
[ $? -eq 0 ] && echo "build success"
