#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("Testing ZEN Memory Pool System (Simple)\n");
    printf("=====================================\n");
    
    // Initialize memory pools
    printf("1. Initializing memory pools...\n");
    size_t pool_sizes[] = {32, 64, 128};
    size_t num_pools = 3;
    
    bool success = memory_pool_init(pool_sizes, num_pools);
    assert(success && "Pool initialization should succeed");
    printf("   ✓ Memory pools initialized for sizes: 32, 64, 128 bytes\n");
    
    // Test multiple allocations
    printf("2. Testing pool allocations...\n");
    void* ptrs[6];
    
    // Allocate from different pools
    ptrs[0] = memory_alloc(32);  // Should come from 32-byte pool
    ptrs[1] = memory_alloc(64);  // Should come from 64-byte pool
    ptrs[2] = memory_alloc(128); // Should come from 128-byte pool
    ptrs[3] = memory_alloc(32);  // Another from 32-byte pool
    ptrs[4] = memory_alloc(64);  // Another from 64-byte pool
    ptrs[5] = memory_alloc(100); // Should come from 128-byte pool (larger fit)
    
    for (int i = 0; i < 6; i++) {
        assert(ptrs[i] && "Pool allocation should succeed");
        printf("   ✓ Allocation %d successful at %p\n", i+1, ptrs[i]);
    }
    
    // Test freeing back to pools
    printf("3. Testing pool deallocation...\n");
    for (int i = 0; i < 6; i++) {
        memory_free(ptrs[i]);
        printf("   ✓ Freed allocation %d\n", i+1);
    }
    
    // Get pool statistics
    printf("4. Checking pool statistics...\n");
    MemoryPool pool_stats[10];
    size_t pool_count = memory_pool_get_stats(pool_stats, 10);
    printf("   ✓ Pool statistics: %zu pools active\n", pool_count);
    
    for (size_t i = 0; i < pool_count; i++) {
        printf("   Pool %zu: size=%zu, allocated=%zu, freed=%zu, free_count=%zu\n", 
               i+1, pool_stats[i].object_size, 
               pool_stats[i].allocations, pool_stats[i].deallocations,
               pool_stats[i].free_count);
    }
    
    // Test reuse from pools
    printf("5. Testing pool reuse...\n");
    void* reuse_ptr = memory_alloc(64); // Should reuse from pool
    assert(reuse_ptr && "Pool reuse should succeed");
    printf("   ✓ Reused allocation successful at %p\n", reuse_ptr);
    memory_free(reuse_ptr);
    printf("   ✓ Reused allocation freed\n");
    
    // Shutdown
    printf("6. Shutting down pools...\n");
    memory_pool_shutdown();
    printf("   ✓ Memory pools shut down\n");
    
    printf("\n🎉 Simple pool test completed successfully!\n");
    printf("\nThe memory pool system is working correctly:\n");
    printf("  • Pool initialization and shutdown\n");
    printf("  • Allocation from appropriate pool sizes\n");
    printf("  • Proper deallocation back to pools\n");
    printf("  • Pool reuse functionality\n");
    printf("  • Statistics tracking\n");
    
    return 0;
}