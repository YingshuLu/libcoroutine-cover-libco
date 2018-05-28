#!/bin/sh

argc=$#
op="install"
define="CO_RELEASE"
[ $argc -gt 0 ] && op="$1"
[ $argc -gt 1 ] && [ "$2" == "debug" ] && define="CO_DEBUG"

if [ "$op" == "test" ]
then
    define="CO_DEBUG -g"

    gcc -D "$define"  -o test/test_client test/client.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c sys_helper.c -I./ -ldl
    [ $? -eq 0 ] && echo "build client success"

    gcc -D "$define"  -o test/test_server test/server.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c sys_helper.c -I./ -ldl
    [ $? -eq 0 ] && echo "build server success"

    gcc -D "$define"  -o test/co_test test/co_main.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c sys_helper.c -I./ -ldl
    [ $? -eq 0 ] && echo "build co_test success"

    gcc -D "$define"  -o test/co_spec test/co_spec.c task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c sys_helper.c -I./ -ldl
    [ $? -eq 0 ] && echo "build co_spec success"

elif [ "$op" == "install" ]
then
    gcc -D "$define" -fPIC -shared  -o libcask.so task_pool.c coroutine.c event.c inner_fd.c run_stacks.c  time_wheel.c array.c list.c task.c  thread_env.c sys_hook.c sys_helper.c -I./ -ldl
    [ $? -eq 0 ] && echo "build libcask.so success"
    [ ! -d "include" ] && mkdir include
    cp *.h include

elif [ "$op" == "clear" ] 
then
     [ -d ./include ] && rm -fr "include"
     [ -f "libcask.so" ] && rm -f "libcask.so"
     rm -f test/co_test test/co_spec test/test_server test/test_client
fi

