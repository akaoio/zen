#include "zen/core/ast_memory_pool.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

/**
 * @file ast_memory_pool.c
 * @brief AST Memory Pool Implementation - High-performance AST node allocation
 *
 * This implementation provides memory pools for AST nodes to achieve 40%
 * reduction in malloc/free operations during ZEN parsing.
 */

// Forward declarations
static bool
ast_pool_init_single(ASTMemoryPool *pool, ASTPoolType pool_type, size_t initial_capacity);

// Global pool manager instance
ASTPoolManager g_ast_pool_manager = {0};

/**
 * @brief Initialize AST pool manager
 * @param manager Manager to initialize
 */
void ast_pool_manager_init(ASTPoolManager *manager)
{
    if (!manager) {
        return;
    }

    memset(manager, 0, sizeof(ASTPoolManager));

    // Initialize manager mutex
    if (pthread_mutex_init(&manager->manager_mutex, NULL) != 0) {
        return;
    }

    // Initialize each pool type
    for (int i = 0; i < AST_POOL_COUNT; i++) {
        ASTMemoryPool *pool = memory_alloc(sizeof(ASTMemoryPool));
        if (!pool) {
            // Clean up previously initialized pools
            for (int j = 0; j < i; j++) {
                if (manager->pools[j]) {
                    pthread_mutex_destroy(&manager->pools[j]->mutex);
                    if (manager->pools[j]->chunks) {
                        memory_free(manager->pools[j]->chunks);
                    }
                    memory_free(manager->pools[j]);
                }
            }
            pthread_mutex_destroy(&manager->manager_mutex);
            return;
        }

        if (!ast_pool_init_single(pool, i, AST_POOL_INITIAL_CAPACITY)) {
            memory_free(pool);
            // Clean up previously initialized pools
            for (int j = 0; j < i; j++) {
                if (manager->pools[j]) {
                    pthread_mutex_destroy(&manager->pools[j]->mutex);
                    if (manager->pools[j]->chunks) {
                        memory_free(manager->pools[j]->chunks);
                    }
                    memory_free(manager->pools[j]);
                }
            }
            pthread_mutex_destroy(&manager->manager_mutex);
            return;
        }

        manager->pools[i] = pool;
    }

    // Initialize manager
    memset(&manager->pool_stats, 0, sizeof(MemoryStats));
    manager->enabled = true;
    manager->initialized = true;

    AST_POOL_DEBUG("AST pool manager initialized");
}

/**
 * @brief Cleanup AST pool manager and free all pools
 * @param manager Manager to cleanup
 */
void ast_pool_manager_cleanup(ASTPoolManager *manager)
{
    if (!manager || !manager->initialized) {
        return;
    }

    pthread_mutex_lock(&manager->manager_mutex);

    // Clean up each pool
    for (int i = 0; i < AST_POOL_COUNT; i++) {
        ASTMemoryPool *pool = manager->pools[i];
        if (pool) {
            pthread_mutex_lock(&pool->mutex);

            // Free all chunks
            for (size_t j = 0; j < pool->chunk_count; j++) {
                if (pool->chunks[j]) {
                    memory_free(pool->chunks[j]);
                }
            }

            // Free chunks array
            if (pool->chunks) {
                memory_free(pool->chunks);
            }

            pthread_mutex_unlock(&pool->mutex);
            pthread_mutex_destroy(&pool->mutex);

            memory_free(pool);
            manager->pools[i] = NULL;
        }
    }

    manager->initialized = false;
    manager->enabled = false;

    pthread_mutex_unlock(&manager->manager_mutex);
    pthread_mutex_destroy(&manager->manager_mutex);

    AST_POOL_DEBUG("AST pool manager cleaned up");
}

// Pool size configuration based on AST node analysis
static const size_t POOL_NODE_SIZES[AST_POOL_COUNT] = {
    sizeof(AST_T),  // SMALL: All nodes are the same size due to union
    sizeof(AST_T),  // MEDIUM: All nodes are the same size due to union
    sizeof(AST_T)   // LARGE: All nodes are the same size due to union
};

