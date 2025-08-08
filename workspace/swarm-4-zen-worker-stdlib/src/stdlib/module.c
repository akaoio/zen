#include "zen/stdlib/module.h"
#include "zen/core/memory.h"
#include "zen/core/error.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/ast.h"
#include "zen/stdlib/io.h"
#include "zen/config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @file module.c
 * @brief Module system with semantic import resolution - Phase 2A
 * 
 * Provides sophisticated module loading with:
 * - Semantic resolution (maps descriptions to actual modules)
 * - Multiple file format support (.zen, .json, .yaml)
 * - Provider registration system
 */

// Default provider capacity
#define MODULE_PROVIDER_DEFAULT_CAPACITY 16

// Forward declarations of static functions
static void module_add_search_path_internal(ModuleResolver* resolver, const char* search_path);
static char* resolve_module_path(ModuleResolver* resolver, const char* module_path);
static size_t hash_module_path(const char* path);
static ModuleCacheEntry* cache_lookup(ModuleCache* cache, const char* path);
static void cache_store(ModuleCache* cache, const char* path, Value* module);
static void cache_clear(ModuleCache* cache);

// Helper function to determine file type from extension
static enum {
    MODULE_TYPE_ZEN,
    MODULE_TYPE_JSON,
    MODULE_TYPE_YAML,
    MODULE_TYPE_UNKNOWN
} get_module_type(const char* path) {
    if (!path) return MODULE_TYPE_UNKNOWN;
    
    const char* ext = strrchr(path, '.');
    if (!ext) return MODULE_TYPE_UNKNOWN;
    
    if (strcmp(ext, ".zen") == 0) return MODULE_TYPE_ZEN;
    if (strcmp(ext, ".json") == 0) return MODULE_TYPE_JSON;
    if (strcmp(ext, ".yaml") == 0 || strcmp(ext, ".yml") == 0) return MODULE_TYPE_YAML;
    
    return MODULE_TYPE_UNKNOWN;
}

/**
 * @brief Initialize semantic module resolution system
 * @return Initialized resolver instance, NULL on error
 */
ModuleResolver* module_resolver_init(void) {
    ModuleResolver* resolver = (ModuleResolver*)memory_alloc(sizeof(ModuleResolver));
    if (!resolver) {
        return NULL;
    }
    
    // Initialize providers array
    resolver->provider_capacity = MODULE_PROVIDER_DEFAULT_CAPACITY;
    resolver->providers = (ModuleProvider*)memory_alloc(sizeof(ModuleProvider) * resolver->provider_capacity);
    if (!resolver->providers) {
        memory_free(resolver);
        return NULL;
    }
    resolver->provider_count = 0;
    
    // Initialize module cache
    resolver->cache = (ModuleCache*)memory_alloc(sizeof(ModuleCache));
    if (!resolver->cache) {
        memory_free(resolver->providers);
        memory_free(resolver);
        return NULL;
    }
    
    resolver->cache->buckets = (ModuleCacheEntry**)memory_alloc(sizeof(ModuleCacheEntry*) * ZEN_DEFAULT_HASH_TABLE_SIZE);
    if (!resolver->cache->buckets) {
        memory_free(resolver->cache);
        memory_free(resolver->providers);
        memory_free(resolver);
        return NULL;
    }
    
    resolver->cache->bucket_count = ZEN_DEFAULT_HASH_TABLE_SIZE;
    resolver->cache->entry_count = 0;
    
    // Initialize all cache buckets as empty
    for (size_t i = 0; i < resolver->cache->bucket_count; i++) {
        resolver->cache->buckets[i] = NULL;
    }
    
    // Initialize search paths
    resolver->search_paths = NULL;
    resolver->search_path_count = 0;
    
    // Register default search paths
    module_add_search_path_internal(resolver, ".");
    module_add_search_path_internal(resolver, "./modules");
    module_add_search_path_internal(resolver, "./lib");
    
    // Register built-in capability providers
    module_register_provider(resolver, "data visualization", "charts.zen");
    module_register_provider(resolver, "chart", "charts.zen"); 
    module_register_provider(resolver, "plotting", "charts.zen");
    module_register_provider(resolver, "http client", "http.zen");
    module_register_provider(resolver, "web requests", "http.zen");
    module_register_provider(resolver, "json processing", "json_utils.zen");
    module_register_provider(resolver, "file operations", "fileio.zen");
    module_register_provider(resolver, "string utilities", "strings.zen");
    module_register_provider(resolver, "math functions", "math_ext.zen");
    module_register_provider(resolver, "date time", "datetime.zen");
    module_register_provider(resolver, "logging", "logger.zen");
    
    return resolver;
}

