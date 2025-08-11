/*
 * module.c
 * Module system with semantic import resolution for ZEN stdlib
 *
 * Provides module loading, caching, and dependency resolution.
 */

#include "zen/stdlib/module.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global hash table size constant
#define ZEN_DEFAULT_HASH_TABLE_SIZE 32

// Module system internal functions

ModuleResolver *module_resolver_init(void)
{
    ModuleResolver *resolver = memory_alloc(sizeof(ModuleResolver));
    if (!resolver) {
        return NULL;
    }

    // Initialize providers array
    resolver->provider_capacity = 16;
    resolver->providers = memory_alloc(resolver->provider_capacity * sizeof(ModuleProvider));
    resolver->provider_count = 0;

    // Initialize cache
    resolver->cache = memory_alloc(sizeof(ModuleCache));
    if (resolver->cache) {
        resolver->cache->bucket_count = 32;
        resolver->cache->buckets =
            memory_alloc(resolver->cache->bucket_count * sizeof(ModuleCacheEntry *));
        resolver->cache->entry_count = 0;
        if (resolver->cache->buckets) {
            for (size_t i = 0; i < resolver->cache->bucket_count; i++) {
                resolver->cache->buckets[i] = NULL;
            }
        }
    }

    // Initialize search paths
    resolver->search_path_count = 3;
    resolver->search_paths = memory_alloc(resolver->search_path_count * sizeof(char *));
    if (resolver->search_paths) {
        resolver->search_paths[0] = memory_strdup(".");
        resolver->search_paths[1] = memory_strdup("./lib");
        resolver->search_paths[2] = memory_strdup("/usr/local/lib/zen");
    }

    return resolver;
}

char *module_resolve_semantic(ModuleResolver *resolver, const char *semantic_description)
{
    if (!resolver || !semantic_description) {
        return NULL;
    }

    // Check registered providers first
    for (size_t i = 0; i < resolver->provider_count; i++) {
        if (resolver->providers[i].capability &&
            strcmp(resolver->providers[i].capability, semantic_description) == 0) {
            return memory_strdup(resolver->providers[i].module_path);
        }
    }

    // Basic semantic mappings for common requests
    if (strstr(semantic_description, "http") || strstr(semantic_description, "web")) {
        return memory_strdup("lib/http.zen");
    } else if (strstr(semantic_description, "json") || strstr(semantic_description, "data")) {
        return memory_strdup("lib/json.zen");
    } else if (strstr(semantic_description, "math") || strstr(semantic_description, "calculate")) {
        return memory_strdup("lib/math.zen");
    } else if (strstr(semantic_description, "string") || strstr(semantic_description, "text")) {
        return memory_strdup("lib/string.zen");
    }

    // Default fallback - try to find a module with similar name
    char *module_path = memory_alloc(256);
    snprintf(module_path, 256, "lib/%s.zen", semantic_description);
    return module_path;
}

void module_register_provider(ModuleResolver *resolver,
                              const char *capability,
                              const char *module_path)
{
    if (!resolver || !capability || !module_path || !resolver->providers) {
        return;
    }

    // Check if we need to expand providers array
    if (resolver->provider_count >= resolver->provider_capacity) {
        resolver->provider_capacity *= 2;
        ModuleProvider *new_providers = memory_realloc(
            resolver->providers, resolver->provider_capacity * sizeof(ModuleProvider));
        if (!new_providers) {
            return;  // Failed to expand
        }
        resolver->providers = new_providers;
    }

    // Add new provider
    size_t index = resolver->provider_count++;
    resolver->providers[index].capability = memory_strdup(capability);
    resolver->providers[index].module_path = memory_strdup(module_path);

    // Optional: validate that the module file exists
    FILE *test_file = fopen(module_path, "r");
    if (test_file) {
        fclose(test_file);
        // Module exists, registration successful
    }
}

