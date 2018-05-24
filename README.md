Libcask is a linux-based, multiple coroutines, analog synchronous network programming framework. 
It adopts MIT-License for free source codes.
The implementment of coroutine is a shared stackfull asymmetric coroutine.

Thanks for the inspires of libco and libgo, libcask implements follows features:
1. libcask analog synchornous style with hooking system call APIs of socket.
2. colock and cocond are performed for coroutine-level sync.
3. with shared stacks, ten thousand of coroutines can be easily created and ran. 
4. more addition, libcask ensure any coroutines can be created, called, freed in any coroutines,
   if the shared-stacks are set big enough (64KB by default).
5. coroutine pool are performed to avoid careless coroutines' call/free paths.

To build and run a test:

$./build.sh
$./test