/**
 * @brief Resolve semantic description to actual module path
 * @param resolver The module resolver instance
 * @param semantic_description Semantic description (e.g., "data visualization")
 * @return Resolved module path, NULL if not found (caller must free)
 */
char* module_resolve_semantic(ModuleResolver* resolver, const char* semantic_description) {
    if (!resolver || !semantic_description) {
        return NULL;
    }
    
    // Direct lookup first
    for (size_t i = 0; i < resolver->provider_count; i++) {
        if (strcmp(resolver->providers[i].capability, semantic_description) == 0) {
            return memory_strdup(resolver->providers[i].module_path);
        }
    }
    
    // Fuzzy matching for partial descriptions
    for (size_t i = 0; i < resolver->provider_count; i++) {
        const char* capability = resolver->providers[i].capability;
        
        // Check if semantic description is contained in capability
        if (strstr(capability, semantic_description) != NULL) {
            return memory_strdup(resolver->providers[i].module_path);
        }
        
        // Check if capability is contained in semantic description
        if (strstr(semantic_description, capability) != NULL) {
            return memory_strdup(resolver->providers[i].module_path);
        }
    }
    
    // Keyword-based matching
    const char* keywords[][2] = {
        {"chart", "charts.zen"},
        {"graph", "charts.zen"},
        {"plot", "charts.zen"},
        {"http", "http.zen"},
        {"web", "http.zen"},
        {"request", "http.zen"},
        {"json", "json_utils.zen"},
        {"file", "fileio.zen"},
        {"io", "fileio.zen"},
        {"string", "strings.zen"},
        {"text", "strings.zen"},
        {"math", "math_ext.zen"},
        {"calculate", "math_ext.zen"},
        {"date", "datetime.zen"},
        {"time", "datetime.zen"},
        {"log", "logger.zen"}
    };
    
    size_t keyword_count = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < keyword_count; i++) {
        if (strstr(semantic_description, keywords[i][0]) != NULL) {
            return memory_strdup(keywords[i][1]);
        }
    }
    
    return NULL; // No match found
}

/**
 * @brief Register module as provider of specific capability
 * @param resolver The module resolver instance
 * @param capability The capability name
 * @param module_path The path to the providing module
 */
void module_register_provider(ModuleResolver* resolver, const char* capability, const char* module_path) {
    if (!resolver || !capability || !module_path) {
        return;
    }
    
    // Check if we need to expand the providers array
    if (resolver->provider_count >= resolver->provider_capacity) {
        size_t new_capacity = resolver->provider_capacity * 2;
        ModuleProvider* new_providers = (ModuleProvider*)memory_realloc(
            resolver->providers, 
            sizeof(ModuleProvider) * new_capacity
        );
        if (!new_providers) {
            return; // Failed to expand, skip registration
        }
        resolver->providers = new_providers;
        resolver->provider_capacity = new_capacity;
    }
    
    // Check if capability already exists (update if so)
    for (size_t i = 0; i < resolver->provider_count; i++) {
        if (strcmp(resolver->providers[i].capability, capability) == 0) {
            // Update existing provider
            memory_free(resolver->providers[i].module_path);
            resolver->providers[i].module_path = memory_strdup(module_path);
            return;
        }
    }
    
    // Add new provider
    ModuleProvider* provider = &resolver->providers[resolver->provider_count];
    provider->capability = memory_strdup(capability);
    provider->module_path = memory_strdup(module_path);
    resolver->provider_count++;
}

