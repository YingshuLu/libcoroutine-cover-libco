#!/bin/sh

op=$1
define="CO_INFO"
[ "$op" == "debug" ] && define="CO_DEBUG"

result=`gcc -D "$define" -g -o test_client client.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c -I./ -ldl`

[ $? -eq 0 ] && echo "build client success"

gcc -D "$define" -g -o test_server server.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c -I./ -ldl
[ $? -eq 0 ] && echo "build server success"

gcc -D "$define" -g -o co_test co_main.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c -I./ -ldl
[ $? -eq 0 ] && echo "build co_test success"

gcc -D "$define" -g -o co_spec co_spec.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c -I./ -ldl
[ $? -eq 0 ] && echo "build co_spec success"
