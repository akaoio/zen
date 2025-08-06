/*
 * benchmark_runtime.c
 * Performance benchmark for ZEN runtime optimizations
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "src/include/zen/performance/runtime_optimizations.h"
#include "src/include/zen/types/value.h"

// Timing utilities
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec * 0.001;
}

// Benchmark configuration
#define NUM_ITERATIONS 10000

// Test functions
void benchmark_value_allocation(void) {
    printf("=== Value Allocation Benchmark ===\n");
    
    // Standard allocation
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* v = value_new_number((double)i);
        value_free(v);
    }
    double standard_time = get_time_ms() - start;
    
    // Optimized allocation (if available)
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
#if ZEN_ENABLE_VALUE_POOLING
        Value* v = value_new_number_pooled((double)i);
        value_free_pooled(v);
#else
        Value* v = value_new_number((double)i);
        value_free(v);
#endif
    }
    double optimized_time = get_time_ms() - start;
    
    printf("Standard allocation:  %.2f ms (%d iterations)\n", standard_time, NUM_ITERATIONS);
    printf("Optimized allocation: %.2f ms (%d iterations)\n", optimized_time, NUM_ITERATIONS);
    printf("Speedup: %.2fx\n", standard_time / optimized_time);
    printf("\n");
}

void benchmark_string_operations(void) {
    printf("=== String Operations Benchmark ===\n");
    
    const char* test_strings[] = {"Hello", "World", "ZEN", "Language", "Performance"};
    const int num_strings = sizeof(test_strings) / sizeof(test_strings[0]);
    
    // Standard string concatenation
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* a = value_new_string(test_strings[i % num_strings]);
        Value* b = value_new_string(test_strings[(i + 1) % num_strings]);
        
        // Simulate concatenation (would use op_add in real code)
        char combined[256];
        snprintf(combined, sizeof(combined), "%s%s", 
                 a->as.string ? a->as.string->data : "",
                 b->as.string ? b->as.string->data : "");
        
        Value* result = value_new_string(combined);
        
        value_free(a);
        value_free(b);
        value_free(result);
    }
    double standard_time = get_time_ms() - start;
    
    // Optimized string operations (if available)
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
#if ZEN_ENABLE_VALUE_POOLING
        Value* a = value_new_string_pooled(test_strings[i % num_strings]);
        Value* b = value_new_string_pooled(test_strings[(i + 1) % num_strings]);
        
        // Simulate optimized concatenation
        char combined[256];
        snprintf(combined, sizeof(combined), "%s%s", 
                 a->as.string ? a->as.string->data : "",
                 b->as.string ? b->as.string->data : "");
        
        Value* result = value_new_string_pooled(combined);
        
        value_free_pooled(a);
        value_free_pooled(b);
        value_free_pooled(result);
#else
        // Same as standard if optimizations not available
        Value* a = value_new_string(test_strings[i % num_strings]);
        Value* b = value_new_string(test_strings[(i + 1) % num_strings]);
        
        char combined[256];
        snprintf(combined, sizeof(combined), "%s%s", 
                 a->as.string ? a->as.string->data : "",
                 b->as.string ? b->as.string->data : "");
        
        Value* result = value_new_string(combined);
        
        value_free(a);
        value_free(b);
        value_free(result);
#endif
    }
    double optimized_time = get_time_ms() - start;
    
    printf("Standard strings:  %.2f ms (%d iterations)\n", standard_time, NUM_ITERATIONS);
    printf("Optimized strings: %.2f ms (%d iterations)\n", optimized_time, NUM_ITERATIONS);
    printf("Speedup: %.2fx\n", standard_time / optimized_time);
    printf("\n");
}

void benchmark_arithmetic_operations(void) {
    printf("=== Arithmetic Operations Benchmark ===\n");
    
    // Standard arithmetic
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* a = value_new_number((double)i);
        Value* b = value_new_number((double)(i + 1));
        
        // Would use op_add, op_multiply, etc. in real code
        Value* sum = value_new_number(a->as.number + b->as.number);
        Value* product = value_new_number(a->as.number * b->as.number);
        
        value_free(a);
        value_free(b);
        value_free(sum);
        value_free(product);
    }
    double standard_time = get_time_ms() - start;
    
    // Optimized arithmetic (if available)
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
#if ZEN_ENABLE_VALUE_POOLING
        Value* a = value_new_number_pooled((double)i);
        Value* b = value_new_number_pooled((double)(i + 1));
        
        Value* sum = value_new_number_pooled(a->as.number + b->as.number);
        Value* product = value_new_number_pooled(a->as.number * b->as.number);
        
        value_free_pooled(a);
        value_free_pooled(b);
        value_free_pooled(sum);
        value_free_pooled(product);
#else
        Value* a = value_new_number((double)i);
        Value* b = value_new_number((double)(i + 1));
        
        Value* sum = value_new_number(a->as.number + b->as.number);
        Value* product = value_new_number(a->as.number * b->as.number);
        
        value_free(a);
        value_free(b);
        value_free(sum);
        value_free(product);
#endif
    }
    double optimized_time = get_time_ms() - start;
    
    printf("Standard arithmetic:  %.2f ms (%d iterations)\n", standard_time, NUM_ITERATIONS);
    printf("Optimized arithmetic: %.2f ms (%d iterations)\n", optimized_time, NUM_ITERATIONS);
    printf("Speedup: %.2fx\n", standard_time / optimized_time);
    printf("\n");
}

int main(void) {
    printf("ZEN Runtime Performance Benchmark\n");
    printf("==================================\n\n");
    
    // Initialize optimizations
#if ZEN_ENABLE_VALUE_POOLING || ZEN_ENABLE_LITERAL_CACHING
    zen_runtime_optimizations_init();
#endif
    
    // Run benchmarks
    benchmark_value_allocation();
    benchmark_string_operations();
    benchmark_arithmetic_operations();
    
    // Print optimization statistics
#if ZEN_ENABLE_VALUE_POOLING || ZEN_ENABLE_LITERAL_CACHING
    zen_print_all_performance_stats();
#else
    printf("=== Optimization Status ===\n");
    printf("Runtime optimizations are DISABLED in this build.\n");
    printf("Enable optimizations by defining ZEN_ENABLE_* macros.\n");
#endif
    
    // Cleanup optimizations
#if ZEN_ENABLE_VALUE_POOLING || ZEN_ENABLE_LITERAL_CACHING
    zen_runtime_optimizations_cleanup();
#endif
    
    printf("\nBenchmark complete.\n");
    return 0;
}