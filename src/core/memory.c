/**
 * @file memory.c
 * @brief Memory management system with reference counting and leak detection
 *
 * This module provides thread-safe memory allocation, reference counting,
 * and comprehensive debugging capabilities for the ZEN language runtime.
 */

#include "zen/core/memory.h"

#include <assert.h>
#include <execinfo.h>  // For call stack traces
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/* Global memory debugging state */
static bool g_debug_enabled = false;
static MemoryBlock *g_allocated_blocks = NULL;
static pthread_mutex_t g_memory_mutex = PTHREAD_MUTEX_INITIALIZER;
static MemoryStats g_memory_stats = {0};

/* Internal helper functions */
static void memory_update_stats_alloc(size_t size);
static void memory_update_stats_free(size_t size);
static void memory_add_debug_block(void *ptr, size_t size, const char *file, int line);
static void memory_remove_debug_block(void *ptr);
static MemoryBlock *memory_find_block(const void *ptr);

/* Forward declarations for debug functions */
void *memory_debug_alloc(size_t size, const char *file, int line);
void *memory_debug_realloc(void *ptr, size_t new_size, const char *file, int line);
void memory_debug_free(void *ptr, const char *file, int line);
char *memory_debug_strdup(const char *str, const char *file, int line);

/**
 * @brief Safe memory allocation with zero initialization
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 * @note Memory is zero-initialized
 */
void *memory_alloc(size_t size)
{
    if (size == 0) {
        size = 1;  // Allocate at least 1 byte for empty structs
    }

    // CRITICAL FIX: Always use calloc for zero initialization
    // The documentation promises zero-initialized memory
    void *ptr = calloc(1, size);

    if (ptr) {
        memory_update_stats_alloc(size);
        if (g_debug_enabled) {
            memory_add_debug_block(ptr, size, "unknown", 0);
        }
    }

    return ptr;
}

/**
 * @brief Safe memory reallocation
 * @param ptr Existing pointer (may be NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 * @note If ptr is NULL, behaves like memory_alloc()
 * @note If new_size is 0 and ptr is not NULL, behaves like memory_free()
 */
void *memory_realloc(void *ptr, size_t new_size)
{
    if (new_size == 0) {
        if (ptr) {
            memory_free(ptr);
        }
        return NULL;
    }

    if (!ptr) {
        return memory_alloc(new_size);
    }

    // For debugging, we need to track the old size
    size_t old_size = 0;
    if (g_debug_enabled) {
        pthread_mutex_lock(&g_memory_mutex);
        MemoryBlock *block = memory_find_block(ptr);
        if (block) {
            old_size = block->size;
        }
        pthread_mutex_unlock(&g_memory_mutex);

        // Remove debug tracking before realloc to avoid use-after-free warning
        if (old_size > 0) {
            memory_remove_debug_block(ptr);
        }
    }

    void *new_ptr = realloc(ptr, new_size);
    if (new_ptr) {
        if (g_debug_enabled) {
            memory_add_debug_block(new_ptr, new_size, "unknown", 0);
        }

        // Update statistics - realloc is conceptually a free + alloc
        if (old_size > 0) {
            // Free the old memory
            memory_update_stats_free(old_size);
        }
        // Allocate the new memory
        memory_update_stats_alloc(new_size);

        // CRITICAL FIX: Only zero new memory if we have accurate old_size from debug tracking
        // When debugging is off, old_size is 0 so don't zero memory (realloc preserves data)
        if (new_size > old_size && old_size > 0) {
            memset((char *)new_ptr + old_size, 0, new_size - old_size);
        }
    }

    return new_ptr;
}

/**
 * @brief Safe memory deallocation
 * @param ptr Pointer to free (may be NULL)
 * @note Safe to call with NULL pointer
 */
void memory_free(void *ptr)
{
    if (!ptr) {
        return;
    }

    size_t size = 0;
    bool found_in_debug = false;
    bool already_freed = false;

    // Get size from debug tracking if available and check for double-free
    if (g_debug_enabled) {
        pthread_mutex_lock(&g_memory_mutex);
        MemoryBlock *block = memory_find_block(ptr);
        if (block) {
            size = block->size;
            found_in_debug = true;
            // Check if this block was already marked as freed
            if (block->size == 0) {
                already_freed = true;
                fprintf(stderr, "WARNING: Double-free detected for pointer %p\n", ptr);
            } else {
                // Mark as freed to detect future double-frees
                block->size = 0;
            }
        }
        pthread_mutex_unlock(&g_memory_mutex);
    }

    // Prevent double-free when debugging is enabled
    if (already_freed) {
        fprintf(stderr, "ERROR: Preventing double-free of pointer %p\n", ptr);
        return;
    }

    // Remove from debug tracking before free
    if (found_in_debug && !already_freed) {
        memory_remove_debug_block(ptr);
    }

    free(ptr);

    // Update statistics with the actual freed size
    if (size > 0) {
        memory_update_stats_free(size);
    }
}