// Resolve full path using search paths
static char* resolve_module_path(ModuleResolver* resolver, const char* module_path) {
    if (!resolver || !module_path) return NULL;
    
    // If path is absolute or starts with ./, use as-is
    if (module_path[0] == '/' || (module_path[0] == '.' && module_path[1] == '/')) {
        return memory_strdup(module_path);
    }
    
    // Try each search path
    for (size_t i = 0; i < resolver->search_path_count; i++) {
        size_t path_len = strlen(resolver->search_paths[i]) + strlen(module_path) + 2;
        char* full_path = (char*)memory_alloc(path_len);
        if (!full_path) continue;
        
        snprintf(full_path, path_len, "%s/%s", resolver->search_paths[i], module_path);
        
        // Check if file exists
        FILE* file = fopen(full_path, "r");
        if (file) {
            fclose(file);
            return full_path; // Found it
        }
        
        memory_free(full_path);
    }
    
    // Not found in any search path, return original
    return memory_strdup(module_path);
}

/**
 * @brief Load and execute ZEN module file
 * @param module_path Path to the module file
 * @return Module result value, error value on failure
 */
Value* module_load_file(const char* module_path) {
    if (!module_path) {
        return error_invalid_argument("module_load_file", "module_path");
    }
    
    // Create a temporary resolver for path resolution
    static ModuleResolver* global_resolver = NULL;
    if (!global_resolver) {
        global_resolver = module_resolver_init();
        if (!global_resolver) {
            return error_memory_allocation();
        }
    }
    
    // Resolve full path
    char* full_path = resolve_module_path(global_resolver, module_path);
    if (!full_path) {
        return error_file_not_found(module_path);
    }
    
    // Check cache first
    ModuleCacheEntry* cached = cache_lookup(global_resolver->cache, full_path);
    if (cached) {
        memory_free(full_path);
        value_ref(cached->module); // Return a new reference
        return cached->module;
    }
    
    // Read file content
    FILE* file = fopen(full_path, "r");
    if (!file) {
        memory_free(full_path);
        return error_file_not_found(module_path);
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(file);
        memory_free(full_path);
        return error_new("Failed to get file size");
    }
    
    // Read content
    char* content = (char*)memory_alloc(file_size + 1);
    if (!content) {
        fclose(file);
        memory_free(full_path);
        return error_memory_allocation();
    }
    
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';
    fclose(file);
    
    Value* result = NULL;
    int module_type = get_module_type(full_path);
    
    switch (module_type) {
        case MODULE_TYPE_ZEN: {
            // Parse and execute ZEN file
            lexer_T* lexer = lexer_new(content);
            if (!lexer) {
                memory_free(content);
                memory_free(full_path);
                return error_parsing_failed("Failed to create lexer");
            }
            
            parser_T* parser = parser_new(lexer);
            if (!parser) {
                lexer_free(lexer);
                memory_free(content);
                memory_free(full_path);
                return error_parsing_failed("Failed to create parser");
            }
            
            // Create a scope for parsing
            scope_T* scope = scope_new(NULL);
            if (!scope) {
                parser_free(parser);
                lexer_free(lexer);
                memory_free(content);
                memory_free(full_path);
                return error_memory_allocation();
            }
            
            AST_T* ast = parser_parse(parser, scope);
            if (!ast) {
                scope_free(scope);
                parser_free(parser);
                lexer_free(lexer);
                memory_free(content);
                memory_free(full_path);
                return error_parsing_failed("Failed to parse ZEN file");
            }
            
            // Execute the AST
            visitor_T* visitor = visitor_new();
            if (!visitor) {
                ast_free(ast);
                scope_free(scope);
                parser_free(parser);
                lexer_free(lexer);
                memory_free(content);
                memory_free(full_path);
                return error_memory_allocation();
            }
            
            // Execute the AST - the visitor evaluates and modifies the AST in place
            visitor_visit(visitor, ast);
            
            // Return the evaluated result from visitor execution
            // The visitor should have evaluated the AST and set the result
            if (visitor->has_return_value) {
                result = visitor->return_value;
                value_ref(result); // Add reference for return
            } else {
                // If no explicit return value, create a module object containing exported symbols
                Value* module_obj = value_new_object();
                
                // Extract exported functions and variables from the scope
                // This would ideally traverse the scope and add all defined symbols
                // For now, return a success indicator with module metadata
                object_set(module_obj, "loaded", value_new_boolean(true));
                object_set(module_obj, "path", value_new_string(full_path));
                object_set(module_obj, "type", value_new_string("zen"));
                
                result = module_obj;
            }
            
            // Cleanup
            visitor_free(visitor);
            ast_free(ast);
            scope_free(scope);
            parser_free(parser);
            lexer_free(lexer);
            break;
        }
        
        case MODULE_TYPE_JSON: {
            // Parse JSON file using existing JSON stdlib functions
            Value* json_string = value_new_string(content);
            Value* json_args[] = { json_string };
            
            // Use the existing JSON parse function
            result = json_parse(json_args, 1);
            
            // If JSON parsing failed, wrap in a module object
            if (!result || error_is_error(result)) {
                Value* module_obj = value_new_object();
                object_set(module_obj, "error", result ? result : error_parsing_failed("Invalid JSON"));
                object_set(module_obj, "path", value_new_string(full_path));
                object_set(module_obj, "type", value_new_string("json"));
                result = module_obj;
            } else {
                // Wrap successful JSON result in module object
                Value* module_obj = value_new_object();
                object_set(module_obj, "data", result);
                object_set(module_obj, "loaded", value_new_boolean(true));
                object_set(module_obj, "path", value_new_string(full_path));
                object_set(module_obj, "type", value_new_string("json"));
                result = module_obj;
            }
            
            value_unref(json_string);
            break;
        }
        
        case MODULE_TYPE_YAML: {
            // Parse YAML file - for now, create a structured representation
            // In a full implementation, this would use a YAML parser like libyaml
            Value* module_obj = value_new_object();
            
            // Basic YAML parsing - split by lines and try to parse key:value pairs
            Value* data_obj = value_new_object();
            char* content_copy = memory_strdup(content);
            char* line = strtok(content_copy, "\n");
            
            while (line) {
                // Skip comments and empty lines
                while (*line == ' ' || *line == '\t') line++; // trim leading whitespace
                if (*line == '#' || *line == '\0') {
                    line = strtok(NULL, "\n");
                    continue;
                }
                
                // Look for key: value pattern
                char* colon = strchr(line, ':');
                if (colon) {
                    *colon = '\0';
                    char* key = line;
                    char* value_str = colon + 1;
                    
                    // Trim whitespace
                    while (*value_str == ' ' || *value_str == '\t') value_str++;
                    
                    // Remove trailing whitespace
                    char* end = value_str + strlen(value_str) - 1;
                    while (end > value_str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
                        *end-- = '\0';
                    }
                    
                    // Try to parse value as number or boolean, otherwise string
                    Value* parsed_value;
                    if (strcmp(value_str, "true") == 0) {
                        parsed_value = value_new_boolean(true);
                    } else if (strcmp(value_str, "false") == 0) {
                        parsed_value = value_new_boolean(false);
                    } else if (strcmp(value_str, "null") == 0) {
                        parsed_value = value_new_null();
                    } else {
                        char* endptr;
                        double num = strtod(value_str, &endptr);
                        if (*endptr == '\0' && endptr != value_str) {
                            parsed_value = value_new_number(num);
                        } else {
                            // Remove quotes if present
                            if ((*value_str == '"' || *value_str == '\'') && 
                                value_str[strlen(value_str)-1] == *value_str) {
                                value_str++;
                                value_str[strlen(value_str)-1] = '\0';
                            }
                            parsed_value = value_new_string(value_str);
                        }
                    }
                    
                    object_set(data_obj, key, parsed_value);
                }
                
                line = strtok(NULL, "\n");
            }
            
            memory_free(content_copy);
            
            // Wrap in module object
            object_set(module_obj, "data", data_obj);
            object_set(module_obj, "loaded", value_new_boolean(true));
            object_set(module_obj, "path", value_new_string(full_path));
            object_set(module_obj, "type", value_new_string("yaml"));
            
            result = module_obj;
            break;
        }
        
        default:
            result = error_new("Unsupported module file type");
            break;
    }
    
    // Store result in cache if successful
    if (result && !error_is_error(result)) {
        cache_store(global_resolver->cache, full_path, result);
    }
    
    memory_free(content);
    memory_free(full_path);
    return result;
}

