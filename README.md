
This implement for linux coroutine library is stackfull asymmetric coroutine.

You can call any coroutine instance in any corotine.

The library optionally support to automatically free coroutines if they are ran out their function. (using -D AUTO_FREE_COROUTINE)

As library uses shared-stack for coroutine, the max length is 128KB, only make sure any coroutine's stack can not over 128KB.

--------------
-----build----
--------------

$./build