/**
 * @brief Duplicate a null-terminated string
 * @param str String to duplicate (may be NULL)
 * @return Heap-allocated copy of string, or NULL on failure/NULL input
 * @note Caller must free the returned string with memory_free()
 */
char *memory_strdup(const char *str)
{
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *copy = memory_alloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }

    return copy;
}

/* Reference counting utilities */

/**
 * @brief Increment reference count atomically
 * @param ref_count Pointer to reference count
 * @return New reference count value
 * @note Thread-safe atomic operation
 */
size_t memory_ref_inc(size_t *ref_count)
{
    if (!ref_count) {
        return 0;
    }

    return atomic_fetch_add((_Atomic size_t *)ref_count, 1) + 1;
}

/**
 * @brief Decrement reference count atomically
 * @param ref_count Pointer to reference count
 * @return New reference count value
 * @note Thread-safe atomic operation
 * @note Returns 0 when count reaches zero
 */
size_t memory_ref_dec(size_t *ref_count)
{
    if (!ref_count) {
        return 0;
    }

    size_t old_count = atomic_fetch_sub((_Atomic size_t *)ref_count, 1);
    return old_count > 0 ? old_count - 1 : 0;
}

/**
 * @brief Get reference count value atomically
 * @param ref_count Pointer to reference count
 * @return Current reference count value
 * @note Thread-safe atomic operation
 */
size_t memory_ref_get(const size_t *ref_count)
{
    if (!ref_count) {
        return 0;
    }

    return atomic_load((_Atomic size_t *)ref_count);
}

/* Memory debugging and statistics */

/**
 * @brief Enable or disable memory debugging
 * @param enable true to enable, false to disable
 * @note When enabled, tracks all allocations for leak detection
 */
void memory_debug_enable(bool enable)
{
    pthread_mutex_lock(&g_memory_mutex);
    g_debug_enabled = enable;
    pthread_mutex_unlock(&g_memory_mutex);
}

/**
 * @brief Check if memory debugging is enabled
 * @return true if debugging is enabled, false otherwise
 */
bool memory_debug_is_enabled(void)
{
    pthread_mutex_lock(&g_memory_mutex);
    bool enabled = g_debug_enabled;
    pthread_mutex_unlock(&g_memory_mutex);
    return enabled;
}

/**
 * @brief Get current memory allocation statistics
 * @param stats Pointer to MemoryStats structure to fill
 * @note Thread-safe operation
 */
void memory_get_stats(MemoryStats *stats)
{
    if (!stats) {
        return;
    }

    pthread_mutex_lock(&g_memory_mutex);
    *stats = g_memory_stats;
    pthread_mutex_unlock(&g_memory_mutex);
}

/**
 * @brief Reset memory allocation statistics
 * @note Resets counters but doesn't affect actual allocations
 */
void memory_reset_stats(void)
{
    pthread_mutex_lock(&g_memory_mutex);
    memset(&g_memory_stats, 0, sizeof(MemoryStats));
    pthread_mutex_unlock(&g_memory_mutex);
}

/**
 * @brief Print memory leak report to stderr
 * @note Only effective when memory debugging is enabled
 * @note Call before program exit to detect leaks
 */
void memory_print_leak_report(void)
{
    if (!g_debug_enabled) {
        fprintf(stderr, "Memory debugging not enabled - no leak report available\n");
        return;
    }

    pthread_mutex_lock(&g_memory_mutex);

    MemoryBlock *block = g_allocated_blocks;
    size_t leak_count = 0;
    size_t total_leaked = 0;

    if (!block) {
        fprintf(stderr, "✓ No memory leaks detected\n");
        pthread_mutex_unlock(&g_memory_mutex);
        return;
    }

    fprintf(stderr, "❌ Memory leaks detected:\n");
    fprintf(stderr, "========================================\n");

    while (block) {
        fprintf(stderr, "Leak #%zu: %zu bytes at %p", leak_count + 1, block->size, block->ptr);

        if (block->file && block->line > 0) {
            fprintf(stderr, " (allocated at %s:%d)", block->file, block->line);
        }
        fprintf(stderr, "\n");

        leak_count++;
        total_leaked += block->size;
        block = block->next;
    }

    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Total: %zu leaks, %zu bytes\n", leak_count, total_leaked);

    pthread_mutex_unlock(&g_memory_mutex);
}

/**
 * @brief Check for memory leaks
 * @return Number of leaked allocations
 * @note Only effective when memory debugging is enabled
 */
