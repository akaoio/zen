/*
 * value_pool.c
 * Memory pools for optimized value allocation
 */

#include "../include/zen/performance/runtime_optimizations.h"
#include "../include/zen/types/value.h"
#include "../include/zen/core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#if ZEN_ENABLE_VALUE_POOLING

// Pool configuration
#define POOL_SIZE_NUMBERS 1000
#define POOL_SIZE_BOOLEANS 10
#define POOL_SIZE_STRINGS 500
#define SMALL_STRING_MAX_SIZE 64

// Pre-cached common values range
#define CACHE_NUMBER_MIN 0
#define CACHE_NUMBER_MAX 19

// Memory pool structure
typedef struct {
    Value* pool;
    size_t capacity;
    size_t available;
    size_t allocated;
    size_t deallocated;
    pthread_mutex_t mutex;
} ValuePool;

// Small string pool entry
typedef struct small_string_entry {
    Value* value;
    bool in_use;
    struct small_string_entry* next;
} SmallStringEntry;

// Global pools
static ValuePool number_pool = {0};
static ValuePool boolean_pool = {0};
static SmallStringEntry* string_pool_head = NULL;
static size_t string_pool_size = 0;
static size_t string_pool_hits = 0;
static size_t string_pool_misses = 0;

// Pre-cached common values
static Value* cached_numbers[CACHE_NUMBER_MAX - CACHE_NUMBER_MIN + 1] = {NULL};
static Value* cached_true = NULL;
static Value* cached_false = NULL;
static Value* cached_null = NULL;

// Mutex for thread-safe access
static pthread_mutex_t string_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// Statistics
static size_t total_pool_hits = 0;
static size_t total_pool_misses = 0;

/**
 * @brief Initialize a value pool
 */
static void init_value_pool(ValuePool* pool, size_t capacity, ValueType type) {
    pool->pool = calloc(capacity, sizeof(Value));
    if (!pool->pool) {
        fprintf(stderr, "Failed to allocate value pool\n");
        return;
    }
    
    pool->capacity = capacity;
    pool->available = capacity;
    pool->allocated = 0;
    pool->deallocated = 0;
    
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize pool mutex\n");
        free(pool->pool);
        pool->pool = NULL;
        return;
    }
    
    // Pre-initialize pool values
    for (size_t i = 0; i < capacity; i++) {
        pool->pool[i].type = type;
        pool->pool[i].ref_count = 0;
        // Mark as available by setting ref_count to 0
    }
}

/**
 * @brief Initialize all value pools
 */
static void __attribute__((constructor)) init_value_pools(void) {
    init_value_pool(&number_pool, POOL_SIZE_NUMBERS, VALUE_NUMBER);
    init_value_pool(&boolean_pool, POOL_SIZE_BOOLEANS, VALUE_BOOLEAN);
    
    // Initialize cached common values
    pthread_mutex_lock(&cache_mutex);
    
    // Cache common numbers (0-19)
    for (int i = CACHE_NUMBER_MIN; i <= CACHE_NUMBER_MAX; i++) {
        cached_numbers[i - CACHE_NUMBER_MIN] = calloc(1, sizeof(Value));
        if (cached_numbers[i - CACHE_NUMBER_MIN]) {
            cached_numbers[i - CACHE_NUMBER_MIN]->type = VALUE_NUMBER;
            cached_numbers[i - CACHE_NUMBER_MIN]->as.number = (double)i;
            cached_numbers[i - CACHE_NUMBER_MIN]->ref_count = 1; // Never free
        }
    }
    
    // Cache boolean values
    cached_true = calloc(1, sizeof(Value));
    if (cached_true) {
        cached_true->type = VALUE_BOOLEAN;
        cached_true->as.boolean = true;
        cached_true->ref_count = 1;
    }
    
    cached_false = calloc(1, sizeof(Value));
    if (cached_false) {
        cached_false->type = VALUE_BOOLEAN;
        cached_false->as.boolean = false;
        cached_false->ref_count = 1;
    }
    
    // Cache null value
    cached_null = calloc(1, sizeof(Value));
    if (cached_null) {
        cached_null->type = VALUE_NULL;
        cached_null->ref_count = 1;
    }
    
    pthread_mutex_unlock(&cache_mutex);
}

/**
 * @brief Cleanup value pools
 */
static void __attribute__((destructor)) cleanup_value_pools(void) {
    if (number_pool.pool) {
        free(number_pool.pool);
        pthread_mutex_destroy(&number_pool.mutex);
    }
    
    if (boolean_pool.pool) {
        free(boolean_pool.pool);
        pthread_mutex_destroy(&boolean_pool.mutex);
    }
    
    // Cleanup string pool
    pthread_mutex_lock(&string_pool_mutex);
    SmallStringEntry* current = string_pool_head;
    while (current) {
        SmallStringEntry* next = current->next;
        if (current->value) {
            free(current->value);
        }
        free(current);
        current = next;
    }
    pthread_mutex_unlock(&string_pool_mutex);
    
    // Cleanup cached values (except permanent ones)
    pthread_mutex_lock(&cache_mutex);
    // Note: We don't free cached values as they might still be referenced
    pthread_mutex_unlock(&cache_mutex);
}

