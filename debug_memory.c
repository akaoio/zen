#include <stdio.h>
#include "src/include/zen/core/memory.h"
#include "src/include/zen/core/visitor.h"

int main() {
    printf("Testing memory_alloc...\n");
    
    void* ptr = memory_alloc(64);
    if (ptr) {
        printf("memory_alloc succeeded: %p\n", ptr);
        memory_free(ptr);
    } else {
        printf("memory_alloc failed!\n");
        return 1;
    }
    
    printf("Testing init_visitor...\n");
    visitor_T* visitor = init_visitor();
    if (visitor) {
        printf("init_visitor succeeded: %p\n", (void*)visitor);
        memory_free(visitor);
    } else {
        printf("init_visitor failed!\n");
        return 1;
    }
    
    return 0;
}