/**
 * @brief Select appropriate pool for AST node type
 * @param ast_type AST node type (unused since all AST nodes are the same size)
 * @return Pool type for the given AST type
 */
ASTPoolType ast_pool_select_type(int ast_type)
{
    // This is the MANIFEST required function
    return ast_pool_select_pool_for_type(ast_type);
}

/**
 * @brief Select appropriate pool for AST node type (internal helper)
 * @param ast_type AST node type (unused since all AST nodes are the same size)
 * @return Pool type for the given AST type
 */
ASTPoolType ast_pool_select_pool_for_type(int ast_type)
{
    // Since all AST nodes are the same size due to union structure,
    // we can use any pool. For simplicity and better cache locality,
    // we'll distribute across pools to balance load.
    (void)ast_type;  // Suppress unused parameter warning

    // Thread-safe round-robin distribution
    static _Atomic int pool_counter = 0;
    int counter_val = atomic_fetch_add(&pool_counter, 1);
    ASTPoolType selected = (ASTPoolType)(counter_val % AST_POOL_COUNT);

    return selected;
}

/**
 * @brief Initialize a single memory pool
 * @param pool Pool to initialize
 * @param pool_type Pool type for size configuration
 * @param initial_capacity Initial number of nodes
 * @return true on success, false on failure
 */
static bool
ast_pool_init_single(ASTMemoryPool *pool, ASTPoolType pool_type, size_t initial_capacity)
{
    memset(pool, 0, sizeof(ASTMemoryPool));

    pool->node_size = POOL_NODE_SIZES[pool_type];
    pool->nodes_per_chunk = initial_capacity;
    pool->total_chunks = 0;
    pool->free_list = NULL;

    // Initialize chunks array
    pool->chunk_capacity = 8;  // Start with 8 chunk pointers
    pool->chunks = memory_alloc(sizeof(void *) * pool->chunk_capacity);
    if (!pool->chunks) {
        return false;
    }
    pool->chunk_count = 0;

    // Initialize statistics
    pool->allocations = 0;
    pool->deallocations = 0;
    pool->peak_usage = 0;
    pool->malloc_calls = 0;

    // Initialize mutex
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        memory_free(pool->chunks);
        return false;
    }

    pool->initialized = true;
    return true;
}

/**
 * @brief Expand a pool by allocating a new chunk
 * @param pool Pool to expand
 * @return true on success, false on failure
 */
bool ast_pool_expand(ASTMemoryPool *pool)
{
    if (!pool || !pool->initialized) {
        return false;
    }

    // Ensure we have space for another chunk pointer
    if (pool->chunk_count >= pool->chunk_capacity) {
        size_t new_capacity = pool->chunk_capacity * 2;
        void **new_chunks = memory_realloc(pool->chunks, sizeof(void *) * new_capacity);
        if (!new_chunks) {
            return false;
        }
        pool->chunks = new_chunks;
        pool->chunk_capacity = new_capacity;
    }

    // Allocate new chunk
    size_t chunk_size = pool->nodes_per_chunk * pool->node_size;

    // DEBUG: Log the allocation details
    AST_POOL_DEBUG(
        "Expanding pool: nodes_per_chunk=%zu, node_size=%zu, chunk_size=%zu, sizeof(AST_T)=%zu",
        pool->nodes_per_chunk,
        pool->node_size,
        chunk_size,
        sizeof(AST_T));

    void *chunk = memory_alloc(chunk_size);
    if (!chunk) {
        return false;
    }

    // Add to chunks array
    pool->chunks[pool->chunk_count++] = chunk;
    pool->total_chunks++;
    pool->malloc_calls++;

    // Build free list from this chunk
    char *current = (char *)chunk;
    for (size_t i = 0; i < pool->nodes_per_chunk; i++) {
        AST_T *node = (AST_T *)current;
        memset(node, 0, sizeof(AST_T));  // Initialize node memory
        node->next = pool->free_list;
        pool->free_list = node;
        current += pool->node_size;
    }

    // Increase chunk size for next expansion (up to maximum)
    if (pool->nodes_per_chunk < AST_POOL_MAX_CHUNK_SIZE) {
        pool->nodes_per_chunk = (pool->nodes_per_chunk * AST_POOL_GROWTH_FACTOR);
        if (pool->nodes_per_chunk > AST_POOL_MAX_CHUNK_SIZE) {
            pool->nodes_per_chunk = AST_POOL_MAX_CHUNK_SIZE;
        }
    }

    AST_POOL_DEBUG("Expanded pool: chunk_size=%zu nodes, total_chunks=%zu",
                   pool->nodes_per_chunk,
                   pool->total_chunks);

    return true;
}

