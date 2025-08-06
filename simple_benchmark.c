/*
 * simple_benchmark.c
 * Simple runtime performance test for ZEN optimizations
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// Basic includes for ZEN
#include "src/include/zen/types/value.h"
#include "src/include/zen/runtime/operators.h"

// Timing utility
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec * 0.001;
}

#define NUM_ITERATIONS 1000

void benchmark_value_creation(void) {
    printf("=== Value Creation Benchmark ===\n");
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* v = value_new_number((double)i);
        value_free(v);
    }
    double time_taken = get_time_ms() - start;
    
    printf("Created and freed %d values in %.2f ms\n", NUM_ITERATIONS, time_taken);
    printf("Average: %.4f ms per value\n", time_taken / NUM_ITERATIONS);
}

void benchmark_arithmetic(void) {
    printf("\n=== Arithmetic Operations Benchmark ===\n");
    
    Value* a = value_new_number(10.0);
    Value* b = value_new_number(5.0);
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* result = op_add(a, b);
        value_free(result);
        
        result = op_multiply(a, b);
        value_free(result);
    }
    double time_taken = get_time_ms() - start;
    
    printf("Performed %d arithmetic operations in %.2f ms\n", NUM_ITERATIONS * 2, time_taken);
    printf("Average: %.4f ms per operation\n", time_taken / (NUM_ITERATIONS * 2));
    
    value_free(a);
    value_free(b);
}

void benchmark_string_operations(void) {
    printf("\n=== String Operations Benchmark ===\n");
    
    Value* a = value_new_string("Hello");
    Value* b = value_new_string("World");
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* result = op_add(a, b);
        value_free(result);
    }
    double time_taken = get_time_ms() - start;
    
    printf("Performed %d string concatenations in %.2f ms\n", NUM_ITERATIONS, time_taken);
    printf("Average: %.4f ms per concatenation\n", time_taken / NUM_ITERATIONS);
    
    value_free(a);
    value_free(b);
}

int main(void) {
    printf("ZEN Runtime Performance Benchmark\n");
    printf("==================================\n\n");
    
    benchmark_value_creation();
    benchmark_arithmetic();
    benchmark_string_operations();
    
    printf("\nBenchmark complete.\n");
    return 0;
}