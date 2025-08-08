/**
 * @file config.c
 * @brief Implementation of centralized configuration system
 */

#include "zen/config.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global configuration instance
static ZenConfig g_config;
static bool g_config_initialized = false;

// Default configuration values
static const ZenConfig DEFAULT_CONFIG = {
    .max_memory_usage = ZEN_MAX_MEMORY_USAGE_MB * 1024 * 1024,
    .max_string_length = ZEN_MAX_STRING_LENGTH,
    .max_array_size = ZEN_MAX_ARRAY_SIZE,
    .default_array_capacity = ZEN_DEFAULT_ARRAY_CAPACITY,

    .max_recursion_depth = ZEN_MAX_RECURSION_DEPTH,
    .max_execution_time_ms = ZEN_MAX_EXECUTION_TIME_MS,
    .gc_frequency_ms = ZEN_GC_FREQUENCY_MS,

    .logic_max_proof_steps = ZEN_LOGIC_MAX_PROOF_STEPS,
    .logic_max_statement_length = ZEN_LOGIC_MAX_STATEMENT_LENGTH,
    .logic_proof_timeout_us = ZEN_LOGIC_PROOF_VERIFY_TIMEOUT_US,

    .enable_debugging = false,
    .enable_profiling = false,
    .enable_gc_logging = false,
    .enable_strict_mode = false,
    .enable_optimization = true,

    .module_path = "",
    .config_file = ""};

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Parse boolean value from string
 * @param str String to parse ("true", "false", "1", "0", "yes", "no")
 * @return Boolean value
 */
static bool config_parse_bool(const char *str)
{
    if (!str)
        return false;

    if (strcmp(str, "true") == 0 || strcmp(str, "1") == 0 || strcmp(str, "yes") == 0 ||
        strcmp(str, "on") == 0) {
        return true;
    }

    return false;
}

/**
 * @brief Parse unsigned integer from string with error checking
 * @param str String to parse
 * @param result Output parameter for result
 * @return True if parsing successful
 */
static bool config_parse_uint(const char *str, uint32_t *result)
{
    if (!str || !result)
        return false;

    char *endptr;
    unsigned long val = strtoul(str, &endptr, 10);

    if (*endptr != '\0' || val > UINT32_MAX) {
        return false;
    }

    *result = (uint32_t)val;
    return true;
}

/**
 * @brief Parse size_t from string with error checking
 * @param str String to parse
 * @param result Output parameter for result
 * @return True if parsing successful
 */
static bool config_parse_size(const char *str, size_t *result)
{
    if (!str || !result)
        return false;

    char *endptr;
    unsigned long long val = strtoull(str, &endptr, 10);

    if (*endptr != '\0' || val > SIZE_MAX) {
        return false;
    }

    *result = (size_t)val;
    return true;
}

/**
 * @brief Copy string with length validation
 * @param dest Destination buffer
 * @param src Source string
 * @param max_len Maximum length including null terminator
 * @return True if copy successful
 */
static bool config_safe_strcpy(char *dest, const char *src, size_t max_len)
{
    if (!dest || !src || max_len == 0)
        return false;

    size_t src_len = strlen(src);
    if (src_len >= max_len)
        return false;

    strcpy(dest, src);
    return true;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

/**
 * @brief Get the global ZEN configuration
 * @return Pointer to global configuration structure
 */
const ZenConfig *config_get(void)
{
    if (!g_config_initialized) {
        config_init();
    }
    return &g_config;
}

/**
 * @brief Initialize configuration with defaults
 * @return True if initialization successful
 */
bool config_init(void)
{
    if (g_config_initialized) {
        return true;
    }

    // Initialize with default values
    g_config = DEFAULT_CONFIG;

    // Load from environment variables if available
    config_load_from_env();

    g_config_initialized = true;
    return true;
}

/**
 * @brief Load configuration from environment variables
 * @return True if loaded successfully
 */
bool config_load_from_env(void)
{
    const char *env_value;
    uint32_t uint_val __attribute__((unused));
    size_t size_val;

    // Load log level
    env_value = getenv(ZEN_ENV_LOG_LEVEL);
    if (env_value) {
        // Log level would be handled by logging system
    }

    // Load max memory
    env_value = getenv(ZEN_ENV_MAX_MEMORY);
    if (env_value && config_parse_size(env_value, &size_val)) {
        g_config.max_memory_usage = size_val * 1024 * 1024;  // Convert MB to bytes
    }

    // Load config file path
    env_value = getenv(ZEN_ENV_CONFIG_FILE);
    if (env_value) {
        config_safe_strcpy(g_config.config_file, env_value, sizeof(g_config.config_file));
    }

    // Load module path
    env_value = getenv(ZEN_ENV_MODULE_PATH);
    if (env_value) {
        config_safe_strcpy(g_config.module_path, env_value, sizeof(g_config.module_path));
    }

    // Load debug flag
    env_value = getenv(ZEN_ENV_DEBUG);
    if (env_value) {
        g_config.enable_debugging = config_parse_bool(env_value);
    }

    // Load profile flag
    env_value = getenv(ZEN_ENV_PROFILE);
    if (env_value) {
        g_config.enable_profiling = config_parse_bool(env_value);
    }

    return true;
}

/**
 * @brief Load configuration from file
 * @param filename Configuration file path
 * @return True if loaded successfully
 */
bool config_load_from_file(const char *filename)
{
    if (!filename)
        return false;

    FILE *file = fopen(filename, "r");
    if (!file)
        return false;

    char line[512];
    char key[256];
    char value[256];

    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // Parse key=value format
        if (sscanf(line, "%255[^=]=%255s", key, value) == 2) {
            config_set(key, value);
        }
    }

    fclose(file);
    return true;
}

