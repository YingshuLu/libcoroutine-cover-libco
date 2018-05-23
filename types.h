#ifndef STACK_TYPES_H_
#define STACK_TYPES_H_

#define true  1
#define false 0
#define MAX_ERROR_BUFFER_SIZE 1024

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

typedef int bool;

static __thread char *_error_buffer = NULL;
#define dperror(ret) do {\
    if(!_error_buffer) _error_buffer = (char *)malloc(MAX_ERROR_BUFFER_SIZE);\
    strerror_r(errno, _error_buffer, MAX_ERROR_BUFFER_SIZE);\
    printf("[Error] return value: %d, errno: %d, %s <%s>@<%s:%d>\n", ret, errno, _error_buffer, __func__, basename(__FILE__), __LINE__);\
}while(0)

#define warnf(buf) do {\
    printf("[warnning]");\
    printf("%s",buf);\
}while(0)

#endif
