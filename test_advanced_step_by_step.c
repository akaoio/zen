#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <assert.h>

void test_memory_pools_only(void) {
    printf("=== Testing Memory Pool System Only ===\n");
    
    // Initialize memory pools for common sizes
    size_t pool_sizes[] = {32, 64, 128, 256, 512};
    size_t num_pools = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    
    bool success = memory_pool_init(pool_sizes, num_pools);
    assert(success && "Pool initialization should succeed");
    printf("✓ Memory pools initialized for sizes: 32, 64, 128, 256, 512 bytes\n");
    
    // Test pool allocations
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = memory_alloc(64); // Should come from 64-byte pool
        assert(ptrs[i] && "Pool allocation should succeed");
    }
    printf("✓ Allocated 10 objects from 64-byte pool\n");
    
    // Free some objects back to pool
    for (int i = 0; i < 5; i++) {
        memory_free(ptrs[i]);
    }
    printf("✓ Returned 5 objects to pool\n");
    
    // Get pool statistics
    MemoryPool pool_stats[10];
    size_t pool_count = memory_pool_get_stats(pool_stats, 10);
    printf("✓ Pool statistics: %zu pools active\n", pool_count);
    
    for (size_t i = 0; i < pool_count; i++) {
        printf("  Pool %zu: size=%zu, allocated=%zu, free=%zu\n", 
               i+1, pool_stats[i].object_size, 
               pool_stats[i].allocations, pool_stats[i].free_count);
    }
    
    // Clean up remaining allocations
    for (int i = 5; i < 10; i++) {
        memory_free(ptrs[i]);
    }
    
    memory_pool_shutdown();
    printf("✓ Memory pools shut down\n\n");
}

void test_gc_init_only(void) {
    printf("=== Testing GC Initialization Only ===\n");
    
    // Configure garbage collector
    GCConfig gc_config = {
        .enabled = true,
        .young_threshold = 1024,
        .old_threshold = 4096,
        .growth_factor = 1.5,
        .max_pause_ms = 10,
        .incremental = false
    };
    
    bool success = memory_gc_init(&gc_config);
    assert(success && "GC initialization should succeed");
    printf("✓ Garbage collector initialized\n");
    
    memory_gc_shutdown();
    printf("✓ Garbage collector shut down\n\n");
}

int main(void) {
    printf("ZEN Advanced Memory Step-by-Step Test\n");
    printf("====================================\n\n");
    
    printf("Step 1: Memory pools...\n");
    test_memory_pools_only();
    
    printf("Step 2: Garbage collection init...\n");
    test_gc_init_only();
    
    printf("🎉 Step-by-step test completed successfully!\n");
    return 0;
}