size_t memory_check_leaks(void)
{
    if (!g_debug_enabled) {
        return 0;
    }

    pthread_mutex_lock(&g_memory_mutex);

    size_t leak_count = 0;
    MemoryBlock *block = g_allocated_blocks;

    while (block) {
        leak_count++;
        block = block->next;
    }

    pthread_mutex_unlock(&g_memory_mutex);
    return leak_count;
}

/**
 * @brief Clean up memory debugging system
 * @note Should be called before program exit
 * @note Frees internal debugging structures
 */
void memory_debug_cleanup(void)
{
    pthread_mutex_lock(&g_memory_mutex);

    // Free all debug blocks
    MemoryBlock *block = g_allocated_blocks;
    while (block) {
        MemoryBlock *next = block->next;
        free(block);  // Use libc free directly for debug structures
        block = next;
    }

    g_allocated_blocks = NULL;
    g_debug_enabled = false;

    pthread_mutex_unlock(&g_memory_mutex);
}

/* Memory validation */

/**
 * @brief Validate a memory pointer
 * @param ptr Pointer to validate
 * @param expected_size Expected size (0 to skip size check)
 * @return true if pointer appears valid, false otherwise
 * @note Only works when memory debugging is enabled
 */
bool memory_validate_ptr(const void *ptr, size_t expected_size)
{
    if (!ptr) {
        return false;
    }

    if (!g_debug_enabled) {
        return true;  // Can't validate without debugging
    }

    pthread_mutex_lock(&g_memory_mutex);

    MemoryBlock *block = memory_find_block(ptr);
    bool valid = false;

    if (block) {
        valid = (expected_size == 0 || block->size == expected_size);
    }

    pthread_mutex_unlock(&g_memory_mutex);
    return valid;
}

/**
 * @brief Check if pointer is tracked by memory system
 * @param ptr Pointer to check
 * @return true if pointer is tracked, false otherwise
 * @note Only works when memory debugging is enabled
 */
bool memory_is_tracked(const void *ptr)
{
    if (!ptr || !g_debug_enabled) {
        return false;
    }

    pthread_mutex_lock(&g_memory_mutex);
    MemoryBlock *block = memory_find_block(ptr);
    pthread_mutex_unlock(&g_memory_mutex);

    return block != NULL;
}

/* Debug allocation functions (used by macros) */

/**
 * @brief Debug allocation with file and line tracking
 * @param size Number of bytes to allocate
 * @param file Source file where allocation occurs
 * @param line Source line where allocation occurs
 * @return Pointer to allocated memory, or NULL on failure
 * @note Always available, but only tracks location when debugging enabled
 */
void *memory_debug_alloc(size_t size, const char *file, int line)
{
    if (size == 0) {
        size = 1;  // Allocate at least 1 byte for empty structs
    }

    void *ptr = malloc(size);

    // Fall back to system allocation if pool failed
    if (!ptr) {
        ptr = calloc(1, size);
    }

    if (ptr) {
        memory_update_stats_alloc(size);
        if (g_debug_enabled) {
            memory_add_debug_block(ptr, size, file, line);
        }
    }

    return ptr;
}

/**
 * @brief Debug reallocation with file and line tracking
 * @param ptr Existing pointer (may be NULL)
 * @param new_size New size in bytes
 * @param file Source file where reallocation occurs
 * @param line Source line where reallocation occurs
 * @return Pointer to reallocated memory, or NULL on failure
 * @note Always available, but only tracks location when debugging enabled
 */
void *memory_debug_realloc(void *ptr, size_t new_size, const char *file, int line)
{
    if (new_size == 0) {
        if (ptr) {
            memory_debug_free(ptr, file, line);
        }
        return NULL;
    }

    if (!ptr) {
        return memory_debug_alloc(new_size, file, line);
    }

    size_t old_size = 0;
    if (g_debug_enabled) {
        pthread_mutex_lock(&g_memory_mutex);
        MemoryBlock *block = memory_find_block(ptr);
        if (block) {
            old_size = block->size;
        }
        pthread_mutex_unlock(&g_memory_mutex);
    }

    // Remove debug tracking before realloc to avoid use-after-free warning
    if (g_debug_enabled && old_size > 0) {
        memory_remove_debug_block(ptr);
    }

    void *new_ptr = realloc(ptr, new_size);
    if (new_ptr) {
        if (g_debug_enabled) {
            memory_add_debug_block(new_ptr, new_size, file, line);
        }

        // Update statistics - realloc is conceptually a free + alloc
        if (old_size > 0) {
            // Free the old memory
            memory_update_stats_free(old_size);
        }
        // Allocate the new memory
        memory_update_stats_alloc(new_size);

        // CRITICAL FIX: Only zero new memory if we have accurate old_size from debug tracking
        // When debugging is off, old_size is 0 so don't zero memory (realloc preserves data)
        if (new_size > old_size && old_size > 0) {
            memset((char *)new_ptr + old_size, 0, new_size - old_size);
        }

    } else if (g_debug_enabled && old_size > 0) {
        // Realloc failed, restore debug tracking for original pointer
        memory_add_debug_block(ptr, old_size, file, line);
    }

    return new_ptr;
}

