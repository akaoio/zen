#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("Testing basic enhanced memory system...\n");
    
    // Test basic allocation without pools first
    printf("Testing basic allocation...\n");
    void* ptr1 = memory_alloc(100);
    printf("Allocated 100 bytes at %p\n", ptr1);
    
    void* ptr2 = memory_alloc(200);
    printf("Allocated 200 bytes at %p\n", ptr2);
    
    printf("Freeing first allocation...\n");
    memory_free(ptr1);
    printf("Freed first allocation\n");
    
    printf("Freeing second allocation...\n");
    memory_free(ptr2);
    printf("Freed second allocation\n");
    
    printf("Basic memory test completed successfully!\n");
    return 0;
}