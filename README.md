

This lib aims to implements features in libco (tencent opensource).

From the practical uses, I found libco has many defects, like:

1. DO not support cross-tasks stack variants, as all tasks share one running stack, if a task hold a data pointer from
anther one, it will have a wrong value.
2. Task local storage only supports global key.
3. Not hook accept, seems tencent uses libco in client side.
4. Shared stack consumes lots of cpu resources when task switching.
5. Libco manually use assembly codes to simplify the ucontext primitives, but it seems support tencent Opensuse platform and little
another linux platform.


This libcoroutine is a try to find out the secert of libco and resolve or relief some defects:

1. Use ucontext primitives of glibc for common usages (can consider boost context to replace it)
2. Add more run stacks to reduce the number of memory-copy on task switching.
3. Hook accept API and UDP sendmsg/recvmsg.
... ...

Description as below:

libcoroutine is a linux-based, multiple coroutines, analog synchronous network programming framework.
it adopts MIT-License for free source codes.                                                    
the implementment of coroutine is a shared stackfull asymmetric coroutine.                      
                                                                                                
thanks for the inspires of libco and libgo, libcoroutine implements follows features:                
1. it analog synchornous style with hooking system call APIs of socket.                    
2. colock and cocond are performed for coroutine-level sync.                                    
3. with shared stacks, ten thousand of coroutines can be easily created and ran.                
4. libcoroutine ensures any coroutines can be created, called, freed in any coroutines,              
   if the shared-stacks are set big enough (64KB by default).                                   
5. coroutine pool is performed to avoid careless coroutines' call/free paths.                   
                                                                                                
To build test examples:                                                                         
$ ./build.sh test                                                                               
                                                                                                
example binary files are in test dir.                                                           
                                                                                                
To build a so library:                                                                          
$ ./build install                                                                               
                                                                                                
copy include/ and so to your project.    
