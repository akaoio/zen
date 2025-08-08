#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Load manifest
const manifestPath = path.join(__dirname, '..', 'MANIFEST.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));

// Add missing header files
const missingHeaders = {
    "src/include/zen/config.h": {
        "description": "Configuration system header",
        "structs": [],
        "typedefs": [],
        "enums": []
    },
    "src/include/zen/core/ast_memory_pool.h": {
        "description": "AST memory pool management header",
        "structs": [],
        "typedefs": [],
        "enums": []
    },
    "src/include/zen/core/logger.h": {
        "description": "Logging system header",
        "structs": [],
        "typedefs": [],
        "enums": []
    },
    "src/include/zen/stdlib/logging.h": {
        "description": "Standard library logging functions header",
        "structs": [],
        "typedefs": [],
        "enums": []
    },
    "src/include/zen/stdlib/yaml.h": {
        "description": "YAML parsing and generation header",
        "structs": [],
        "typedefs": [],
        "enums": []
    },
    "src/include/zen/types/priority_queue.h": {
        "description": "Priority queue data structure header",
        "structs": [],
        "typedefs": [],
        "enums": []
    },
    "src/include/zen/types/set.h": {
        "description": "Set data structure header",
        "structs": [],
        "typedefs": [],
        "enums": []
    }
};

// Add missing headers to manifest
for (const [headerPath, headerInfo] of Object.entries(missingHeaders)) {
    if (!manifest.headers[headerPath]) {
        console.log(`Adding header: ${headerPath}`);
        manifest.headers[headerPath] = headerInfo;
    }
}

// Add missing functions
const missingFunctions = {
    "src/core/config.c": [
        {
            name: "config_get",
            signature: "const ZenConfig* config_get(void)",
            description: "Get the global configuration",
            returns: "const ZenConfig*"
        },
        {
            name: "config_get_string",
            signature: "const char* config_get_string(const char* key)",
            description: "Get a string configuration value",
            returns: "const char*"
        }
    ],
    "src/core/error.c": [
        {
            name: "error_get_message",
            signature: "const char* error_get_message(const Value* error)",
            description: "Get error message from an error Value",
            returns: "const char*"
        }
    ],
    "src/stdlib/logging.c": [
        {
            name: "logging_level_string",
            signature: "const char* logging_level_string(ZenLogLevel level)",
            description: "Convert log level to string representation",
            returns: "const char*"
        }
    ],
    "src/stdlib/stdlib.c": [
        {
            name: "stdlib_get",
            signature: "const ZenStdlibFunction* stdlib_get(const char* name)",
            description: "Get stdlib function by name",
            returns: "const ZenStdlibFunction*"
        },
        {
            name: "stdlib_get_all",
            signature: "const ZenStdlibFunction* stdlib_get_all()",
            description: "Get all stdlib functions",
            returns: "const ZenStdlibFunction*"
        }
    ],
    "src/types/value.c": [
        {
            name: "value_type_name",
            signature: "const char* value_type_name(ValueType type)",
            description: "Get string name of value type",
            returns: "const char*",
            doxygen_required: true
        }
    ]
};

// Add missing functions to files
for (const [filePath, functions] of Object.entries(missingFunctions)) {
    if (manifest.files[filePath]) {
        for (const func of functions) {
            const existing = manifest.files[filePath].functions.find(f => f.name === func.name);
            if (!existing) {
                console.log(`Adding function to ${filePath}: ${func.name}`);
                manifest.files[filePath].functions.push(func);
            }
        }
        // Sort functions
        manifest.files[filePath].functions.sort((a, b) => a.name.localeCompare(b.name));
    }
}

// Write updated manifest
fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');

console.log('\nMANIFEST.json updated with missing entries!');