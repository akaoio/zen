#ifndef ZEN_CONFIG_H
#define ZEN_CONFIG_H

/**
 * @file config.h
 * @brief Centralized configuration system for ZEN interpreter
 *
 * This header consolidates all magic numbers, buffer sizes, and limits
 * into named constants for better maintainability and configurability.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// BUFFER SIZES AND LIMITS
// ============================================================================

// Logging system limits
#define ZEN_MAX_LOG_MESSAGE_SIZE    1024
#define ZEN_MAX_LOG_FILENAME_SIZE   256
#define ZEN_DEFAULT_LOG_BUFFER_SIZE 4096

// String processing limits
#define ZEN_MAX_STRING_LENGTH        (1024 * 1024)  // 1MB max string
#define ZEN_MAX_IDENTIFIER_LENGTH    128
#define ZEN_MAX_NUMBER_STRING_LENGTH 64
#define ZEN_MAX_FORMAT_BUFFER_SIZE   512

// Error handling limits
#define ZEN_MAX_ERROR_MESSAGE_SIZE 512
#define ZEN_MAX_ERROR_CONTEXT_SIZE 256
#define ZEN_MAX_ERROR_BUFFER_SIZE  256

// Parser and lexer limits
#define ZEN_MAX_TOKEN_LENGTH        256
#define ZEN_MAX_OPERATOR_LENGTH     4
#define ZEN_MAX_KEYWORD_LENGTH      16
#define ZEN_MAX_NESTING_DEPTH       128
#define ZEN_MAX_FUNCTION_PARAMETERS 64
#define ZEN_MAX_ARRAY_SIZE          (1024 * 1024)  // 1M elements max

// Memory system limits
#define ZEN_DEFAULT_ARRAY_CAPACITY  16
#define ZEN_DEFAULT_OBJECT_CAPACITY 16
#define ZEN_MAX_REFERENCE_COUNT     UINT32_MAX
#define ZEN_MEMORY_ALIGNMENT        8

// Logic system limits (from recent logic.c additions)
#define ZEN_LOGIC_DEFAULT_THEOREM_CAPACITY 16
#define ZEN_LOGIC_DEFAULT_AXIOM_CAPACITY   16
#define ZEN_LOGIC_DEFAULT_PROOF_CAPACITY   16
#define ZEN_LOGIC_MAX_PROOF_STEPS          1000
#define ZEN_LOGIC_MAX_STATEMENT_LENGTH     512

// File system limits
#define ZEN_MAX_FILENAME_LENGTH      260   // Windows compatible
#define ZEN_MAX_PATH_LENGTH          4096  // Unix compatible
#define ZEN_DEFAULT_FILE_BUFFER_SIZE 8192

// Performance and optimization limits
#define ZEN_MAX_CACHE_SIZE          (64 * 1024 * 1024)  // 64MB cache
#define ZEN_DEFAULT_HASH_TABLE_SIZE 256
#define ZEN_MAX_RECURSION_DEPTH     1000

// ============================================================================
// TIMEOUT AND PERFORMANCE LIMITS
// ============================================================================

// Logic system performance targets (microseconds)
#define ZEN_LOGIC_PROOF_VERIFY_TIMEOUT_US  1000000  // 1 second
#define ZEN_LOGIC_THEOREM_PROVE_TIMEOUT_US 5000000  // 5 seconds
#define ZEN_LOGIC_MAX_INFERENCE_TIME_US    100000   // 100ms

// General performance limits
#define ZEN_MAX_EXECUTION_TIME_MS 30000  // 30 seconds max execution
#define ZEN_MAX_MEMORY_USAGE_MB   512    // 512MB max memory
#define ZEN_GC_FREQUENCY_MS       1000   // Garbage collect every second

// ============================================================================
// FILE EXTENSIONS AND PATTERNS
// ============================================================================

// ZEN file extensions
#define ZEN_FILE_EXTENSION     ".zen"
#define ZEN_LIBRARY_EXTENSION  ".zenlib"
#define ZEN_MODULE_EXTENSION   ".zenmod"
#define ZEN_COMPILED_EXTENSION ".zenc"

// Configuration file patterns
#define ZEN_CONFIG_FILE_PATTERN    "zen.config"
#define ZEN_LOCAL_CONFIG_PATTERN   ".zenconfig"
#define ZEN_PROJECT_CONFIG_PATTERN "zen.project"

// ============================================================================
// MAGIC VALUES AND CONSTANTS
// ============================================================================

// Hash table and data structure magic values
#define ZEN_HASH_SEED               0x9e3779b9
#define ZEN_HASH_MULTIPLIER         0x85ebca6b
#define ZEN_FIBONACCI_HASH_CONSTANT 0x9e3779b9

// Special values
#define ZEN_INVALID_INDEX    SIZE_MAX
#define ZEN_INVALID_POSITION UINT32_MAX
#define ZEN_INVALID_LINE     UINT32_MAX
#define ZEN_INVALID_COLUMN   UINT32_MAX

// Version and compatibility
#define ZEN_VERSION_MAJOR         1
#define ZEN_VERSION_MINOR         0
#define ZEN_VERSION_PATCH         0
#define ZEN_VERSION_STRING        "1.0.0"
#define ZEN_MIN_SUPPORTED_VERSION "1.0.0"

// ============================================================================
// ENVIRONMENT VARIABLE NAMES
// ============================================================================

#define ZEN_ENV_LOG_LEVEL   "ZEN_LOG_LEVEL"
#define ZEN_ENV_MAX_MEMORY  "ZEN_MAX_MEMORY"
#define ZEN_ENV_CONFIG_FILE "ZEN_CONFIG_FILE"
#define ZEN_ENV_MODULE_PATH "ZEN_MODULE_PATH"
#define ZEN_ENV_DEBUG       "ZEN_DEBUG"
#define ZEN_ENV_PROFILE     "ZEN_PROFILE"

// ============================================================================
// CONFIGURATION STRUCTURE (RUNTIME CONFIGURABLE)
// ============================================================================

/**
 * @brief Runtime configuration structure
 *
 * This structure allows runtime modification of limits and behavior
 * while providing compile-time defaults from the constants above.
 */