/**
 * @brief Get value from pool
 */
static Value* get_from_pool(ValuePool* pool) {
    if (!pool->pool) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    
    Value* result = NULL;
    for (size_t i = 0; i < pool->capacity; i++) {
        if (pool->pool[i].ref_count == 0) {
            result = &pool->pool[i];
            result->ref_count = 1;
            pool->available--;
            pool->allocated++;
            break;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    
    if (result) {
        total_pool_hits++;
    } else {
        total_pool_misses++;
    }
    
    return result;
}

/**
 * @brief Return value to pool
 */
static void return_to_pool(ValuePool* pool, Value* value) {
    if (!pool->pool || !value) return;
    
    // Check if value is from this pool
    if (value < pool->pool || value >= pool->pool + pool->capacity) {
        return; // Not from this pool
    }
    
    pthread_mutex_lock(&pool->mutex);
    
    value->ref_count = 0;
    // Clear value data
    memset(&value->as, 0, sizeof(value->as));
    
    pool->available++;
    pool->deallocated++;
    
    pthread_mutex_unlock(&pool->mutex);
}

/**
 * @brief Create number value using memory pool
 */
Value* value_new_number_pooled(double number) {
    // Check for cached common numbers first
    if (number >= CACHE_NUMBER_MIN && number <= CACHE_NUMBER_MAX && 
        number == (int)number) { // Is integer
        int index = (int)number - CACHE_NUMBER_MIN;
        if (index >= 0 && index < (CACHE_NUMBER_MAX - CACHE_NUMBER_MIN + 1)) {
            pthread_mutex_lock(&cache_mutex);
            Value* cached = cached_numbers[index];
            if (cached) {
                cached->ref_count++; // Increment reference for caller
                pthread_mutex_unlock(&cache_mutex);
                return cached;
            }
            pthread_mutex_unlock(&cache_mutex);
        }
    }
    
    // Try to get from pool
    Value* value = get_from_pool(&number_pool);
    if (value) {
        value->type = VALUE_NUMBER;
        value->as.number = number;
        return value;
    }
    
    // Fallback to regular allocation
    return value_new_number(number);
}

/**
 * @brief Create boolean value using memory pool
 */
Value* value_new_boolean_pooled(bool boolean_val) {
    // Return cached boolean values
    pthread_mutex_lock(&cache_mutex);
    Value* cached = boolean_val ? cached_true : cached_false;
    if (cached) {
        cached->ref_count++;
        pthread_mutex_unlock(&cache_mutex);
        return cached;
    }
    pthread_mutex_unlock(&cache_mutex);
    
    // Try to get from pool
    Value* value = get_from_pool(&boolean_pool);
    if (value) {
        value->type = VALUE_BOOLEAN;
        value->as.boolean = boolean_val;
        return value;
    }
    
    // Fallback to regular allocation
    return value_new_boolean(boolean_val);
}

/**
 * @brief Create string value using small string pool
 */
Value* value_new_string_pooled(const char* str) {
    if (!str) return value_new_string(str);
    
    size_t len = strlen(str);
    
    // Only pool small strings
    if (len >= SMALL_STRING_MAX_SIZE) {
        string_pool_misses++;
        return value_new_string(str);
    }
    
    // Look for existing string in pool
    pthread_mutex_lock(&string_pool_mutex);
    
    SmallStringEntry* current = string_pool_head;
    while (current) {
        if (!current->in_use && current->value && 
            current->value->as.string && 
            strcmp(current->value->as.string->data, str) == 0) {
            // Found matching string
            current->in_use = true;
            current->value->ref_count = 1;
            string_pool_hits++;
            pthread_mutex_unlock(&string_pool_mutex);
            return current->value;
        }
        current = current->next;
    }
    
    // Create new string pool entry
    if (string_pool_size < POOL_SIZE_STRINGS) {
        SmallStringEntry* entry = calloc(1, sizeof(SmallStringEntry));
        if (entry) {
            entry->value = value_new_string(str);
            if (entry->value) {
                entry->in_use = true;
                entry->next = string_pool_head;
                string_pool_head = entry;
                string_pool_size++;
                string_pool_hits++;
                pthread_mutex_unlock(&string_pool_mutex);
                return entry->value;
            }
            free(entry);
        }
    }
    
    pthread_mutex_unlock(&string_pool_mutex);
    
    // Fallback to regular allocation
    string_pool_misses++;
    return value_new_string(str);
}

/**
 * @brief Check if value was allocated from pool
 */
bool value_is_pooled(const Value* value) {
    if (!value) return false;
    
    // Check if in number pool
    if (value >= number_pool.pool && 
        value < number_pool.pool + number_pool.capacity) {
        return true;
    }
    
    // Check if in boolean pool
    if (value >= boolean_pool.pool && 
        value < boolean_pool.pool + boolean_pool.capacity) {
        return true;
    }
    
    // Check if cached value
    pthread_mutex_lock(&cache_mutex);
    for (int i = 0; i < (CACHE_NUMBER_MAX - CACHE_NUMBER_MIN + 1); i++) {
        if (value == cached_numbers[i]) {
            pthread_mutex_unlock(&cache_mutex);
            return true;
        }
    }
    if (value == cached_true || value == cached_false || value == cached_null) {
        pthread_mutex_unlock(&cache_mutex);
        return true;
    }
    pthread_mutex_unlock(&cache_mutex);
    
    // Check if in string pool
    pthread_mutex_lock(&string_pool_mutex);
    SmallStringEntry* current = string_pool_head;
    while (current) {
        if (value == current->value) {
            pthread_mutex_unlock(&string_pool_mutex);
            return true;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&string_pool_mutex);
    
    return false;
}

/**
 * @brief Free value back to pool or use regular free
 */
void value_free_pooled(Value* value) {
    if (!value) return;
    
    // Check if it's a cached permanent value
    pthread_mutex_lock(&cache_mutex);
    for (int i = 0; i < (CACHE_NUMBER_MAX - CACHE_NUMBER_MIN + 1); i++) {
        if (value == cached_numbers[i]) {
            // Just decrement ref count, never actually free
            if (value->ref_count > 0) value->ref_count--;
            pthread_mutex_unlock(&cache_mutex);
            return;
        }
    }
    if (value == cached_true || value == cached_false || value == cached_null) {
        if (value->ref_count > 0) value->ref_count--;
        pthread_mutex_unlock(&cache_mutex);
        return;
    }
    pthread_mutex_unlock(&cache_mutex);
    
    // Try to return to number pool
    if (value->type == VALUE_NUMBER) {
        return_to_pool(&number_pool, value);
        return;
    }
    
    // Try to return to boolean pool
    if (value->type == VALUE_BOOLEAN) {
        return_to_pool(&boolean_pool, value);
        return;
    }
    
    // Handle string pool
    if (value->type == VALUE_STRING) {
        pthread_mutex_lock(&string_pool_mutex);
        SmallStringEntry* current = string_pool_head;
        while (current) {
            if (value == current->value) {
                current->in_use = false;
                pthread_mutex_unlock(&string_pool_mutex);
                return;
            }
            current = current->next;
        }
        pthread_mutex_unlock(&string_pool_mutex);
    }
    
    // Not pooled, use regular free
    value_free(value);
}

/**
 * @brief Print value pool statistics
 */
void value_pool_print_stats(void) {
    printf("=== Value Pool Statistics ===\n");
    
    printf("Number Pool:\n");
    printf("  Capacity: %zu\n", number_pool.capacity);
    printf("  Available: %zu\n", number_pool.available);
    printf("  Allocated: %zu\n", number_pool.allocated);
    printf("  Deallocated: %zu\n", number_pool.deallocated);
    
    printf("Boolean Pool:\n");
    printf("  Capacity: %zu\n", boolean_pool.capacity);
    printf("  Available: %zu\n", boolean_pool.available);
    printf("  Allocated: %zu\n", boolean_pool.allocated);
    printf("  Deallocated: %zu\n", boolean_pool.deallocated);
    
    printf("String Pool:\n");
    printf("  Size: %zu\n", string_pool_size);
    printf("  Hits: %zu\n", string_pool_hits);
    printf("  Misses: %zu\n", string_pool_misses);
    
    printf("Overall:\n");
    printf("  Total Pool Hits: %zu\n", total_pool_hits);
    printf("  Total Pool Misses: %zu\n", total_pool_misses);
    
    if (total_pool_hits + total_pool_misses > 0) {
        double hit_rate = (double)total_pool_hits / (total_pool_hits + total_pool_misses) * 100.0;
        printf("  Hit Rate: %.2f%%\n", hit_rate);
    }
}

/**
 * @brief Reset value pool statistics
 */
void value_pool_reset_stats(void) {
    pthread_mutex_lock(&number_pool.mutex);
    number_pool.allocated = 0;
    number_pool.deallocated = 0;
    pthread_mutex_unlock(&number_pool.mutex);
    
    pthread_mutex_lock(&boolean_pool.mutex);
    boolean_pool.allocated = 0;
    boolean_pool.deallocated = 0;
    pthread_mutex_unlock(&boolean_pool.mutex);
    
    pthread_mutex_lock(&string_pool_mutex);
    string_pool_hits = 0;
    string_pool_misses = 0;
    pthread_mutex_unlock(&string_pool_mutex);
    
    total_pool_hits = 0;
    total_pool_misses = 0;
}

#endif // ZEN_ENABLE_VALUE_POOLING