// Add search path for module resolution (internal function)
static void module_add_search_path_internal(ModuleResolver* resolver, const char* search_path) {
    if (!resolver || !search_path) return;
    
    // Check if path already exists
    for (size_t i = 0; i < resolver->search_path_count; i++) {
        if (strcmp(resolver->search_paths[i], search_path) == 0) {
            return; // Already exists
        }
    }
    
    // Expand search paths array if needed
    char** new_paths = (char**)memory_realloc(
        resolver->search_paths,
        sizeof(char*) * (resolver->search_path_count + 1)
    );
    if (!new_paths) return;
    
    resolver->search_paths = new_paths;
    resolver->search_paths[resolver->search_path_count] = memory_strdup(search_path);
    resolver->search_path_count++;
}

// ============================================================================
// MODULE CACHE IMPLEMENTATION
// ============================================================================

/**
 * @brief Hash function for module paths
 * @param path Module path to hash
 * @return Hash value for the path
 */
static size_t hash_module_path(const char* path) {
    if (!path) return 0;
    
    size_t hash = 5381;
    const unsigned char* str = (const unsigned char*)path;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash;
}

/**
 * @brief Look up a module in the cache
 * @param cache Module cache to search
 * @param path Module path to find
 * @return Cache entry if found, NULL otherwise
 */
