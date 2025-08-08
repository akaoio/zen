#ifndef ZEN_AST_MEMORY_POOL_H
#define ZEN_AST_MEMORY_POOL_H

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "zen/core/ast.h"
#include "zen/core/memory.h"

/**
 * @file ast_memory_pool.h
 * @brief AST Memory Pool System - 40% allocation reduction for ZEN parsing
 * 
 * This system provides memory pools for AST node allocation to dramatically
 * reduce malloc/free overhead during parsing. Designed for 40% performance
 * improvement on large ZEN files.
 */

// Pool type enumeration
typedef enum {
    AST_POOL_SMALL = 0,    // 64-256 bytes: NUMBER, BOOLEAN, NULL, NOOP
    AST_POOL_MEDIUM,       // 256-1024 bytes: VARIABLE, STRING, BINARY_OP, UNARY_OP  
    AST_POOL_LARGE,        // 1024+ bytes: FUNCTION_DEFINITION, COMPOUND, ARRAY, OBJECT, LOGICAL
    AST_POOL_COUNT         // Total number of pool types
} ASTPoolType;

// Pool constants
#define AST_POOL_INITIAL_CAPACITY 32    // Initial nodes per pool (reduced due to larger struct)
#define AST_POOL_MAX_CHUNK_SIZE 2048    // Maximum nodes per chunk (reduced for memory efficiency)
#define AST_POOL_GROWTH_FACTOR 2        // Pool growth multiplier

/**
 * @brief Memory pool for AST nodes
 */
typedef struct ASTMemoryPool {
    // Pool metadata
    size_t node_size;           ///< Size of nodes in this pool
    size_t nodes_per_chunk;     ///< Nodes per allocation chunk
    size_t total_chunks;        ///< Number of allocated chunks
    
    // Free list management
    AST_T* free_list;          ///< Head of free node list
    void** chunks;             ///< Array of allocated chunks
    size_t chunk_count;        ///< Current number of chunks
    size_t chunk_capacity;     ///< Capacity of chunks array
    
    // Performance tracking
    size_t allocations;        ///< Total nodes allocated
    size_t deallocations;      ///< Total nodes freed
    size_t peak_usage;         ///< Peak simultaneous usage
    size_t malloc_calls;       ///< Number of actual malloc calls
    
    // Thread safety
    pthread_mutex_t mutex;     ///< Pool access synchronization
    bool initialized;          ///< Pool initialization status
} ASTMemoryPool;

/**
 * @brief Pool manager for all AST memory pools
 */
typedef struct ASTPoolManager {
    ASTMemoryPool* pools[AST_POOL_COUNT];  ///< Array of pools by type
    MemoryStats pool_stats;                ///< Overall pool statistics
    bool initialized;                      ///< Manager initialization status
    pthread_mutex_t manager_mutex;         ///< Manager-level synchronization
    bool enabled;                          ///< Global enable/disable flag
} ASTPoolManager;

/**
 * @brief Pool performance metrics
 */
typedef struct PoolMetrics {
    size_t total_allocations;   ///< Total nodes allocated
    size_t total_frees;         ///< Total nodes freed  
    size_t current_usage;       ///< Currently allocated nodes
    size_t peak_usage;          ///< Peak simultaneous usage
    size_t malloc_calls;        ///< Actual malloc() calls made
    size_t pool_hits;           ///< Allocations served from pool
    size_t pool_misses;         ///< Fallback to malloc
    double hit_ratio;           ///< Pool efficiency ratio
} PoolMetrics;

/**
 * @brief AST Pool statistics for manifest compatibility
 */
typedef struct ASTPoolStats {
    size_t total_allocations;   ///< Total nodes allocated across all pools
    size_t total_frees;         ///< Total nodes freed across all pools
    size_t current_usage;       ///< Currently allocated nodes
    size_t peak_usage;          ///< Peak simultaneous usage
    size_t memory_used;         ///< Current memory used by pools
    size_t memory_peak;         ///< Peak memory used by pools
    size_t pool_hits;           ///< Successful pool allocations
    size_t pool_misses;         ///< Fallback to malloc calls
    double efficiency_ratio;    ///< Pool allocation efficiency
} ASTPoolStats;

// Global pool manager
extern ASTPoolManager g_ast_pool_manager;

