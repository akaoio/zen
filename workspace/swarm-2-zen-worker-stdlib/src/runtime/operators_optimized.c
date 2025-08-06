/*
 * operators_optimized.c
 * Optimized operator implementations with string buffer pooling
 */

#include "../include/zen/performance/runtime_optimizations.h"
#include "../include/zen/runtime/operators.h"
#include "../include/zen/types/value.h"
#include "../include/zen/core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#if ZEN_ENABLE_FAST_OPERATORS

// String buffer pool for efficient concatenation
#define STRING_BUFFER_POOL_SIZE 100
#define MAX_STRING_BUFFER_SIZE 1024

typedef struct string_buffer {
    char* data;
    size_t capacity;
    bool in_use;
    struct string_buffer* next;
} StringBuffer;

static StringBuffer* buffer_pool_head = NULL;
static size_t buffer_pool_size = 0;
static size_t buffer_pool_hits = 0;
static size_t buffer_pool_misses = 0;
static pthread_mutex_t buffer_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// Performance metrics
static size_t fast_path_additions = 0;
static size_t fast_path_multiplications = 0;
static size_t fast_path_comparisons = 0;
static size_t string_concatenations = 0;
static pthread_mutex_t metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Initialize string buffer pool
 */
static void __attribute__((constructor)) init_string_buffer_pool(void) {
    // Pre-allocate string buffers
    for (int i = 0; i < STRING_BUFFER_POOL_SIZE; i++) {
        StringBuffer* buffer = calloc(1, sizeof(StringBuffer));
        if (buffer) {
            buffer->data = malloc(MAX_STRING_BUFFER_SIZE);
            if (buffer->data) {
                buffer->capacity = MAX_STRING_BUFFER_SIZE;
                buffer->in_use = false;
                buffer->next = buffer_pool_head;
                buffer_pool_head = buffer;
                buffer_pool_size++;
            } else {
                free(buffer);
            }
        }
    }
}

/**
 * @brief Cleanup string buffer pool
 */
static void __attribute__((destructor)) cleanup_string_buffer_pool(void) {
    pthread_mutex_lock(&buffer_pool_mutex);
    
    StringBuffer* current = buffer_pool_head;
    while (current) {
        StringBuffer* next = current->next;
        if (current->data) {
            free(current->data);
        }
        free(current);
        current = next;
    }
    
    buffer_pool_head = NULL;
    buffer_pool_size = 0;
    
    pthread_mutex_unlock(&buffer_pool_mutex);
}

/**
 * @brief Get string buffer from pool
 */
static StringBuffer* get_string_buffer(void) {
    pthread_mutex_lock(&buffer_pool_mutex);
    
    StringBuffer* buffer = NULL;
    StringBuffer* current = buffer_pool_head;
    
    while (current) {
        if (!current->in_use && current->data) {
            buffer = current;
            buffer->in_use = true;
            buffer_pool_hits++;
            break;
        }
        current = current->next;
    }
    
    if (!buffer) {
        buffer_pool_misses++;
    }
    
    pthread_mutex_unlock(&buffer_pool_mutex);
    
    return buffer;
}

/**
 * @brief Return string buffer to pool
 */
static void return_string_buffer(StringBuffer* buffer) {
    if (!buffer) return;
    
    pthread_mutex_lock(&buffer_pool_mutex);
    buffer->in_use = false;
    // Clear the buffer
    if (buffer->data) {
        buffer->data[0] = '\0';
    }
    pthread_mutex_unlock(&buffer_pool_mutex);
}

/**
 * @brief Optimized addition with string pooling and fast paths
 */
Value* op_add_optimized(Value* a, Value* b) {
    if (!a || !b) return NULL;
    
    // Fast path for number + number
    if (a->type == VALUE_NUMBER && b->type == VALUE_NUMBER) {
        pthread_mutex_lock(&metrics_mutex);
        fast_path_additions++;
        pthread_mutex_unlock(&metrics_mutex);
        
#if ZEN_ENABLE_VALUE_POOLING
        return value_new_number_pooled(a->as.number + b->as.number);
#else
        return value_new_number(a->as.number + b->as.number);
#endif
    }
    
    // Optimized string concatenation
    if (a->type == VALUE_STRING || b->type == VALUE_STRING) {
        pthread_mutex_lock(&metrics_mutex);
        string_concatenations++;
        pthread_mutex_unlock(&metrics_mutex);
        
        StringBuffer* buffer = get_string_buffer();
        if (buffer) {
            // Use pooled buffer for concatenation
            const char* a_str = (a->type == VALUE_STRING && a->as.string) ? 
                               a->as.string->data : value_to_string(a);
            const char* b_str = (b->type == VALUE_STRING && b->as.string) ? 
                               b->as.string->data : value_to_string(b);
            
            if (a_str && b_str) {
                size_t total_len = strlen(a_str) + strlen(b_str) + 1;
                
                if (total_len <= buffer->capacity) {
                    // Use pooled buffer
                    strcpy(buffer->data, a_str);
                    strcat(buffer->data, b_str);
                    
#if ZEN_ENABLE_VALUE_POOLING
                    Value* result = value_new_string_pooled(buffer->data);
#else
                    Value* result = value_new_string(buffer->data);
#endif
                    
                    return_string_buffer(buffer);
                    
                    // Clean up temporary strings if they were created
                    if (a->type != VALUE_STRING) free((void*)a_str);
                    if (b->type != VALUE_STRING) free((void*)b_str);
                    
                    return result;
                }
            }
            
            return_string_buffer(buffer);
            
            // Clean up temporary strings
            if (a->type != VALUE_STRING) free((void*)a_str);
            if (b->type != VALUE_STRING) free((void*)b_str);
        }
        
        // Fallback to regular string concatenation
        return op_add(a, b);
    }
    
    // Fall back to regular operator for other types
    return op_add(a, b);
}

