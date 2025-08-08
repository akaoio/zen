#ifndef ZEN_CORE_MEMORY_H
#define ZEN_CORE_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/**
 * @brief Memory allocation statistics
 */
typedef struct {
    size_t total_allocated;   /**< Total bytes allocated */
    size_t total_freed;       /**< Total bytes freed */
    size_t current_allocated; /**< Currently allocated bytes */
    size_t peak_allocated;    /**< Peak allocated bytes */
    size_t allocation_count;  /**< Number of allocations */
    size_t free_count;        /**< Number of frees */

    /* Advanced memory analytics */
    size_t fragmentation_score; /**< Memory fragmentation score (0-100) */
    size_t pool_hits;           /**< Number of pool allocation hits */
    size_t pool_misses;         /**< Number of pool allocation misses */
    double avg_alloc_size;      /**< Average allocation size */
    time_t last_gc_time;        /**< Last garbage collection time */
    size_t gc_cycles;           /**< Number of GC cycles performed */
    size_t bytes_reclaimed_gc;  /**< Total bytes reclaimed by GC */
} MemoryStats;

/**
 * @brief Memory debugging information for a single allocation
 */
typedef struct MemoryBlock {
    void *ptr;                /**< Allocated pointer */
    size_t size;              /**< Size of allocation */
    const char *file;         /**< Source file where allocated */
    int line;                 /**< Source line where allocated */
    struct MemoryBlock *next; /**< Next block in linked list */

    /* Advanced tracking */
    time_t timestamp;        /**< Allocation timestamp */
    char *call_stack;        /**< Call stack trace (if enabled) */
    uint32_t age_generation; /**< GC generation (0=young, 1=old) */
    uint8_t mark_flag;       /**< Mark flag for mark-and-sweep GC */
    size_t access_count;     /**< Number of times accessed */
    time_t last_access;      /**< Last access timestamp */
} MemoryBlock;

/**
 * @brief Memory pool for specific object sizes
 */
typedef struct MemoryPool {
    size_t object_size;      /**< Size of objects in this pool */
    void **free_list;        /**< Array of free object pointers */
    size_t free_count;       /**< Number of free objects */
    size_t capacity;         /**< Total pool capacity */
    size_t allocations;      /**< Total allocations from pool */
    size_t deallocations;    /**< Total deallocations to pool */
    void *memory_chunk;      /**< Large chunk backing this pool */
    struct MemoryPool *next; /**< Next pool in linked list */
} MemoryPool;

/**
 * @brief Garbage collector configuration
 */
typedef struct {
    bool enabled;           /**< Whether GC is enabled */
    size_t young_threshold; /**< Bytes threshold for young generation GC */
    size_t old_threshold;   /**< Bytes threshold for full GC */
    double growth_factor;   /**< Heap growth factor before GC */
    time_t max_pause_ms;    /**< Maximum GC pause time in milliseconds */
    bool incremental;       /**< Enable incremental collection */
} GCConfig;

/**
 * @brief Memory profiling data
 */
typedef struct {
    const char *function_name; /**< Function name */
    size_t total_allocated;    /**< Total bytes allocated in this function */
    size_t peak_allocated;     /**< Peak bytes allocated in this function */
    size_t allocation_count;   /**< Number of allocations */
    double avg_alloc_size;     /**< Average allocation size */
} MemoryProfile;

/* Core memory allocation functions */

/**
 * @brief Safe memory allocation with zero initialization
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 * @note Memory is zero-initialized
 */
void *memory_alloc(size_t size);

/**
 * @brief Safe memory reallocation
 * @param ptr Existing pointer (may be NULL)
 * @param new_size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 * @note If ptr is NULL, behaves like memory_alloc()
 * @note If new_size is 0 and ptr is not NULL, behaves like memory_free()
 */
void *memory_realloc(void *ptr, size_t new_size);

/**
 * @brief Safe memory deallocation
 * @param ptr Pointer to free (may be NULL)
 * @note Safe to call with NULL pointer
 */
void memory_free(void *ptr);

/**
 * @brief Duplicate a null-terminated string
 * @param str String to duplicate (may be NULL)
 * @return Heap-allocated copy of string, or NULL on failure/NULL input
 * @note Caller must free the returned string with memory_free()
 */
char *memory_strdup(const char *str);

/* Reference counting utilities */

/**
 * @brief Increment reference count atomically
 * @param ref_count Pointer to reference count
 * @return New reference count value
 * @note Thread-safe atomic operation
 */
size_t memory_ref_inc(size_t *ref_count);

/**
 * @brief Decrement reference count atomically
 * @param ref_count Pointer to reference count
 * @return New reference count value
 * @note Thread-safe atomic operation
 * @note Returns 0 when count reaches zero
 */
size_t memory_ref_dec(size_t *ref_count);

/**
 * @brief Get reference count value atomically
 * @param ref_count Pointer to reference count
 * @return Current reference count value
 * @note Thread-safe atomic operation
 */