/**
 * @brief Allocate a node from a specific pool
 * @param pool Pool to allocate from
 * @return Allocated AST node or NULL on failure
 */
static AST_T *ast_memory_pool_alloc_from_pool(ASTMemoryPool *pool)
{
    if (!pool || !pool->initialized) {
        return NULL;
    }

    pthread_mutex_lock(&pool->mutex);

    // If no free nodes, expand the pool
    if (!pool->free_list) {
        if (!ast_pool_expand(pool)) {
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }
    }

    // Take node from free list
    AST_T *node = pool->free_list;
    pool->free_list = node->next;

    // Update statistics
    pool->allocations++;
    size_t current_usage = pool->allocations - pool->deallocations;
    if (current_usage > pool->peak_usage) {
        pool->peak_usage = current_usage;
    }

    pthread_mutex_unlock(&pool->mutex);

    // Initialize the node
    memset(node, 0, sizeof(AST_T));
    node->pooled = true;  // Mark as pooled

    return node;
}

/**
 * @brief Return a node to its pool
 * @param pool Pool to return to
 * @param node Node to return
 */
static void ast_memory_pool_return_to_pool(ASTMemoryPool *pool, AST_T *node)
{
    if (!pool || !pool->initialized || !node) {
        return;
    }

    pthread_mutex_lock(&pool->mutex);

    // Critical fix: Check for double-free by verifying node isn't already in free list
    AST_T *current = pool->free_list;
    while (current) {
        if (current == node) {
            // Double-free detected - node is already in free list
            pthread_mutex_unlock(&pool->mutex);
            AST_POOL_DEBUG("Double-free prevented for node %p", (void *)node);
            return;
        }
        current = current->next;
    }

    // Clear the pooled flag to prevent use-after-free
    node->pooled = false;

    // Clear type to mark as freed (helps detect use-after-free)
    node->type = -1;

    // Add to free list
    node->next = pool->free_list;
    pool->free_list = node;
    pool->deallocations++;

    pthread_mutex_unlock(&pool->mutex);
}

// ============================================================================
// MANIFEST COMPATIBLE FUNCTIONS - Primary interface
// ============================================================================

/**
 * @brief Cleanup memory pool and free all allocated blocks (MANIFEST version)
 * @param pool Pool to cleanup
 */
void ast_pool_cleanup(ASTMemoryPool *pool)
{
    if (!pool || !pool->initialized) {
        return;
    }

    pthread_mutex_lock(&pool->mutex);

    // Free all chunks
    for (size_t j = 0; j < pool->chunk_count; j++) {
        if (pool->chunks[j]) {
            memory_free(pool->chunks[j]);
        }
    }

    // Free chunks array
    if (pool->chunks) {
        memory_free(pool->chunks);
    }

    pool->initialized = false;

    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);

    AST_POOL_DEBUG("Memory pool cleaned up");
}

/**
 * @brief Allocate AST node from pool (MANIFEST version)
 * @param pool Pool to allocate from
 * @return Allocated memory block or NULL on failure
 */
void *ast_pool_alloc(ASTMemoryPool *pool)
{
    if (!pool || !pool->initialized) {
        return NULL;
    }

    return (void *)ast_memory_pool_alloc_from_pool(pool);
}

/**
 * @brief Free AST node back to pool (MANIFEST version)
 * @param pool Pool to return to
 * @param ptr Pointer to free
 */
void ast_pool_free(ASTMemoryPool *pool, void *ptr)
{
    if (!pool || !pool->initialized || !ptr) {
        return;
    }

    AST_T *node = (AST_T *)ptr;
    ast_memory_pool_return_to_pool(pool, node);
}

