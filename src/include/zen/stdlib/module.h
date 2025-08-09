#ifndef ZEN_MODULE_H
#define ZEN_MODULE_H

#include "zen/core/runtime_value.h"

#include <stddef.h>

/**
 * @file module.h
 * @brief Module system with semantic import resolution - Phase 2A
 *
 * This module provides a sophisticated module loading system that supports:
 * - Semantic module resolution (e.g., "data visualization" → "charts.zen")
 * - Multiple module types (.zen, .json, .yaml)
 * - Module caching to avoid duplicate loads
 * - Provider registration for capabilities
 */

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct ModuleProvider ModuleProvider;
typedef struct ModuleCache ModuleCache;

/**
 * @brief Module resolver for semantic import resolution
 */
typedef struct ModuleResolver {
    ModuleProvider *providers; /**< Registered capability providers */
    size_t provider_count;     /**< Number of registered providers */
    size_t provider_capacity;  /**< Capacity of providers array */
    ModuleCache *cache;        /**< Module cache to avoid duplicate loads */
    char **search_paths;       /**< Module search paths */
    size_t search_path_count;  /**< Number of search paths */
} ModuleResolver;

/**
 * @brief Module provider entry
 */
typedef struct ModuleProvider {
    char *capability;  /**< Capability name (e.g., "data visualization") */
    char *module_path; /**< Path to providing module */
} ModuleProvider;

/**
 * @brief Module cache entry
 */
typedef struct ModuleCacheEntry {
    char *path;                    /**< Module path */
    RuntimeValue *module;          /**< Cached module value */
    struct ModuleCacheEntry *next; /**< Next entry for collision resolution */
} ModuleCacheEntry;

/**
 * @brief Module cache
 */
typedef struct ModuleCache {
    ModuleCacheEntry **buckets; /**< Hash table buckets */
    size_t bucket_count;        /**< Number of buckets */
    size_t entry_count;         /**< Number of cached entries */
} ModuleCache;

/**
 * @brief Initialize semantic module resolution system
 * @return ModuleResolver* Initialized resolver instance, NULL on error
 */
ModuleResolver *module_resolver_init(void);

/**
 * @brief Resolve semantic description to actual module path
 * @param resolver The module resolver instance
 * @param semantic_description Semantic description (e.g., "data visualization")
 * @return char* Resolved module path, NULL if not found (caller must free)
 */
char *module_resolve_semantic(ModuleResolver *resolver, const char *semantic_description);

/**
 * @brief Register module as provider of specific capability
 * @param resolver The module resolver instance
 * @param capability The capability name
 * @param module_path The path to the providing module
 */
void module_register_provider(ModuleResolver *resolver,
                              const char *capability,
                              const char *module_path);

/**
 * @brief Load and execute ZEN module file
 * @param module_path Path to the module file
 * @return Value* Module result value, error value on failure
 */
RuntimeValue *module_load_file(const char *module_path);

#ifdef __cplusplus
}
#endif

#endif /* ZEN_MODULE_H */