size_t memory_ref_get(const size_t *ref_count);

/* Memory debugging and statistics */

/**
 * @brief Enable or disable memory debugging
 * @param enable true to enable, false to disable
 * @note When enabled, tracks all allocations for leak detection
 */
void memory_debug_enable(bool enable);

/**
 * @brief Check if memory debugging is enabled
 * @return true if debugging is enabled, false otherwise
 */
bool memory_debug_is_enabled(void);

/**
 * @brief Get current memory allocation statistics
 * @param stats Pointer to MemoryStats structure to fill
 * @note Thread-safe operation
 */
void memory_get_stats(MemoryStats *stats);

/**
 * @brief Reset memory allocation statistics
 * @note Resets counters but doesn't affect actual allocations
 */
void memory_reset_stats(void);

/**
 * @brief Print memory leak report to stderr
 * @note Only effective when memory debugging is enabled
 * @note Call before program exit to detect leaks
 */
void memory_print_leak_report(void);

/**
 * @brief Check for memory leaks
 * @return Number of leaked allocations
 * @note Only effective when memory debugging is enabled
 */
size_t memory_check_leaks(void);

/**
 * @brief Clean up memory debugging system
 * @note Should be called before program exit
 * @note Frees internal debugging structures
 */
void memory_debug_cleanup(void);

/* Memory validation */

/**
 * @brief Validate a memory pointer
 * @param ptr Pointer to validate
 * @param expected_size Expected size (0 to skip size check)
 * @return true if pointer appears valid, false otherwise
 * @note Only works when memory debugging is enabled
 */
bool memory_validate_ptr(const void *ptr, size_t expected_size);

/**
 * @brief Check if pointer is tracked by memory system
 * @param ptr Pointer to check
 * @return true if pointer is tracked, false otherwise
 * @note Only works when memory debugging is enabled
 */
bool memory_is_tracked(const void *ptr);

/* Convenience macros for debugging */
#ifdef DEBUG_MEMORY
#define MEMORY_ALLOC(size)        memory_debug_alloc((size), __FILE__, __LINE__)
#define MEMORY_REALLOC(ptr, size) memory_debug_realloc((ptr), (size), __FILE__, __LINE__)
#define MEMORY_FREE(ptr)          memory_debug_free((ptr), __FILE__, __LINE__)
#define MEMORY_STRDUP(str)        memory_debug_strdup((str), __FILE__, __LINE__)
#else
#define MEMORY_ALLOC(size)        memory_alloc(size)
#define MEMORY_REALLOC(ptr, size) memory_realloc((ptr), (size))
#define MEMORY_FREE(ptr)          memory_free(ptr)
#define MEMORY_STRDUP(str)        memory_strdup(str)
#endif

/* Debug allocation functions (used by macros) */

/**
 * @brief Debug allocation with file and line tracking
 * @param size Number of bytes to allocate
 * @param file Source file where allocation occurs
 * @param line Source line where allocation occurs
 * @return Pointer to allocated memory, or NULL on failure
 * @note Always available, but only tracks location when debugging enabled
 */
void *memory_debug_alloc(size_t size, const char *file, int line);

/**
 * @brief Debug reallocation with file and line tracking
 * @param ptr Existing pointer (may be NULL)
 * @param new_size New size in bytes
 * @param file Source file where reallocation occurs
 * @param line Source line where reallocation occurs
 * @return Pointer to reallocated memory, or NULL on failure
 * @note If ptr is NULL, behaves like memory_debug_alloc()
 * @note If new_size is 0 and ptr is not NULL, behaves like memory_debug_free()
 */
void *memory_debug_realloc(void *ptr, size_t new_size, const char *file, int line);

/**
 * @brief Debug deallocation with file and line tracking
 * @param ptr Pointer to free (may be NULL)
 * @param file Source file where deallocation occurs
 * @param line Source line where deallocation occurs
 * @note Safe to call with NULL pointer
 */
void memory_debug_free(void *ptr, const char *file, int line);

/**
 * @brief Debug string duplication with file and line tracking
 * @param str String to duplicate (may be NULL)
 * @param file Source file where duplication occurs
 * @param line Source line where duplication occurs
 * @return Heap-allocated copy of string, or NULL on failure/NULL input
 * @note Caller must free the returned string with memory_debug_free()
 */
char *memory_debug_strdup(const char *str, const char *file, int line);

/* Advanced Memory Management Features */

/**
 * @brief Initialize memory pool system
 * @param pool_sizes Array of pool sizes to create
 * @param num_pools Number of pools to create
 * @return true on success, false on failure
 */
bool memory_pool_init(const size_t *pool_sizes, size_t num_pools);

/**
 * @brief Shutdown memory pool system
 * @note Frees all pools and their memory
 */
void memory_pool_shutdown(void);

/**
 * @brief Allocate from memory pool if available
 * @param size Size to allocate
 * @return Pointer to allocated memory, or NULL if no suitable pool
 */
void *memory_pool_alloc(size_t size);

