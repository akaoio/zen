#include <stdio.h>
#include "src/include/zen/core/memory.h"

int main() {
    printf("Testing memory_alloc...\n");
    
    void* ptr = memory_alloc(64);
    if (ptr) {
        printf("memory_alloc succeeded: %p\n", ptr);
        memory_free(ptr);
        printf("memory_free completed\n");
    } else {
        printf("memory_alloc failed!\n");
        return 1;
    }
    
    return 0;
}