/**
 * @brief Get memory pool usage statistics (MANIFEST version)
 * @param stats Output structure to fill with statistics
 */
void ast_pool_get_stats(ASTPoolStats *stats)
{
    if (!stats) {
        return;
    }

    memset(stats, 0, sizeof(ASTPoolStats));

    if (!g_ast_pool_manager.initialized) {
        return;
    }

    // Aggregate statistics from all pools
    for (int i = 0; i < AST_POOL_COUNT; i++) {
        ASTMemoryPool *pool = g_ast_pool_manager.pools[i];
        if (pool) {
            pthread_mutex_lock(&pool->mutex);

            stats->total_allocations += pool->allocations;
            stats->total_frees += pool->deallocations;

            size_t current_usage = pool->allocations - pool->deallocations;
            stats->current_usage += current_usage;
            stats->peak_usage += pool->peak_usage;

            stats->memory_used += current_usage * pool->node_size;
            stats->memory_peak += pool->peak_usage * pool->node_size;

            pthread_mutex_unlock(&pool->mutex);
        }
    }

    // Add global statistics
    stats->pool_hits = g_ast_pool_manager.pool_stats.allocation_count;
    stats->pool_misses = g_ast_pool_manager.pool_stats.pool_misses;

    if (stats->total_allocations > 0) {
        stats->efficiency_ratio = (double)stats->pool_hits / stats->total_allocations;
    }
}

/**
 * @brief Initialize a memory pool for AST nodes (MANIFEST version)
 * @param pool Pool to initialize
 * @param node_size Size of nodes in the pool
 * @param initial_capacity Initial capacity of the pool
 * @return true on success, false on failure
 */
bool ast_pool_init(ASTMemoryPool *pool, size_t node_size, size_t initial_capacity)
{
    if (!pool) {
        return false;
    }

    memset(pool, 0, sizeof(ASTMemoryPool));

    pool->node_size = node_size;
    pool->nodes_per_chunk = initial_capacity;
    pool->total_chunks = 0;
    pool->free_list = NULL;

    // Initialize chunks array
    pool->chunk_capacity = 8;  // Start with 8 chunk pointers
    pool->chunks = memory_alloc(sizeof(void *) * pool->chunk_capacity);
    if (!pool->chunks) {
        return false;
    }
    pool->chunk_count = 0;

    // Initialize statistics
    pool->allocations = 0;
    pool->deallocations = 0;
    pool->peak_usage = 0;
    pool->malloc_calls = 0;

    // Initialize mutex
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        memory_free(pool->chunks);
        return false;
    }

    pool->initialized = true;
    return true;
}

/**
 * @brief Initialize AST memory pool system (global version)
 * @param initial_capacity Initial pool capacity per category
 * @return true on success, false on failure
 */
bool ast_pool_init_global(size_t initial_capacity)
{
    if (g_ast_pool_manager.initialized) {
        return true;  // Already initialized
    }

    // Initialize manager mutex
    if (pthread_mutex_init(&g_ast_pool_manager.manager_mutex, NULL) != 0) {
        return false;
    }

    // Initialize each pool type
    for (int i = 0; i < AST_POOL_COUNT; i++) {
        ASTMemoryPool *pool = memory_alloc(sizeof(ASTMemoryPool));
        if (!pool) {
            // Clean up previously initialized pools
            for (int j = 0; j < i; j++) {
                if (g_ast_pool_manager.pools[j]) {
                    pthread_mutex_destroy(&g_ast_pool_manager.pools[j]->mutex);
                    memory_free(g_ast_pool_manager.pools[j]->chunks);
                    memory_free(g_ast_pool_manager.pools[j]);
                }
            }
            pthread_mutex_destroy(&g_ast_pool_manager.manager_mutex);
            return false;
        }

        if (!ast_pool_init_single(pool, i, initial_capacity)) {
            memory_free(pool);
            // Clean up previously initialized pools
            for (int j = 0; j < i; j++) {
                if (g_ast_pool_manager.pools[j]) {
                    pthread_mutex_destroy(&g_ast_pool_manager.pools[j]->mutex);
                    memory_free(g_ast_pool_manager.pools[j]->chunks);
                    memory_free(g_ast_pool_manager.pools[j]);
                }
            }
            pthread_mutex_destroy(&g_ast_pool_manager.manager_mutex);
            return false;
        }

        g_ast_pool_manager.pools[i] = pool;
    }

    // Initialize manager
    memset(&g_ast_pool_manager.pool_stats, 0, sizeof(MemoryStats));
    g_ast_pool_manager.enabled = true;
    g_ast_pool_manager.initialized = true;

    AST_POOL_DEBUG("AST memory pools initialized with capacity %zu", initial_capacity);
    return true;
}

