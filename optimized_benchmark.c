/*
 * optimized_benchmark.c
 * Benchmark comparing standard vs optimized ZEN runtime performance
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// ZEN includes with optimization support
#include "src/include/zen/types/value.h"
#include "src/include/zen/runtime/operators.h"
#include "src/include/zen/performance/runtime_optimizations.h"

// Timing utility
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec * 0.001;
}

#define NUM_ITERATIONS 5000

// Test standard value creation
void benchmark_standard_values(void) {
    printf("=== Standard Value Operations ===\n");
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* v = value_new_number((double)i);
        value_free(v);
        
        Value* b = value_new_boolean(i % 2 == 0);
        value_free(b);
        
        Value* s = value_new_string("test");
        value_free(s);
    }
    double time_taken = get_time_ms() - start;
    
    printf("Standard approach: %.2f ms (%d operations)\n", 
           time_taken, NUM_ITERATIONS * 3);
}

// Test optimized value creation
void benchmark_optimized_values(void) {
    printf("=== Optimized Value Operations ===\n");
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
#if ZEN_ENABLE_VALUE_POOLING
        Value* v = value_new_number_pooled((double)i);
        value_free_pooled(v);
        
        Value* b = value_new_boolean_pooled(i % 2 == 0);
        value_free_pooled(b);
        
        Value* s = value_new_string_pooled("test");
        value_free_pooled(s);
#else
        Value* v = value_new_number((double)i);
        value_free(v);
        
        Value* b = value_new_boolean(i % 2 == 0);
        value_free(b);
        
        Value* s = value_new_string("test");
        value_free(s);
#endif
    }
    double time_taken = get_time_ms() - start;
    
    printf("Optimized approach: %.2f ms (%d operations)\n", 
           time_taken, NUM_ITERATIONS * 3);
}

// Test standard arithmetic
void benchmark_standard_arithmetic(void) {
    printf("\n=== Standard Arithmetic Operations ===\n");
    
    Value* a = value_new_number(10.0);
    Value* b = value_new_number(5.0);
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* result = op_add(a, b);
        value_free(result);
        
        result = op_multiply(a, b);
        value_free(result);
        
        result = op_equals(a, b);
        value_free(result);
    }
    double time_taken = get_time_ms() - start;
    
    printf("Standard arithmetic: %.2f ms (%d operations)\n", 
           time_taken, NUM_ITERATIONS * 3);
    
    value_free(a);
    value_free(b);
}

// Test optimized arithmetic
void benchmark_optimized_arithmetic(void) {
    printf("=== Optimized Arithmetic Operations ===\n");
    
#if ZEN_ENABLE_VALUE_POOLING
    Value* a = value_new_number_pooled(10.0);
    Value* b = value_new_number_pooled(5.0);
#else
    Value* a = value_new_number(10.0);
    Value* b = value_new_number(5.0);
#endif
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
#if ZEN_ENABLE_FAST_OPERATORS
        Value* result = op_add_optimized(a, b);
#if ZEN_ENABLE_VALUE_POOLING
        value_free_pooled(result);
#else
        value_free(result);
#endif
        
        result = op_multiply_optimized(a, b);
#if ZEN_ENABLE_VALUE_POOLING
        value_free_pooled(result);
#else
        value_free(result);
#endif
        
        result = op_equals_optimized(a, b);
#if ZEN_ENABLE_VALUE_POOLING
        value_free_pooled(result);
#else
        value_free(result);
#endif
#else
        Value* result = op_add(a, b);
        value_free(result);
        
        result = op_multiply(a, b);
        value_free(result);
        
        result = op_equals(a, b);
        value_free(result);
#endif
    }
    double time_taken = get_time_ms() - start;
    
    printf("Optimized arithmetic: %.2f ms (%d operations)\n", 
           time_taken, NUM_ITERATIONS * 3);
    
#if ZEN_ENABLE_VALUE_POOLING
    value_free_pooled(a);
    value_free_pooled(b);
#else
    value_free(a);
    value_free(b);
#endif
}

int main(void) {
    printf("ZEN Runtime Optimization Benchmark\n");
    printf("=====================================\n\n");
    
    // Initialize optimizations if available
#if ZEN_ENABLE_VALUE_POOLING || ZEN_ENABLE_LITERAL_CACHING
    zen_runtime_optimizations_init();
#endif
    
    // Run benchmarks
    benchmark_standard_values();
    benchmark_optimized_values();
    benchmark_standard_arithmetic();
    benchmark_optimized_arithmetic();
    
    // Print optimization statistics
    printf("\n");
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