static ModuleCacheEntry* cache_lookup(ModuleCache* cache, const char* path) {
    if (!cache || !path) {
        return NULL;
    }
    
    size_t hash = hash_module_path(path);
    size_t index = hash % cache->bucket_count;
    
    ModuleCacheEntry* entry = cache->buckets[index];
    while (entry) {
        if (strcmp(entry->path, path) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief Store a module in the cache
 * @param cache Module cache to store in
 * @param path Module path
 * @param module Module value to cache
 */
static void cache_store(ModuleCache* cache, const char* path, Value* module) {
    if (!cache || !path || !module) {
        return;
    }
    
    // Check if already cached
    if (cache_lookup(cache, path)) {
        return; // Already cached
    }
    
    // Create new cache entry
    ModuleCacheEntry* entry = (ModuleCacheEntry*)memory_alloc(sizeof(ModuleCacheEntry));
    if (!entry) {
        return;
    }
    
    entry->path = memory_strdup(path);
    if (!entry->path) {
        memory_free(entry);
        return;
    }
    
    entry->module = module;
    value_ref(module); // Increment reference count for caching
    
    // Add to hash table
    size_t hash = hash_module_path(path);
    size_t index = hash % cache->bucket_count;
    
    entry->next = cache->buckets[index];
    cache->buckets[index] = entry;
    cache->entry_count++;
}

/**
 * @brief Clear all entries from the cache
 * @param cache Module cache to clear
 */
__attribute__((unused)) static void cache_clear(ModuleCache* cache) {
    if (!cache) {
        return;
    }
    
    for (size_t i = 0; i < cache->bucket_count; i++) {
        ModuleCacheEntry* entry = cache->buckets[i];
        while (entry) {
            ModuleCacheEntry* next = entry->next;
            
            if (entry->path) {
                memory_free(entry->path);
            }
            if (entry->module) {
                value_unref(entry->module);
            }
            memory_free(entry);
            
            entry = next;
        }
        cache->buckets[i] = NULL;
    }
    
    cache->entry_count = 0;
}