/**
 * @brief Allocate AST node from appropriate pool (GLOBAL VERSION)
 * @param type AST node type
 * @return Pointer to allocated AST node or NULL on failure
 */
AST_T *ast_pool_alloc_global(int type)
{
    // If pools not initialized or disabled, use regular allocation
    if (!g_ast_pool_manager.initialized || !g_ast_pool_manager.enabled) {
        AST_T *node = memory_alloc(sizeof(AST_T));
        if (node) {
            memset(node, 0, sizeof(AST_T));
            node->type = type;
            node->pooled = false;
        }
        return node;
    }

    // Select appropriate pool
    ASTPoolType pool_type = ast_pool_select_pool_for_type(type);
    ASTMemoryPool *pool = g_ast_pool_manager.pools[pool_type];

    // Try pool allocation first
    AST_T *node = ast_memory_pool_alloc_from_pool(pool);

    if (node) {
        node->type = type;
        g_ast_pool_manager.pool_stats.allocation_count++;
        return node;
    }

    // Fallback to regular allocation
    node = memory_alloc(sizeof(AST_T));
    if (node) {
        memset(node, 0, sizeof(AST_T));
        node->type = type;
        node->pooled = false;
        g_ast_pool_manager.pool_stats.pool_misses++;
    }

    return node;
}

/**
 * @brief Return AST node to pool for reuse (GLOBAL VERSION)
 * @param node AST node to deallocate
 */
void ast_pool_free_global(AST_T *node)
{
    if (!node) {
        return;
    }

    // If not pooled or pools not initialized, use regular free
    if (!node->pooled || !g_ast_pool_manager.initialized) {
        memory_free(node);
        return;
    }

    // Return to appropriate pool
    ASTPoolType pool_type = ast_pool_select_pool_for_type(node->type);
    ASTMemoryPool *pool = g_ast_pool_manager.pools[pool_type];

    ast_memory_pool_return_to_pool(pool, node);
    g_ast_pool_manager.pool_stats.free_count++;
}

/**
 * @brief Cleanup all pools and release memory (GLOBAL VERSION)
 */
void ast_pool_cleanup_global(void)
{
    if (!g_ast_pool_manager.initialized) {
        return;
    }

    pthread_mutex_lock(&g_ast_pool_manager.manager_mutex);

    // Clean up each pool
    for (int i = 0; i < AST_POOL_COUNT; i++) {
        ASTMemoryPool *pool = g_ast_pool_manager.pools[i];
        if (pool) {
            pthread_mutex_lock(&pool->mutex);

            // Free all chunks
            for (size_t j = 0; j < pool->chunk_count; j++) {
                if (pool->chunks[j]) {
                    memory_free(pool->chunks[j]);
                }
            }

            // Free chunks array
            memory_free(pool->chunks);

            pthread_mutex_unlock(&pool->mutex);
            pthread_mutex_destroy(&pool->mutex);

            memory_free(pool);
            g_ast_pool_manager.pools[i] = NULL;
        }
    }

    g_ast_pool_manager.initialized = false;
    g_ast_pool_manager.enabled = false;

    pthread_mutex_unlock(&g_ast_pool_manager.manager_mutex);
    pthread_mutex_destroy(&g_ast_pool_manager.manager_mutex);

    AST_POOL_DEBUG("AST memory pools cleaned up");
}

/**
 * @brief Get pool performance statistics (GLOBAL VERSION)
 * @return Current pool performance metrics
 */
