#include <stdlib.h>
#include "hook.h"

int main() {
    enable_hook();    
    char* str = (char *)malloc(12);
    str[0] = '1';
    str[1] = '2';
    str[2] = '3';
    str[3] = '\0';
    disable_hook();
    printf("str=%s\n", str);
    free(str);
    return 0;
}
