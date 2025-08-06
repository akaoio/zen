#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("Testing only the memory pool system...\n");
    
    // Test without pools first
    printf("1. Testing regular allocation without pools...\n");
    void* ptr1 = memory_alloc(100);
    printf("   Allocated 100 bytes at %p\n", ptr1);
    memory_free(ptr1);
    printf("   Freed successfully\n");
    
    // Now test with pools
    printf("2. Initializing memory pools...\n");
    size_t pool_sizes[] = {64, 128};
    size_t num_pools = 2;
    
    bool success = memory_pool_init(pool_sizes, num_pools);
    if (!success) {
        printf("   Pool initialization failed!\n");
        return 1;
    }
    printf("   Memory pools initialized\n");
    
    // Test single pool allocation and free
    printf("3. Testing single pool allocation...\n");
    void* pool_ptr = memory_alloc(64);
    printf("   Allocated 64 bytes at %p\n", pool_ptr);
    
    printf("4. Testing pool free...\n");
    memory_free(pool_ptr);
    printf("   Freed successfully\n");
    
    // Shutdown pools
    printf("5. Shutting down pools...\n");
    memory_pool_shutdown();
    printf("   Pool shutdown complete\n");
    
    printf("Pool test completed successfully!\n");
    return 0;
}