/**
 * @brief Optimized subtraction with fast paths for numbers
 */
Value* op_subtract_optimized(Value* a, Value* b) {
    if (!a || !b) return NULL;
    
    // Fast path for number - number
    if (a->type == VALUE_NUMBER && b->type == VALUE_NUMBER) {
        pthread_mutex_lock(&metrics_mutex);
        fast_path_additions++; // Reuse counter for arithmetic
        pthread_mutex_unlock(&metrics_mutex);
        
#if ZEN_ENABLE_VALUE_POOLING
        return value_new_number_pooled(a->as.number - b->as.number);
#else
        return value_new_number(a->as.number - b->as.number);
#endif
    }
    
    // Fall back to regular operator
    return op_subtract(a, b);
}

/**
 * @brief Optimized multiplication with fast paths
 */
Value* op_multiply_optimized(Value* a, Value* b) {
    if (!a || !b) return NULL;
    
    // Fast path for number * number
    if (a->type == VALUE_NUMBER && b->type == VALUE_NUMBER) {
        pthread_mutex_lock(&metrics_mutex);
        fast_path_multiplications++;
        pthread_mutex_unlock(&metrics_mutex);
        
        double result = a->as.number * b->as.number;
        
        // Check for special cases that can be optimized
        if (result == 0.0 || result == 1.0) {
#if ZEN_ENABLE_VALUE_POOLING
            return value_new_number_pooled(result);
#else
            return value_new_number(result);
#endif
        }
        
#if ZEN_ENABLE_VALUE_POOLING
        return value_new_number_pooled(result);
#else
        return value_new_number(result);
#endif
    }
    
    // Fall back to regular operator
    return op_multiply(a, b);
}

/**
 * @brief Optimized equality comparison
 */
Value* op_equals_optimized(Value* a, Value* b) {
    if (!a || !b) return NULL;
    
    pthread_mutex_lock(&metrics_mutex);
    fast_path_comparisons++;
    pthread_mutex_unlock(&metrics_mutex);
    
    // Fast path for same type comparisons
    if (a->type == b->type) {
        bool result = false;
        
        switch (a->type) {
            case VALUE_NUMBER:
                result = (a->as.number == b->as.number);
                break;
            case VALUE_BOOLEAN:
                result = (a->as.boolean == b->as.boolean);
                break;
            case VALUE_NULL:
                result = true; // null == null
                break;
            case VALUE_STRING:
                if (a->as.string && b->as.string && 
                    a->as.string->data && b->as.string->data) {
                    result = (strcmp(a->as.string->data, b->as.string->data) == 0);
                } else {
                    result = (a->as.string == b->as.string);
                }
                break;
            default:
                // Fall back to regular comparison for complex types
                return op_equals(a, b);
        }
        
#if ZEN_ENABLE_VALUE_POOLING
        return value_new_boolean_pooled(result);
#else
        return value_new_boolean(result);
#endif
    }
    
    // Fall back to regular operator for mixed types
    return op_equals(a, b);
}

/**
 * @brief Cleanup operator optimizations
 */
void operators_cleanup_optimizations(void) {
    pthread_mutex_lock(&metrics_mutex);
    
    fast_path_additions = 0;
    fast_path_multiplications = 0;
    fast_path_comparisons = 0;
    string_concatenations = 0;
    
    pthread_mutex_unlock(&metrics_mutex);
    
    pthread_mutex_lock(&buffer_pool_mutex);
    buffer_pool_hits = 0;
    buffer_pool_misses = 0;
    pthread_mutex_unlock(&buffer_pool_mutex);
}

/**
 * @brief Print operator optimization statistics
 */
void operators_print_stats(void) {
    printf("=== Operator Optimization Statistics ===\n");
    
    pthread_mutex_lock(&metrics_mutex);
    printf("Fast Path Additions: %zu\n", fast_path_additions);
    printf("Fast Path Multiplications: %zu\n", fast_path_multiplications);
    printf("Fast Path Comparisons: %zu\n", fast_path_comparisons);
    printf("String Concatenations: %zu\n", string_concatenations);
    pthread_mutex_unlock(&metrics_mutex);
    
    pthread_mutex_lock(&buffer_pool_mutex);
    printf("String Buffer Pool:\n");
    printf("  Size: %zu\n", buffer_pool_size);
    printf("  Hits: %zu\n", buffer_pool_hits);
    printf("  Misses: %zu\n", buffer_pool_misses);
    
    if (buffer_pool_hits + buffer_pool_misses > 0) {
        double hit_rate = (double)buffer_pool_hits / (buffer_pool_hits + buffer_pool_misses) * 100.0;
        printf("  Hit Rate: %.2f%%\n", hit_rate);
    }
    pthread_mutex_unlock(&buffer_pool_mutex);
}

#endif // ZEN_ENABLE_FAST_OPERATORS