/**
 * @brief Debug free with file and line tracking
 * @param ptr Pointer to free (may be NULL)
 * @param file Source file where free occurs
 * @param line Source line where free occurs
 * @note Always available, but only tracks location when debugging enabled
 */
void memory_debug_free(void *ptr, const char *file, int line)
{
    // Note: file and line could be used for enhanced debug tracking
    // For now, we maintain compatibility with the existing debug block system
    (void)file;
    (void)line;

    if (!ptr) {
        return;
    }

    size_t size = 0;
    bool found_in_debug = false;

    // Get size from debug tracking if available
    if (g_debug_enabled) {
        pthread_mutex_lock(&g_memory_mutex);
        MemoryBlock *block = memory_find_block(ptr);
        if (block) {
            size = block->size;
            found_in_debug = true;
        }
        pthread_mutex_unlock(&g_memory_mutex);
    }

    // Remove from debug tracking before free
    if (found_in_debug) {
        memory_remove_debug_block(ptr);
    }

    free(ptr);

    // Update statistics with the actual freed size
    if (size > 0) {
        memory_update_stats_free(size);
    }
}

/**
 * @brief Debug string duplication with file and line tracking
 * @param str String to duplicate (may be NULL)
 * @param file Source file where duplication occurs
 * @param line Source line where duplication occurs
 * @return Heap-allocated copy of string, or NULL on failure/NULL input
 * @note Always available, but only tracks location when debugging enabled
 */
char *memory_debug_strdup(const char *str, const char *file, int line)
{
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *copy = memory_debug_alloc(len, file, line);
    if (copy) {
        memcpy(copy, str, len);
    }

    return copy;
}

/* Internal helper functions */

static void memory_update_stats_alloc(size_t size)
{
    pthread_mutex_lock(&g_memory_mutex);

    g_memory_stats.total_allocated += size;
    g_memory_stats.current_allocated += size;
    g_memory_stats.allocation_count++;

    if (g_memory_stats.current_allocated > g_memory_stats.peak_allocated) {
        g_memory_stats.peak_allocated = g_memory_stats.current_allocated;
    }

    pthread_mutex_unlock(&g_memory_mutex);
}

static void memory_update_stats_free(size_t size)
{
    pthread_mutex_lock(&g_memory_mutex);

    g_memory_stats.total_freed += size;
    if (g_memory_stats.current_allocated >= size) {
        g_memory_stats.current_allocated -= size;
    }
    g_memory_stats.free_count++;

    pthread_mutex_unlock(&g_memory_mutex);
}

static void memory_add_debug_block(void *ptr, size_t size, const char *file, int line)
{
    if (!g_debug_enabled) {
        return;
    }

    MemoryBlock *block = malloc(sizeof(MemoryBlock));
    if (!block) {
        return;  // Can't track this allocation
    }

    // Initialize all fields
    memset(block, 0, sizeof(MemoryBlock));
    block->ptr = ptr;
    block->size = size;
    block->file = file;
    block->line = line;
    block->timestamp = time(NULL);
    block->age_generation = 0;  // Start as young generation
    block->mark_flag = 0;
    block->access_count = 0;
    block->last_access = block->timestamp;
    block->call_stack = NULL;

    pthread_mutex_lock(&g_memory_mutex);
    block->next = g_allocated_blocks;
    g_allocated_blocks = block;
    pthread_mutex_unlock(&g_memory_mutex);
}

static void memory_remove_debug_block(void *ptr)
{
    if (!g_debug_enabled) {
        return;
    }

    pthread_mutex_lock(&g_memory_mutex);

    MemoryBlock **current = &g_allocated_blocks;
    while (*current) {
        if ((*current)->ptr == ptr) {
            MemoryBlock *to_remove = *current;
            *current = (*current)->next;
            free(to_remove);
            break;
        }
        current = &(*current)->next;
    }

    pthread_mutex_unlock(&g_memory_mutex);
}

static MemoryBlock *memory_find_block(const void *ptr)
{
    // Caller must hold mutex
    MemoryBlock *block = g_allocated_blocks;
    while (block) {
        if (block->ptr == ptr) {
            return block;
        }
        block = block->next;
    }
    return NULL;
}
