#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Load manifest
const manifestPath = path.join(__dirname, '..', 'MANIFEST.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));

console.log('Fixing TODO descriptions in MANIFEST.json...\n');

// Fix file descriptions
const fileDescriptions = {
    "src/types/set.c": "Set data structure implementation with hash-based uniqueness",
    "src/types/priority_queue.c": "Priority queue implementation with min-heap data structure",
    "src/core/ast_memory_pool.c": "Memory pool allocator for AST nodes to improve performance",
    "src/core/config.c": "Configuration management system for ZEN interpreter",
    "src/core/logger.c": "Logging system with categories and level filtering"
};

// Fix function descriptions
const functionDescriptions = {
    "src/types/set.c": {
        "datastructures_set_create": "Create internal set data structure",
        "datastructures_set_free": "Free internal set data structure and all elements",
        "datastructures_set_value_to_key": "Convert a Value to a string key for set storage"
    },
    "src/types/priority_queue.c": {
        "datastructures_priority_queue_create": "Create internal priority queue data structure",
        "datastructures_priority_queue_ensure_capacity": "Ensure priority queue has enough capacity for new elements",
        "datastructures_priority_queue_free": "Free internal priority queue and all elements",
        "datastructures_priority_queue_heap_down": "Restore heap property by moving element down",
        "datastructures_priority_queue_heap_up": "Restore heap property by moving element up",
        "datastructures_priority_queue_left_child": "Get index of left child in heap",
        "datastructures_priority_queue_parent": "Get index of parent in heap",
        "datastructures_priority_queue_right_child": "Get index of right child in heap"
    },
    "src/core/visitor.c": {
        "value_to_ast": "Convert a Value back to its AST representation"
    },
    "src/core/ast_memory_pool.c": {
        "ast_pool_manager_init": "Initialize the AST memory pool manager",
        "ast_pool_manager_cleanup": "Clean up the AST memory pool manager",
        "ast_pool_select_type": "Select pool type based on AST node type",
        "ast_pool_select_pool_for_type": "Select specific pool for given AST type",
        "ast_pool_expand": "Expand pool capacity when full",
        "ast_pool_cleanup": "Clean up a specific pool",
        "ast_pool_alloc": "Allocate memory from pool",
        "ast_pool_free": "Return memory to pool",
        "ast_pool_get_stats": "Get pool usage statistics",
        "ast_pool_init": "Initialize a single pool",
        "ast_pool_init_global": "Initialize global pool system",
        "ast_pool_alloc_global": "Allocate from global pool",
        "ast_pool_free_global": "Free to global pool",
        "ast_pool_cleanup_global": "Clean up global pool system",
        "ast_pool_get_stats_global": "Get global pool statistics",
        "ast_pool_set_enabled": "Enable or disable pooling",
        "ast_pool_is_enabled": "Check if pooling is enabled",
        "ast_pool_get_pool_metrics": "Get detailed pool metrics",
        "ast_pool_debug_status": "Print pool debug information",
        "ast_pool_is_pooled_node": "Check if node is from pool",
        "ast_pool_global_init": "Initialize global pool (alias)",
        "ast_pool_global_cleanup": "Clean up global pool (alias)",
        "ast_pool_alloc_node": "Allocate AST node from pool",
        "ast_pool_free_node": "Free AST node to pool",
        "ast_pool_defragment": "Defragment pool memory"
    },
    "src/core/config.c": {
        "config_init": "Initialize configuration system",
        "config_load_from_env": "Load configuration from environment variables",
        "config_load_from_file": "Load configuration from file",
        "config_set": "Set a configuration value",
        "config_cleanup": "Clean up configuration system",
        "config_get": "Get the global configuration",
        "config_get_string": "Get a string configuration value"
    },
    "src/core/logger.c": {
        "logger_init": "Initialize logging system",
        "logger_set_level": "Set minimum logging level",
        "logger_get_level": "Get current logging level",
        "logger_set_categories": "Set enabled log categories",
        "logger_get_categories": "Get enabled log categories",
        "logger_set_file": "Set log output file",
        "logger_should_log": "Check if message should be logged",
        "logger_log": "Log a message with category and level",
        "logger_cleanup": "Clean up logging system"
    }
};

// Update file descriptions
for (const [filePath, description] of Object.entries(fileDescriptions)) {
    if (manifest.files[filePath] && manifest.files[filePath].description.includes("TODO")) {
        console.log(`Updating description for ${filePath}`);
        manifest.files[filePath].description = description;
    }
}

// Update function descriptions
for (const [filePath, functions] of Object.entries(functionDescriptions)) {
    if (manifest.files[filePath]) {
        for (const [funcName, description] of Object.entries(functions)) {
            const func = manifest.files[filePath].functions.find(f => f.name === funcName);
            if (func && func.description.includes("TODO")) {
                console.log(`  Updating description for ${funcName}()`);
                func.description = description;
            }
        }
    }
}

// Also add missing functions that might be internal/static but still need descriptions
const additionalFunctions = {
    "src/stdlib/stdlib.c": {
        "stdlib_count": "Get count of available stdlib functions"
    },
    "src/stdlib/logging.c": {
        "logging_get_timestamp": "Get formatted timestamp for log messages",
        "logging_parse_level": "Parse log level from string",
        "logging_should_log": "Check if message should be logged at current level",
        "logging_level_string": "Convert log level to string representation"
    }
};

// Update additional function descriptions
for (const [filePath, functions] of Object.entries(additionalFunctions)) {
    if (manifest.files[filePath]) {
        for (const [funcName, description] of Object.entries(functions)) {
            const func = manifest.files[filePath].functions.find(f => f.name === funcName);
            if (func && func.description.includes("TODO")) {
                console.log(`  Updating description for ${funcName}()`);
                func.description = description;
            }
        }
    }
}

// Write updated manifest
fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');

console.log('\nMANIFEST.json descriptions updated successfully!');