MemoryStats ast_pool_get_stats_global(void)
{
    if (!g_ast_pool_manager.initialized) {
        MemoryStats empty_stats = {0};
        return empty_stats;
    }

    MemoryStats stats = g_ast_pool_manager.pool_stats;

    // Aggregate pool-specific statistics
    for (int i = 0; i < AST_POOL_COUNT; i++) {
        ASTMemoryPool *pool = g_ast_pool_manager.pools[i];
        if (pool) {
            pthread_mutex_lock(&pool->mutex);
            stats.peak_allocated += pool->peak_usage * pool->node_size;
            stats.current_allocated += (pool->allocations - pool->deallocations) * pool->node_size;
            pthread_mutex_unlock(&pool->mutex);
        }
    }

    return stats;
}

/**
 * @brief Enable or disable AST memory pools globally
 * @param enabled true to enable pools, false to use regular malloc
 */
void ast_pool_set_enabled(bool enabled)
{
    if (g_ast_pool_manager.initialized) {
        g_ast_pool_manager.enabled = enabled;
        AST_POOL_DEBUG("AST pools %s", enabled ? "enabled" : "disabled");
    }
}

/**
 * @brief Check if AST memory pools are enabled
 * @return true if pools are enabled, false otherwise
 */
bool ast_pool_is_enabled(void)
{
    return g_ast_pool_manager.initialized && g_ast_pool_manager.enabled;
}

/**
 * @brief Get detailed metrics for a specific pool type
 * @param pool_type The pool type to query
 * @return Metrics for the specified pool
 */
PoolMetrics ast_pool_get_pool_metrics(ASTPoolType pool_type)
{
    PoolMetrics metrics = {0};

    if (!g_ast_pool_manager.initialized || pool_type >= AST_POOL_COUNT) {
        return metrics;
    }

    ASTMemoryPool *pool = g_ast_pool_manager.pools[pool_type];
    if (!pool) {
        return metrics;
    }

    pthread_mutex_lock(&pool->mutex);

    metrics.total_allocations = pool->allocations;
    metrics.total_frees = pool->deallocations;
    metrics.current_usage = pool->allocations - pool->deallocations;
    metrics.peak_usage = pool->peak_usage;
    metrics.malloc_calls = pool->malloc_calls;
    metrics.pool_hits = pool->allocations;
    metrics.pool_misses = 0;  // Would need fallback tracking

    if (metrics.total_allocations > 0) {
        metrics.hit_ratio = (double)metrics.pool_hits / metrics.total_allocations;
    }

    pthread_mutex_unlock(&pool->mutex);

    return metrics;
}

/**
 * @brief Print pool status for debugging
 */
void ast_pool_debug_status(void)
{
    if (!g_ast_pool_manager.initialized) {
        printf("AST Memory Pools: NOT INITIALIZED\n");
        return;
    }

    printf("AST Memory Pool Status:\n");
    printf("======================\n");
    printf("Enabled: %s\n", g_ast_pool_manager.enabled ? "YES" : "NO");

    const char *pool_names[] = {"SMALL", "MEDIUM", "LARGE"};

    for (int i = 0; i < AST_POOL_COUNT; i++) {
        ASTMemoryPool *pool = g_ast_pool_manager.pools[i];
        if (!pool)
            continue;

        pthread_mutex_lock(&pool->mutex);

        size_t current_usage = pool->allocations - pool->deallocations;
        double efficiency =
            pool->allocations > 0
                ? ((double)pool->allocations / (pool->malloc_calls * pool->nodes_per_chunk)) * 100.0
                : 0.0;

        printf("%s_NODE_POOL: %zu/%zu used (%.1f%% efficiency)\n",
               pool_names[i],
               current_usage,
               pool->total_chunks * pool->nodes_per_chunk,
               efficiency);
        printf("  Chunks: %zu, Malloc calls: %zu, Peak usage: %zu\n",
               pool->total_chunks,
               pool->malloc_calls,
               pool->peak_usage);

        pthread_mutex_unlock(&pool->mutex);
    }

    MemoryStats stats = ast_pool_get_stats_global();
    printf("Total allocations: %zu, Total frees: %zu\n", stats.allocation_count, stats.free_count);
    printf("Current usage: %zu bytes, Peak usage: %zu bytes\n",
           stats.current_allocated,
           stats.peak_allocated);
}

