/*
 * database.h
 * Database-like operations for ZEN
 */

#ifndef ZEN_STDLIB_DATABASE_H
#define ZEN_STDLIB_DATABASE_H

#include "zen/core/runtime_value.h"
#include <stdbool.h>
#include <time.h>

/**
 * Get a value from a nested object using dot notation path
 */
RuntimeValue *database_get_nested_property(RuntimeValue *obj, const char *path);

/**
 * Set a value in a nested object using dot notation path
 * Creates nested objects as needed
 */
bool database_set_nested_property(RuntimeValue *obj, const char *path, RuntimeValue *value);

/**
 * Load a file with caching support
 */
RuntimeValue *database_load_file_cached(const char *filepath);

/**
 * Save a value to a file
 */
bool database_save_file(const char *filepath, RuntimeValue *value);

/**
 * Clear the file cache
 */
void database_clear_cache(void);

#endif // ZEN_STDLIB_DATABASE_H