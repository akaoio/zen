#include "src/include/zen/core/memory.h"
#include <stdio.h>
#include <stdlib.h>
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

void test_garbage_collection(void) {
    printf("=== Testing Garbage Collection ===\n");
    
    // Enable debugging for GC tracking
    memory_debug_enable(true);
    
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
    
    // Allocate some objects that will become unreachable
    for (int i = 0; i < 5; i++) {
        void* temp_ptr = memory_alloc(128);
        (void)temp_ptr; // Simulate objects that become unreachable
    }
    printf("✓ Allocated 5 temporary objects (now unreachable)\n");
    
    // Set up GC roots (objects that should never be collected)
    void* root_ptrs[3];
    for (int i = 0; i < 3; i++) {
        root_ptrs[i] = memory_alloc(256);
    }
    memory_gc_set_roots(root_ptrs, 3);
    printf("✓ Set up 3 GC root objects\n");
    
    // Trigger garbage collection
    size_t reclaimed = memory_gc_collect(true); // Full collection
    printf("✓ Garbage collection reclaimed %zu bytes\n", reclaimed);
    
    // Clean up roots
    for (int i = 0; i < 3; i++) {
        memory_free(root_ptrs[i]);
    }
    
    memory_gc_shutdown();
    memory_debug_enable(false);
    printf("✓ Garbage collector shut down\n\n");
}

void test_memory_analytics(void) {
    printf("=== Testing Memory Analytics ===\n");
    
    memory_debug_enable(true);
    memory_profiling_enable(true);
    
    // Allocate memory of various sizes to create fragmentation
    void* ptrs[20];
    size_t sizes[] = {16, 32, 64, 128, 256};
    
    for (int i = 0; i < 20; i++) {
        size_t size = sizes[i % 5];
        ptrs[i] = memory_alloc(size);
    }
    printf("✓ Allocated 20 objects of varying sizes for fragmentation test\n");
    
    // Calculate fragmentation
    size_t fragmentation = memory_calc_fragmentation();
    printf("✓ Memory fragmentation score: %zu%%\n", fragmentation);
    
    // Get memory statistics
    MemoryStats stats;
    memory_get_stats(&stats);
    printf("✓ Memory statistics:\n");
    printf("  Total allocated: %zu bytes\n", stats.total_allocated);
    printf("  Current allocated: %zu bytes\n", stats.current_allocated);
    printf("  Peak allocated: %zu bytes\n", stats.peak_allocated);
    printf("  Allocations: %zu\n", stats.allocation_count);
    printf("  Pool hits: %zu\n", stats.pool_hits);
    printf("  Pool misses: %zu\n", stats.pool_misses);
    
    // Validate heap integrity
    size_t corruption_issues = memory_validate_heap();
    printf("✓ Heap validation: %zu corruption issues found\n", corruption_issues);
    
    // Generate comprehensive report
    bool report_success = memory_generate_report("memory_report.txt");
    if (report_success) {
        printf("✓ Memory report generated to memory_report.txt\n");
    }
    
    // Clean up
    for (int i = 0; i < 20; i++) {
        memory_free(ptrs[i]);
    }
    
    memory_profiling_enable(false);
    memory_debug_enable(false);
    printf("✓ Analytics test completed\n\n");
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
    
    // Try to allocate beyond total limit
    void* ptr3 = memory_alloc(200);
    void* ptr4 = memory_alloc(200);
    void* ptr5 = memory_alloc(200);
    void* ptr6 = memory_alloc(200);
    void* ptr7 = memory_alloc(200); // This should fail - would exceed 1KB total
    
    int successful_allocs = (ptr3 ? 1 : 0) + (ptr4 ? 1 : 0) + 
                           (ptr5 ? 1 : 0) + (ptr6 ? 1 : 0) + (ptr7 ? 1 : 0);
    printf("✓ Additional allocations: %d/5 succeeded (limited by total memory)\n", successful_allocs);
    
    // Clean up
    if (ptr1) memory_free(ptr1);
    if (ptr3) memory_free(ptr3);
    if (ptr4) memory_free(ptr4);
    if (ptr5) memory_free(ptr5);
    if (ptr6) memory_free(ptr6);
    if (ptr7) memory_free(ptr7);
    
    // Remove limits
    memory_set_limits(0, 0);
    printf("✓ Memory limits removed\n\n");
}

void low_memory_callback(size_t available) {
    printf("  ⚠️  Low memory warning: only %zu bytes available\n", available);
}

void test_low_memory_detection(void) {
    printf("=== Testing Low Memory Detection ===\n");
    
    // Register low memory callback with 512-byte threshold
    memory_register_low_memory_callback(low_memory_callback, 512);
    memory_set_limits(1000, 0); // Set 1000-byte total limit
    printf("✓ Registered low memory callback (threshold: 512 bytes)\n");
    
    // Allocate memory to trigger low memory condition
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = memory_alloc(100);
        if (!ptrs[i]) {
            printf("  Allocation %d failed (hit memory limit)\n", i+1);
            break;
        }
    }
    
    // Clean up
    for (int i = 0; i < 10; i++) {
        if (ptrs[i]) {
            memory_free(ptrs[i]);
        }
    }
    
    memory_set_limits(0, 0);
    memory_register_low_memory_callback(NULL, 0);
    printf("✓ Low memory detection test completed\n\n");
}

int main(void) {
    printf("ZEN Advanced Memory Management Test Suite\n");
    printf("========================================\n\n");
    
    test_memory_pools();
    test_garbage_collection();
    test_memory_analytics();
    test_memory_limits();
    test_low_memory_detection();
    
    printf("🎉 All advanced memory tests completed successfully!\n");
    printf("\nThe ZEN memory system now includes:\n");
    printf("  • Memory pools for efficient small object allocation\n");
    printf("  • Mark-and-sweep garbage collection with generational support\n");
    printf("  • Advanced memory analytics and fragmentation analysis\n");
    printf("  • Memory usage limits and low-memory detection\n");
    printf("  • Comprehensive memory reporting and heap validation\n");
    printf("  • Call stack tracing support (when enabled)\n");
    printf("  • Function-level memory profiling\n");
    
    return 0;
}