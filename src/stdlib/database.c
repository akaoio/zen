/*
 * database.c
 * Database-like operations for ZEN
 * Provides enhanced get/put operations with nested property access
 */

#define _GNU_SOURCE  // For strtok_r

#include "zen/stdlib/database.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/json.h"
#include "zen/stdlib/yaml.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// File cache structure
typedef struct FileCacheEntry {
    char *filepath;
    RuntimeValue *content;
    time_t last_modified;
    struct FileCacheEntry *next;
} FileCacheEntry;

static FileCacheEntry *file_cache = NULL;

/**
 * Get a value from a nested object using dot notation path
 * @param obj The object to traverse
 * @param path The dot-separated path (e.g., "alice.scores.math")
 * @return The value at the path or NULL if not found
 */
RuntimeValue *database_get_nested_property(RuntimeValue *obj, const char *path)
{
    if (!obj || !path) {
        return NULL;
    }
    
    // Make a copy of the path to tokenize
    char *path_copy = memory_strdup(path);
    char *token = strtok(path_copy, ".");
    RuntimeValue *current = obj;
    
    while (token != NULL) {
        if (current->type == RV_OBJECT) {
            RuntimeValue *next = rv_object_get(current, token);
            if (!next) {
                memory_free(path_copy);
                return NULL;
            }
            current = next;
        } else if (current->type == RV_ARRAY) {
            // Try to parse as array index
            char *endptr;
            long index = strtol(token, &endptr, 10);
            if (*endptr == '\0' && index >= 0 && index < (long)current->data.array.count) {
                current = current->data.array.elements[index];
            } else {
                memory_free(path_copy);
                return NULL;
            }
        } else {
            memory_free(path_copy);
            return NULL;
        }
        token = strtok(NULL, ".");
    }
    
    memory_free(path_copy);
    return current;
}

/**
 * Set a value in a nested object using dot notation path
 * Creates nested objects as needed
 * @param obj The object to modify
 * @param path The dot-separated path
 * @param value The value to set
 * @return True if successful
 */
bool database_set_nested_property(RuntimeValue *obj, const char *path, RuntimeValue *value)
{
    if (!obj || !path || !value || obj->type != RV_OBJECT) {
        return false;
    }
    
    // Make a copy of the path to tokenize
    char *path_copy = memory_strdup(path);
    char *saveptr;
    char *token = strtok_r(path_copy, ".", &saveptr);
    
    // Find the parent object and the final key
    RuntimeValue *current = obj;
    
    while (token != NULL) {
        char *next_token = strtok_r(NULL, ".", &saveptr);
        
        if (next_token == NULL) {
            // This is the final key - set the value
            rv_object_set(current, token, value);
            memory_free(path_copy);
            return true;
        }
        
        // Navigate deeper
        RuntimeValue *next = rv_object_get(current, token);
        if (!next || next->type != RV_OBJECT) {
            // Create nested object if it doesn't exist
            next = rv_new_object();
            rv_object_set(current, token, next);
            rv_unref(next); // rv_object_set refs it
            next = rv_object_get(current, token);
        }
        
        current = next;
        token = next_token;
    }
    
    memory_free(path_copy);
    return false;
}

/**
 * Load a file with caching support
 * @param filepath The file path
 * @return The parsed content (JSON or YAML) or NULL on error
 */
RuntimeValue *database_load_file_cached(const char *filepath)
{
    // Check cache first
    FileCacheEntry *entry = file_cache;
    while (entry) {
        if (strcmp(entry->filepath, filepath) == 0) {
            // TODO: Check if file has been modified
            return rv_ref(entry->content);
        }
        entry = entry->next;
    }
    
    // Load file
    char *file_content = io_read_file_internal(filepath);
    if (!file_content) {
        return NULL;
    }
    
    RuntimeValue *content = rv_new_string(file_content);
    memory_free(file_content);
    
    // Parse based on file extension
    RuntimeValue *parsed = NULL;
    if (strstr(filepath, ".json")) {
        parsed = json_parse(content->data.string.data);
    } else if (strstr(filepath, ".yaml") || strstr(filepath, ".yml")) {
        parsed = yaml_parse(content->data.string.data);
    } else {
        // Try JSON first, then YAML
        parsed = json_parse(content->data.string.data);
        if (!parsed || parsed->type == RV_ERROR) {
            if (parsed) rv_unref(parsed);
            parsed = yaml_parse(content->data.string.data);
        }
    }
    
    rv_unref(content);
    
    if (parsed && parsed->type != RV_ERROR) {
        // Add to cache
        FileCacheEntry *new_entry = memory_alloc(sizeof(FileCacheEntry));
        new_entry->filepath = memory_strdup(filepath);
        new_entry->content = rv_ref(parsed);
        new_entry->last_modified = 0; // TODO: Get actual file modification time
        new_entry->next = file_cache;
        file_cache = new_entry;
    }
    
    return parsed;
}

/**
 * Save a value to a file
 * @param filepath The file path
 * @param value The value to save
 * @return True if successful
 */
bool database_save_file(const char *filepath, RuntimeValue *value)
{
    if (!filepath || !value) {
        return false;
    }
    
    // Convert to appropriate format based on extension
    char *content_str = NULL;
    if (strstr(filepath, ".json")) {
        content_str = json_stringify(value);
    } else if (strstr(filepath, ".yaml") || strstr(filepath, ".yml")) {
        content_str = yaml_stringify(value);
    } else {
        // Default to JSON
        content_str = json_stringify(value);
    }
    
    if (!content_str) {
        return false;
    }
    
    // Write to file
    bool success = io_write_file_internal(filepath, content_str);
    memory_free(content_str);
    
    // Update cache if successful
    if (success) {
        FileCacheEntry *entry = file_cache;
        while (entry) {
            if (strcmp(entry->filepath, filepath) == 0) {
                rv_unref(entry->content);
                entry->content = rv_ref(value);
                break;
            }
            entry = entry->next;
        }
    }
    
    return success;
}

/**
 * Clear the file cache
 */
void database_clear_cache(void)
{
    FileCacheEntry *entry = file_cache;
    while (entry) {
        FileCacheEntry *next = entry->next;
        memory_free(entry->filepath);
        rv_unref(entry->content);
        memory_free(entry);
        entry = next;
    }
    file_cache = NULL;
}