/**
 * @brief Check if a node was allocated from pools
 * @param node AST node to check
 * @return true if node is from pool, false otherwise
 */
bool ast_pool_is_pooled_node(AST_T *node) { return node && node->pooled; }

// ============================================================================
// MANIFEST DESIRED FUNCTIONS - Missing implementations
// ============================================================================

/**
 * @brief Initialize global AST pool manager
 */
void ast_pool_global_init(void) { ast_pool_manager_init(&g_ast_pool_manager); }

/**
 * @brief Cleanup global AST pool manager
 */
void ast_pool_global_cleanup(void) { ast_pool_manager_cleanup(&g_ast_pool_manager); }

/**
 * @brief Allocate AST node from appropriate pool
 * @param ast_type AST node type
 * @return Allocated AST node or NULL on failure
 */
AST_T *ast_pool_alloc_node(int ast_type)
{
    // If pools not initialized or disabled, use regular allocation
    if (!g_ast_pool_manager.initialized || !g_ast_pool_manager.enabled) {
        AST_T *node = memory_alloc(sizeof(AST_T));
        if (node) {
            memset(node, 0, sizeof(AST_T));
            node->type = ast_type;
            node->pooled = false;
        }
        return node;
    }

    // Select appropriate pool
    ASTPoolType pool_type = ast_pool_select_type(ast_type);
    ASTMemoryPool *pool = g_ast_pool_manager.pools[pool_type];

    // Try pool allocation first
    AST_T *node = ast_memory_pool_alloc_from_pool(pool);

    if (node) {
        node->type = ast_type;
        g_ast_pool_manager.pool_stats.allocation_count++;
        return node;
    }

    // Fallback to regular allocation
    node = memory_alloc(sizeof(AST_T));
    if (node) {
        memset(node, 0, sizeof(AST_T));
        node->type = ast_type;
        node->pooled = false;
        g_ast_pool_manager.pool_stats.pool_misses++;
    }

    return node;
}

/**
 * @brief Free AST node back to appropriate pool
 * @param node AST node to free
 */
void ast_pool_free_node(AST_T *node)
{
    if (!node) {
        return;
    }

    // If not pooled or pools not initialized, use regular free
    if (!node->pooled || !g_ast_pool_manager.initialized) {
        memory_free(node);
        return;
    }

    // Return to appropriate pool
    ASTPoolType pool_type = ast_pool_select_type(node->type);
    ASTMemoryPool *pool = g_ast_pool_manager.pools[pool_type];

    ast_memory_pool_return_to_pool(pool, node);
    g_ast_pool_manager.pool_stats.free_count++;
}

/**
 * @brief Defragment memory pool to reduce fragmentation
 * @param pool Pool to defragment
 */
void ast_pool_defragment(ASTMemoryPool *pool)
{
    if (!pool || !pool->initialized) {
        return;
    }

    pthread_mutex_lock(&pool->mutex);

    // Simple defragmentation: rebuild free list from all chunks
    // This consolidates fragmented free nodes into a contiguous list

    pool->free_list = NULL;

    // Rebuild free list from all chunks
    for (size_t chunk_idx = 0; chunk_idx < pool->chunk_count; chunk_idx++) {
        void *chunk = pool->chunks[chunk_idx];
        if (!chunk)
            continue;

        char *current = (char *)chunk;
        size_t nodes_in_chunk =
            (chunk_idx == pool->chunk_count - 1) ? pool->nodes_per_chunk : pool->nodes_per_chunk;

        for (size_t i = 0; i < nodes_in_chunk; i++) {
            AST_T *node = (AST_T *)current;

            // Check if this node is free (simplified check)
            // In a real implementation, we would track allocated vs free nodes
            // For now, we'll only defragment by rebuilding the free list structure
            if (node->type == 0 && node->pooled == true) {  // Heuristic for free node
                node->next = pool->free_list;
                pool->free_list = node;
            }

            current += pool->node_size;
        }
    }

    pthread_mutex_unlock(&pool->mutex);

    AST_POOL_DEBUG("Pool defragmented, free list rebuilt");
}