/**
 * @brief Return memory to pool
 * @param ptr Pointer to return to pool
 * @param size Size of the allocation
 * @return true if returned to pool, false if not pooled
 */
bool memory_pool_free(void *ptr, size_t size);

/**
 * @brief Get memory pool statistics
 * @param pool_stats Array to fill with pool statistics
 * @param max_pools Maximum number of pools to report
 * @return Number of pools reported
 */
size_t memory_pool_get_stats(MemoryPool *pool_stats, size_t max_pools);

/* Garbage Collection */

/**
 * @brief Initialize garbage collector
 * @param config GC configuration
 * @return true on success, false on failure
 */
bool memory_gc_init(const GCConfig *config);

/**
 * @brief Shutdown garbage collector
 */
void memory_gc_shutdown(void);

/**
 * @brief Trigger garbage collection
 * @param full_collection true for full collection, false for young generation only
 * @return Number of bytes reclaimed
 */
size_t memory_gc_collect(bool full_collection);

/**
 * @brief Set GC root pointers (objects that should never be collected)
 * @param roots Array of root pointers
 * @param count Number of root pointers
 */
void memory_gc_set_roots(void **roots, size_t count);

/**
 * @brief Mark object as reachable (for mark-and-sweep)
 * @param ptr Pointer to mark
 */
void memory_gc_mark(void *ptr);

/* Memory Analytics and Profiling */

/**
 * @brief Enable detailed memory profiling
 * @param enable true to enable, false to disable
 * @note Adds overhead but provides detailed allocation tracking
 */
void memory_profiling_enable(bool enable);

/**
 * @brief Get memory allocation profile for function
 * @param function_name Name of function to profile
 * @param profile Pointer to profile structure to fill
 * @return true if profile found, false otherwise
 */
bool memory_get_function_profile(const char *function_name, MemoryProfile *profile);

/**
 * @brief Calculate memory fragmentation score
 * @return Fragmentation score (0-100, higher = more fragmented)
 */
size_t memory_calc_fragmentation(void);

/**
 * @brief Generate comprehensive memory report
 * @param filename File to write report to (NULL for stdout)
 * @return true on success, false on failure
 */
bool memory_generate_report(const char *filename);

/**
 * @brief Start memory usage monitoring session
 * @param session_name Name for the monitoring session
 * @return Session ID for stopping monitoring
 */
uint64_t memory_start_monitoring(const char *session_name);

/**
 * @brief Stop memory usage monitoring session
 * @param session_id Session ID from memory_start_monitoring
 * @return true on success, false if session not found
 */
bool memory_stop_monitoring(uint64_t session_id);

/**
 * @brief Enable call stack tracking for allocations
 * @param enable true to enable, false to disable
 * @param max_depth Maximum call stack depth to capture
 * @note Significant performance overhead when enabled
 */
void memory_enable_stack_trace(bool enable, int max_depth);

/**
 * @brief Validate entire heap for corruption
 * @return Number of corruption issues found (0 = healthy heap)
 */
size_t memory_validate_heap(void);

/**
 * @brief Set memory usage limits
 * @param max_total_bytes Maximum total memory usage (0 = no limit)
 * @param max_single_alloc Maximum single allocation size (0 = no limit)
 * @return true on success, false on failure
 */
bool memory_set_limits(size_t max_total_bytes, size_t max_single_alloc);

/**
 * @brief Register callback for low memory conditions
 * @param callback Function to call when memory is low
 * @param threshold Threshold bytes for low memory condition
 */
void memory_register_low_memory_callback(void (*callback)(size_t available), size_t threshold);

/* Advanced debugging macros */
#ifdef DEBUG_MEMORY_ADVANCED
#define MEMORY_PROFILE_FUNC()     memory_profile_function(__func__)
#define MEMORY_CHECK_CORRUPTION() memory_validate_heap()
#define MEMORY_TRACE_ALLOC(ptr, size)                                                              \
    memory_trace_allocation((ptr), (size), __FILE__, __LINE__, __func__)
#else
#define MEMORY_PROFILE_FUNC()                                                                      \
    do {                                                                                           \
    } while (0)
#define MEMORY_CHECK_CORRUPTION() (0)
#define MEMORY_TRACE_ALLOC(ptr, size)                                                              \
    do {                                                                                           \
    } while (0)
#endif

/* Internal functions for advanced debugging */

/**
 * @brief Profile memory usage for a specific function
 * @param function_name Name of the function to profile
 * @note Used internally for memory profiling and debugging
 */
void memory_profile_function(const char *function_name);

/**
 * @brief Trace memory allocation with location information
 * @param ptr Allocated memory pointer
 * @param size Size of allocation in bytes
 * @param file Source file where allocation occurred
 * @param line Line number where allocation occurred
 * @param function Function name where allocation occurred
 * @note Used internally for advanced memory debugging and tracing
 */
void memory_trace_allocation(
    void *ptr, size_t size, const char *file, int line, const char *function);

#endif /* ZEN_CORE_MEMORY_H */