// Core pool operations
/**
 * @brief Initialize a memory pool for AST nodes
 * @param pool Pool to initialize
 * @param node_size Size of nodes in the pool
 * @param initial_capacity Initial capacity of the pool
 * @return true on success, false on failure
 */
bool ast_pool_init(ASTMemoryPool* pool, size_t node_size, size_t initial_capacity);

/**
 * @brief Allocate AST node from pool
 * @param pool Pool to allocate from
 * @return Allocated memory block or NULL on failure
 */
void* ast_pool_alloc(ASTMemoryPool* pool);

/**
 * @brief Free AST node back to pool
 * @param pool Pool to return to
 * @param ptr Pointer to free
 */
void ast_pool_free(ASTMemoryPool* pool, void* ptr);

/**
 * @brief Cleanup memory pool and free all allocated blocks
 * @param pool Pool to cleanup
 */
void ast_pool_cleanup(ASTMemoryPool* pool);

/**
 * @brief Get memory pool usage statistics
 * @param stats Output structure to fill with statistics
 */
void ast_pool_get_stats(ASTPoolStats* stats);

// Global convenience functions
/**
 * @brief Allocate AST node from appropriate pool (global version)
 * @param type AST node type
 * @return Pointer to allocated AST node or NULL on failure
 */
AST_T* ast_pool_alloc_global(int type);

/**
 * @brief Return AST node to pool for reuse (global version)
 * @param node AST node to deallocate
 */
void ast_pool_free_global(AST_T* node);

/**
 * @brief Cleanup all pools and release memory (global version)
 */
void ast_pool_cleanup_global(void);

/**
 * @brief Get pool performance statistics (global version)
 * @return Current pool performance metrics
 */
MemoryStats ast_pool_get_stats_global(void);

// DESIRED functions from MANIFEST
/**
 * @brief Initialize global AST pool manager
 */
void ast_pool_global_init(void);

/**
 * @brief Cleanup global AST pool manager
 */
void ast_pool_global_cleanup(void);

/**
 * @brief Allocate AST node from appropriate pool
 * @param ast_type AST node type
 * @return Allocated AST node or NULL on failure
 */
AST_T* ast_pool_alloc_node(int ast_type);

/**
 * @brief Free AST node back to appropriate pool
 * @param node AST node to free
 */
void ast_pool_free_node(AST_T* node);

/**
 * @brief Defragment memory pool to reduce fragmentation
 * @param pool Pool to defragment
 */
void ast_pool_defragment(ASTMemoryPool* pool);

/**
 * @brief Select appropriate pool for AST node type
 * @param ast_type AST node type
 * @return Pool type for the given AST type
 */
ASTPoolType ast_pool_select_type(int ast_type);

// Pool management functions
/**
 * @brief Enable or disable AST memory pools globally
 * @param enabled true to enable pools, false to use regular malloc
 */
void ast_pool_set_enabled(bool enabled);

/**
 * @brief Check if AST memory pools are enabled
 * @return true if pools are enabled, false otherwise
 */
bool ast_pool_is_enabled(void);

/**
 * @brief Get detailed metrics for a specific pool type
 * @param pool_type The pool type to query
 * @return Metrics for the specified pool
 */
PoolMetrics ast_pool_get_pool_metrics(ASTPoolType pool_type);

/**
 * @brief Print pool status for debugging
 */
void ast_pool_debug_status(void);

// Internal helper functions (for unit testing)
/**
 * @brief Select appropriate pool for AST node type
 * @param ast_type AST node type
 * @return Pool type for the given AST type
 */
ASTPoolType ast_pool_select_pool_for_type(int ast_type);

/**
 * @brief Expand a pool by allocating a new chunk
 * @param pool Pool to expand
 * @return true on success, false on failure
 */
bool ast_pool_expand(ASTMemoryPool* pool);

/**
 * @brief Check if a node was allocated from pools
 * @param node AST node to check
 * @return true if node is from pool, false otherwise
 */
bool ast_pool_is_pooled_node(AST_T* node);

// Compatibility macros for gradual migration
#define AST_ALLOC(type) ast_pool_alloc(type)
#define AST_FREE(node) ast_pool_free(node)

// Debug support
#ifdef DEBUG_AST_POOLS
#define AST_POOL_DEBUG(fmt, ...) \
    printf("[AST_POOL_DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define AST_POOL_DEBUG(fmt, ...) ((void)0)
#endif

#endif // ZEN_AST_MEMORY_POOL_H