RuntimeValue *module_load_file(const char *module_path)
{
    if (!module_path) {
        return rv_new_error("module_load_file requires valid module path", -1);
    }

    // Open the module file
    FILE *file = fopen(module_path, "r");
    if (!file) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Cannot open module file: %s", module_path);
        return rv_new_error(error_msg, -1);
    }

    // Read file contents
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return rv_new_error("Module file is empty or invalid", -1);
    }

    char *content = memory_alloc(file_size + 1);
    if (!content) {
        fclose(file);
        return rv_new_error("Failed to allocate memory for module content", -1);
    }

    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        memory_free(content);
        return rv_new_error("Failed to read module file completely", -1);
    }

    content[file_size] = '\0';

    // Create module object with metadata
    RuntimeValue *module = rv_new_object();
    if (!module) {
        memory_free(content);
        return rv_new_error("Failed to create module object", -1);
    }

    // Set module properties
    rv_object_set(module, "path", rv_new_string(module_path));
    rv_object_set(module, "content", rv_new_string(content));
    rv_object_set(module, "size", rv_new_number((double)file_size));
    rv_object_set(module, "loaded", rv_new_boolean(true));

    // Extract module name from path
    const char *module_name = strrchr(module_path, '/');
    if (!module_name) {
        module_name = module_path;
    } else {
        module_name++;  // Skip the slash
    }

    // Remove .zen extension if present
    char name_buffer[256];
    strncpy(name_buffer, module_name, sizeof(name_buffer) - 1);
    name_buffer[sizeof(name_buffer) - 1] = '\0';
    char *ext = strstr(name_buffer, ".zen");
    if (ext) {
        *ext = '\0';
    }

    rv_object_set(module, "name", rv_new_string(name_buffer));
    rv_object_set(module, "timestamp", rv_new_number((double)time(NULL)));

    // Add exports object (empty initially, would be populated by module execution)
    RuntimeValue *exports = rv_new_object();
    rv_object_set(module, "exports", exports);
    rv_unref(exports);

    memory_free(content);
    return module;
}

RuntimeValue *module_import_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("import requires exactly 1 argument (module_path)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("import requires string module path", -1);
    }

    const char *module_path = args[0]->data.string.data;

    // Load the module using module_load_file
    RuntimeValue *module = module_load_file(module_path);
    if (!module || module->type == RV_ERROR) {
        return module;  // Return error from module_load_file
    }

    // Create import result
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "module", rv_ref(module));
    rv_object_set(result, "path", rv_new_string(module_path));
    rv_object_set(result, "imported", rv_new_boolean(true));

    rv_unref(module);
    return result;
}

RuntimeValue *module_require_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("requireModule requires exactly 1 argument (module_path)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("requireModule requires string module path", -1);
    }

    const char *module_path = args[0]->data.string.data;

    // Load the module using module_load_file
    RuntimeValue *module = module_load_file(module_path);
    if (!module || module->type == RV_ERROR) {
        // For require, we return an error if module can't be loaded
        if (module && module->type == RV_ERROR) {
            return module;
        }
        return rv_new_error("Failed to require module", -1);
    }

    // Create require result with stronger validation
    RuntimeValue *loaded = rv_object_get(module, "loaded");
    if (!loaded || loaded->type != RV_BOOLEAN || !loaded->data.boolean) {
        rv_unref(module);
        return rv_new_error("Required module failed validation", -1);
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "module", rv_ref(module));
    rv_object_set(result, "path", rv_new_string(module_path));
    rv_object_set(result, "required", rv_new_boolean(true));
    rv_object_set(result, "validated", rv_new_boolean(true));
    rv_object_set(result, "timestamp", rv_new_number((double)time(NULL)));

    rv_unref(module);
    return result;
}

void module_resolver_cleanup(ModuleResolver *resolver)
{
    if (!resolver) {
        return;
    }

    // Clean up providers
    if (resolver->providers) {
        for (size_t i = 0; i < resolver->provider_count; i++) {
            if (resolver->providers[i].capability) {
                memory_free((void *)resolver->providers[i].capability);
            }
            if (resolver->providers[i].module_path) {
                memory_free((void *)resolver->providers[i].module_path);
            }
        }
        memory_free(resolver->providers);
    }

    // Clean up cache
    if (resolver->cache) {
        if (resolver->cache->buckets) {
            for (size_t i = 0; i < resolver->cache->bucket_count; i++) {
                ModuleCacheEntry *entry = resolver->cache->buckets[i];
                while (entry) {
                    ModuleCacheEntry *next = entry->next;
                    if (entry->path) {
                        memory_free((void *)entry->path);
                    }
                    if (entry->module) {
                        rv_unref(entry->module);
                    }
                    memory_free(entry);
                    entry = next;
                }
            }
            memory_free(resolver->cache->buckets);
        }
        memory_free(resolver->cache);
    }

    // Clean up search paths
    if (resolver->search_paths) {
        for (size_t i = 0; i < resolver->search_path_count; i++) {
            if (resolver->search_paths[i]) {
                memory_free(resolver->search_paths[i]);
            }
        }
        memory_free(resolver->search_paths);
    }

    memory_free(resolver);
}