typedef struct ZenConfig {
    // Memory limits (runtime configurable)
    size_t max_memory_usage;
    size_t max_string_length;
    size_t max_array_size;
    size_t default_array_capacity;

    // Performance limits (runtime configurable)
    uint32_t max_recursion_depth;
    uint32_t max_execution_time_ms;
    uint32_t gc_frequency_ms;

    // Logic system limits (runtime configurable)
    size_t logic_max_proof_steps;
    size_t logic_max_statement_length;
    uint32_t logic_proof_timeout_us;

    // Feature flags (runtime configurable)
    bool enable_debugging;
    bool enable_profiling;
    bool enable_gc_logging;
    bool enable_strict_mode;
    bool enable_optimization;

    // File system configuration
    char module_path[ZEN_MAX_PATH_LENGTH];
    char config_file[ZEN_MAX_PATH_LENGTH];

} ZenConfig;

// ============================================================================
// CONFIGURATION API
// ============================================================================

/**
 * @brief Get the global ZEN configuration
 * @return Pointer to global configuration structure
 */
const ZenConfig *config_get(void);

/**
 * @brief Initialize configuration with defaults
 * @return True if initialization successful
 */
bool config_init(void);

/**
 * @brief Load configuration from environment variables
 * @return True if loaded successfully
 */
bool config_load_from_env(void);

/**
 * @brief Load configuration from file
 * @param filename Configuration file path
 * @return True if loaded successfully
 */
bool config_load_from_file(const char *filename);

/**
 * @brief Set a configuration value by name
 * @param key Configuration key name
 * @param value Configuration value (string format)
 * @return True if set successfully
 */
bool config_set(const char *key, const char *value);

/**
 * @brief Get a configuration value by name
 * @param key Configuration key name
 * @return Configuration value or NULL if not found
 */
const char *config_get_string(const char *key);

/**
 * @brief Cleanup configuration system
 */
void config_cleanup(void);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

// Memory allocation with configured limits
#define ZEN_ALLOC_CHECK_LIMIT(size) ((size) <= config_get()->max_memory_usage)

// String length validation
#define ZEN_STRING_LENGTH_OK(len) ((len) <= config_get()->max_string_length)

// Array size validation
#define ZEN_ARRAY_SIZE_OK(size) ((size) <= config_get()->max_array_size)

// Recursion depth check
#define ZEN_RECURSION_DEPTH_OK(depth) ((depth) < config_get()->max_recursion_depth)

#endif  // ZEN_CONFIG_H