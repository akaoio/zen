/*
 * performance_test.c
 * Basic performance test for ZEN runtime before implementing optimizations
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "src/include/zen/types/value.h"
#include "src/include/zen/runtime/operators.h"

static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec * 0.001;
}

#define NUM_ITERATIONS 10000

void test_value_allocation(void) {
    printf("=== Value Allocation Performance ===\n");
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* v = value_new_number((double)i);
        value_free(v);
    }
    double time_taken = get_time_ms() - start;
    
    printf("Number allocation: %.2f ms (%d operations, %.4f ms/op)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
    
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* v = value_new_boolean(i % 2 == 0);
        value_free(v);
    }
    time_taken = get_time_ms() - start;
    
    printf("Boolean allocation: %.2f ms (%d operations, %.4f ms/op)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
    
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* v = value_new_string("test string");
        value_free(v);
    }
    time_taken = get_time_ms() - start;
    
    printf("String allocation: %.2f ms (%d operations, %.4f ms/op)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
}

void test_arithmetic_operations(void) {
    printf("\n=== Arithmetic Operations Performance ===\n");
    
    Value* a = value_new_number(10.0);
    Value* b = value_new_number(5.0);
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* result = op_add(a, b);
        value_free(result);
    }
    double time_taken = get_time_ms() - start;
    
    printf("Addition: %.2f ms (%d operations, %.4f ms/op)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
    
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* result = op_multiply(a, b);
        value_free(result);
    }
    time_taken = get_time_ms() - start;
    
    printf("Multiplication: %.2f ms (%d operations, %.4f ms/op)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
    
    start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* result = op_equals(a, b);
        value_free(result);
    }
    time_taken = get_time_ms() - start;
    
    printf("Equality comparison: %.2f ms (%d operations, %.4f ms/op)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
    
    value_free(a);
    value_free(b);
}

void test_string_operations(void) {
    printf("\n=== String Operations Performance ===\n");
    
    Value* a = value_new_string("Hello");
    Value* b = value_new_string("World");
    
    double start = get_time_ms();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        Value* result = op_add(a, b);
        value_free(result);
    }
    double time_taken = get_time_ms() - start;
    
    printf("String concatenation: %.2f ms (%d operations, %.4f ms/op)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
    
    value_free(a);
    value_free(b);
}

void test_mixed_workload(void) {
    printf("\n=== Mixed Workload Performance ===\n");
    
    double start = get_time_ms();
    
    for (int i = 0; i < NUM_ITERATIONS / 4; i++) {
        // Numbers
        Value* n1 = value_new_number((double)i);
        Value* n2 = value_new_number((double)(i + 1));
        Value* n_sum = op_add(n1, n2);
        Value* n_prod = op_multiply(n1, n2);
        
        // Strings
        Value* s1 = value_new_string("prefix");
        Value* s2 = value_new_string("suffix");
        Value* s_concat = op_add(s1, s2);
        
        // Booleans
        Value* b1 = value_new_boolean(i % 2 == 0);
        Value* b2 = value_new_boolean(i % 3 == 0);
        Value* b_eq = op_equals(b1, b2);
        
        // Cleanup
        value_free(n1); value_free(n2); value_free(n_sum); value_free(n_prod);
        value_free(s1); value_free(s2); value_free(s_concat);
        value_free(b1); value_free(b2); value_free(b_eq);
    }
    
    double time_taken = get_time_ms() - start;
    int total_operations = (NUM_ITERATIONS / 4) * 10; // 10 operations per iteration
    
    printf("Mixed workload: %.2f ms (%d total operations, %.4f ms/op)\n", 
           time_taken, total_operations, time_taken / total_operations);
}

void memory_pressure_test(void) {
    printf("\n=== Memory Pressure Test ===\n");
    
    // Test with many simultaneous allocations
    Value** values = malloc(NUM_ITERATIONS * sizeof(Value*));
    
    double start = get_time_ms();
    
    // Allocate all at once
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        values[i] = value_new_number((double)i);
    }
    
    // Free all at once
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        value_free(values[i]);
    }
    
    double time_taken = get_time_ms() - start;
    
    printf("Batch allocation/deallocation: %.2f ms (%d values, %.4f ms/value)\n", 
           time_taken, NUM_ITERATIONS, time_taken / NUM_ITERATIONS);
    
    free(values);
}

int main(void) {
    printf("ZEN Runtime Performance Analysis\n");
    printf("==================================\n\n");
    
    printf("Running %d iterations per test...\n\n", NUM_ITERATIONS);
    
    test_value_allocation();
    test_arithmetic_operations();
    test_string_operations();
    test_mixed_workload();
    memory_pressure_test();
    
    printf("\nPerformance analysis complete.\n");
    printf("This baseline will be used to measure optimization improvements.\n");
    
    return 0;
}