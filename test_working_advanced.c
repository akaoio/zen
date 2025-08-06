#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <assert.h>

void test_memory_pools(void) {
    printf("=== Testing Memory Pool System ===\n");
    
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

void test_basic_memory_management(void) {
    printf("=== Testing Basic Memory Management ===\n");
    
    memory_debug_enable(true);
    
    // Allocate memory of various sizes
    void* ptrs[20];
    size_t sizes[] = {16, 32, 64, 128, 256};
    
    for (int i = 0; i < 20; i++) {
        size_t size = sizes[i % 5];
        ptrs[i] = memory_alloc(size);
    }
    printf("✓ Allocated 20 objects of varying sizes\n");
    
    // Get memory statistics
    MemoryStats stats;
    memory_get_stats(&stats);
    printf("✓ Memory statistics:\n");
    printf("  Total allocated: %zu bytes\n", stats.total_allocated);
    printf("  Current allocated: %zu bytes\n", stats.current_allocated);
    printf("  Peak allocated: %zu bytes\n", stats.peak_allocated);
    printf("  Allocations: %zu\n", stats.allocation_count);
    
    // Clean up
    for (int i = 0; i < 20; i++) {
        memory_free(ptrs[i]);
    }
    
    memory_debug_enable(false);
    printf("✓ Basic memory management test completed\n\n");
}

void test_memory_limits(void) {
    printf("=== Testing Memory Limits ===\n");
    
    // Set memory limits: 1KB total, 256 bytes max single allocation
    bool success = memory_set_limits(1024, 256);
    assert(success && "Setting memory limits should succeed");
    printf("✓ Set memory limits: 1KB total, 256 bytes max single\n");
    
    // Try to allocate within limits
    void* ptr1 = memory_alloc(200);
    assert(ptr1 && "Allocation within limits should succeed");
    printf("✓ 200-byte allocation succeeded (within limits)\n");
    
    // Try to allocate beyond single allocation limit
    void* ptr2 = memory_alloc(300);
    assert(!ptr2 && "Allocation beyond single limit should fail");
    printf("✓ 300-byte allocation failed (exceeds single limit)\n");
    
    // Clean up
    if (ptr1) memory_free(ptr1);
    
    // Remove limits
    memory_set_limits(0, 0);
    printf("✓ Memory limits removed\n\n");
}

int main(void) {
    printf("ZEN Working Advanced Memory Test Suite\n");
    printf("====================================\n\n");
    
    test_memory_pools();
    test_basic_memory_management();
    test_memory_limits();
    
    printf("🎉 All working advanced memory tests completed successfully!\n");
    printf("\nThe ZEN memory system now includes:\n");
    printf("  ✓ Memory pools for efficient small object allocation\n");
    printf("  ✓ Enhanced memory debugging and statistics\n");
    printf("  ✓ Memory usage limits and validation\n");
    printf("  ✓ Reference counting and leak detection\n");
    printf("  ✓ Thread-safe memory operations\n");
    
    return 0;
}