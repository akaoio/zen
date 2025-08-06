#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/include/zen/core/memory.h"

void test_basic_allocation() {
    printf("Testing basic allocation...\n");
    
    // Test memory_alloc
    void* ptr1 = memory_alloc(100);
    assert(ptr1 != NULL);
    
    // Test memory_realloc
    void* ptr2 = memory_realloc(ptr1, 200);
    assert(ptr2 != NULL);
    
    // Test memory_strdup
    char* str = memory_strdup("Hello, World!");
    assert(str != NULL);
    assert(strcmp(str, "Hello, World!") == 0);
    
    // Test memory_free
    memory_free(ptr2);
    memory_free(str);
    
    printf("✓ Basic allocation tests passed\n");
}

void test_debug_allocation() {
    printf("Testing debug allocation...\n");
    
    // Enable debugging
    memory_debug_enable(true);
    
    // Test debug allocation functions
    void* ptr1 = memory_debug_alloc(150, __FILE__, __LINE__);
    assert(ptr1 != NULL);
    
    void* ptr2 = memory_debug_realloc(ptr1, 300, __FILE__, __LINE__);
    assert(ptr2 != NULL);
    
    char* str = memory_debug_strdup("Debug Test", __FILE__, __LINE__);
    assert(str != NULL);
    assert(strcmp(str, "Debug Test") == 0);
    
    // Free everything
    memory_debug_free(ptr2, __FILE__, __LINE__);
    memory_debug_free(str, __FILE__, __LINE__);
    
    // Check for leaks
    size_t leaks = memory_check_leaks();
    printf("Detected %zu memory leaks\n", leaks);
    
    memory_debug_enable(false);
    
    printf("✓ Debug allocation tests passed\n");
}

void test_memory_pools() {
    printf("Testing memory pools...\n");
    
    // Initialize memory pools
    size_t pool_sizes[] = {32, 64, 128, 256};
    bool init_success = memory_pool_init(pool_sizes, 4);
    assert(init_success);
    
    // Allocate from pools
    void* ptr1 = memory_alloc(32);  // Should use pool
    void* ptr2 = memory_alloc(64);  // Should use pool
    void* ptr3 = memory_alloc(1000); // Should use regular allocation
    
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(ptr3 != NULL);
    
    // Free back to pools
    memory_free(ptr1);
    memory_free(ptr2);
    memory_free(ptr3);
    
    // Get pool statistics
    MemoryPool pool_stats[4];
    size_t num_pools = memory_pool_get_stats(pool_stats, 4);
    printf("Number of pools: %zu\n", num_pools);
    
    for (size_t i = 0; i < num_pools; i++) {
        printf("Pool %zu: size=%zu, allocations=%zu, deallocations=%zu\n",
               i, pool_stats[i].object_size, 
               pool_stats[i].allocations, pool_stats[i].deallocations);
    }
    
    memory_pool_shutdown();
    
    printf("✓ Memory pool tests passed\n");
}

void test_memory_limits() {
    printf("Testing memory limits...\n");
    
    // Reset statistics first to have a clean slate
    memory_reset_stats();
    
    // Set memory limits - give more room for total limit
    bool limit_success = memory_set_limits(2048, 512);
    assert(limit_success);
    
    // Try allocation within limits
    void* ptr1 = memory_alloc(256);
    if (ptr1 == NULL) {
        MemoryStats stats;
        memory_get_stats(&stats);
        printf("DEBUG: Allocation failed. Current allocated: %zu, trying to allocate: 256\n", 
               stats.current_allocated);
        printf("DEBUG: Total limit: 2048, Single limit: 512\n");
    }
    assert(ptr1 != NULL);
    
    // Try allocation exceeding single limit
    void* ptr2 = memory_alloc(600);
    if (ptr2 != NULL) {
        printf("DEBUG: Large allocation succeeded when it should have failed\n");
        memory_free(ptr2);
    }
    assert(ptr2 == NULL); // Should fail
    
    // Clean up
    memory_free(ptr1);
    
    // Reset limits
    memory_set_limits(0, 0);
    
    printf("✓ Memory limit tests passed\n");
}

void test_reference_counting() {
    printf("Testing reference counting...\n");
    
    size_t ref_count = 1;
    
    // Test increment
    size_t new_count = memory_ref_inc(&ref_count);
    assert(new_count == 2);
    assert(memory_ref_get(&ref_count) == 2);
    
    // Test decrement
    new_count = memory_ref_dec(&ref_count);
    assert(new_count == 1);
    assert(memory_ref_get(&ref_count) == 1);
    
    new_count = memory_ref_dec(&ref_count);
    assert(new_count == 0);
    assert(memory_ref_get(&ref_count) == 0);
    
    printf("✓ Reference counting tests passed\n");
}

void test_memory_statistics() {
    printf("Testing memory statistics...\n");
    
    // Reset statistics
    memory_reset_stats();
    
    MemoryStats stats;
    memory_get_stats(&stats);
    
    printf("Initial stats - allocated: %zu, freed: %zu, count: %zu\n",
           stats.total_allocated, stats.total_freed, stats.allocation_count);
    
    // Do some allocations
    void* ptr1 = memory_alloc(100);
    void* ptr2 = memory_alloc(200);
    
    memory_get_stats(&stats);
    printf("After allocs - allocated: %zu, count: %zu\n",
           stats.total_allocated, stats.allocation_count);
    
    // Free them
    memory_free(ptr1);
    memory_free(ptr2);
    
    memory_get_stats(&stats);
    printf("After frees - allocated: %zu, freed: %zu, current: %zu\n",
           stats.total_allocated, stats.total_freed, stats.current_allocated);
    
    printf("✓ Memory statistics tests passed\n");
}

int main() {
    printf("=== Testing Memory Management System ===\n\n");
    
    test_basic_allocation();
    test_debug_allocation();
    test_memory_pools();
    test_memory_limits();
    test_reference_counting();
    test_memory_statistics();
    
    // Generate a memory report
    printf("\n=== Memory Report ===\n");
    memory_generate_report(NULL);
    
    // Final cleanup
    memory_debug_cleanup();
    
    printf("\n🎉 All memory management tests passed!\n");
    return 0;
}