/**
 * @brief Set a configuration value by name
 * @param key Configuration key name
 * @param value Configuration value (string format)
 * @return True if set successfully
 */
bool config_set(const char *key, const char *value)
{
    if (!key || !value)
        return false;

    uint32_t uint_val;
    size_t size_val;

    // Memory configuration
    if (strcmp(key, "max_memory_usage") == 0) {
        if (config_parse_size(value, &size_val)) {
            g_config.max_memory_usage = size_val;
            return true;
        }
    } else if (strcmp(key, "max_string_length") == 0) {
        if (config_parse_size(value, &size_val)) {
            g_config.max_string_length = size_val;
            return true;
        }
    } else if (strcmp(key, "max_array_size") == 0) {
        if (config_parse_size(value, &size_val)) {
            g_config.max_array_size = size_val;
            return true;
        }
    } else if (strcmp(key, "default_array_capacity") == 0) {
        if (config_parse_size(value, &size_val)) {
            g_config.default_array_capacity = size_val;
            return true;
        }
    }

    // Performance configuration
    else if (strcmp(key, "max_recursion_depth") == 0) {
        if (config_parse_uint(value, &uint_val)) {
            g_config.max_recursion_depth = uint_val;
            return true;
        }
    } else if (strcmp(key, "max_execution_time_ms") == 0) {
        if (config_parse_uint(value, &uint_val)) {
            g_config.max_execution_time_ms = uint_val;
            return true;
        }
    } else if (strcmp(key, "gc_frequency_ms") == 0) {
        if (config_parse_uint(value, &uint_val)) {
            g_config.gc_frequency_ms = uint_val;
            return true;
        }
    }

    // Logic system configuration
    else if (strcmp(key, "logic_max_proof_steps") == 0) {
        if (config_parse_size(value, &size_val)) {
            g_config.logic_max_proof_steps = size_val;
            return true;
        }
    } else if (strcmp(key, "logic_max_statement_length") == 0) {
        if (config_parse_size(value, &size_val)) {
            g_config.logic_max_statement_length = size_val;
            return true;
        }
    } else if (strcmp(key, "logic_proof_timeout_us") == 0) {
        if (config_parse_uint(value, &uint_val)) {
            g_config.logic_proof_timeout_us = uint_val;
            return true;
        }
    }

    // Feature flags
    else if (strcmp(key, "enable_debugging") == 0) {
        g_config.enable_debugging = config_parse_bool(value);
        return true;
    } else if (strcmp(key, "enable_profiling") == 0) {
        g_config.enable_profiling = config_parse_bool(value);
        return true;
    } else if (strcmp(key, "enable_gc_logging") == 0) {
        g_config.enable_gc_logging = config_parse_bool(value);
        return true;
    } else if (strcmp(key, "enable_strict_mode") == 0) {
        g_config.enable_strict_mode = config_parse_bool(value);
        return true;
    } else if (strcmp(key, "enable_optimization") == 0) {
        g_config.enable_optimization = config_parse_bool(value);
        return true;
    }

    // Path configuration
    else if (strcmp(key, "module_path") == 0) {
        return config_safe_strcpy(g_config.module_path, value, sizeof(g_config.module_path));
    } else if (strcmp(key, "config_file") == 0) {
        return config_safe_strcpy(g_config.config_file, value, sizeof(g_config.config_file));
    }

    return false;  // Unknown key
}

/**
 * @brief Get a configuration value by name
 * @param key Configuration key name
 * @return Configuration value or NULL if not found
 */
const char *config_get_string(const char *key)
{
    if (!key)
        return NULL;

    static char buffer[256];  // Static buffer for return values

    if (strcmp(key, "max_memory_usage") == 0) {
        snprintf(buffer, sizeof(buffer), "%zu", g_config.max_memory_usage);
        return buffer;
    } else if (strcmp(key, "max_string_length") == 0) {
        snprintf(buffer, sizeof(buffer), "%zu", g_config.max_string_length);
        return buffer;
    } else if (strcmp(key, "enable_debugging") == 0) {
        return g_config.enable_debugging ? "true" : "false";
    } else if (strcmp(key, "enable_profiling") == 0) {
        return g_config.enable_profiling ? "true" : "false";
    } else if (strcmp(key, "module_path") == 0) {
        return g_config.module_path[0] ? g_config.module_path : NULL;
    } else if (strcmp(key, "config_file") == 0) {
        return g_config.config_file[0] ? g_config.config_file : NULL;
    }

    // Add more string getters as needed

    return NULL;
}

/**
 * @brief Cleanup configuration system
 */
void config_cleanup(void)
{
    // Clear sensitive configuration data
    memset(&g_config, 0, sizeof(g_config));
    g_config_initialized = false;
}