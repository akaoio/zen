/**
 * @file test_module_implementation.c
 * @brief Test the module system implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include ZEN headers
#include "zen/stdlib/io.h"
#include "zen/types/value.h"

/**
 * @brief Test JSON module loading functionality
 */
void test_json_module_loading(void) {
    printf("Testing JSON module loading...\n");
    
    // Test loading our test_config.json
    Value* json_module = zen_load_json_file("test_config.json");
    
    if (json_module) {
        printf("✓ JSON module loaded successfully\n");
        
        // Test accessing nested properties
        Value* database = value_object_get(json_module, "database");
        if (database) {
            printf("✓ Database object found\n");
            
            Value* host = value_object_get(database, "host");
            if (host && host->type == TYPE_STRING) {
                printf("✓ Host: %s\n", host->as.string->data);
            }
            
            Value* port = value_object_get(database, "port");
            if (port && port->type == TYPE_NUMBER) {
                printf("✓ Port: %.0f\n", port->as.number);
            }
            
            value_unref(database);
        }
        
        Value* version = value_object_get(json_module, "version");
        if (version && version->type == TYPE_STRING) {
            printf("✓ Version: %s\n", version->as.string->data);
            value_unref(version);
        }
        
        value_unref(json_module);
    } else {
        printf("❌ Failed to load JSON module\n");
    }
    
    printf("\n");
}

/**
 * @brief Test module path resolution
 */
void test_module_path_resolution(void) {
    printf("Testing module path resolution...\n");
    
    // Test resolving test_config (should find test_config.json)
    char* resolved = zen_resolve_module_path("test_config");
    if (resolved) {
        printf("✓ Resolved 'test_config' to: %s\n", resolved);
        free(resolved);
    } else {
        printf("❌ Failed to resolve 'test_config'\n");
    }
    
    // Test resolving with explicit extension
    resolved = zen_resolve_module_path("test_config.json");
    if (resolved) {
        printf("✓ Resolved 'test_config.json' to: %s\n", resolved);
        free(resolved);
    } else {
        printf("❌ Failed to resolve 'test_config.json'\n");
    }
    
    printf("\n");
}

/**
 * @brief Test YAML module loading (simplified)
 */
void test_yaml_module_loading(void) {
    printf("Testing YAML module loading...\n");
    
    // Create a simple YAML test file
    FILE* yaml_file = fopen("test_simple.yaml", "w");
    if (yaml_file) {
        fprintf(yaml_file, "name: zen-test\n");
        fprintf(yaml_file, "version: 1.0\n");
        fprintf(yaml_file, "enabled: true\n");
        fclose(yaml_file);
        
        // Test loading the YAML file
        Value* yaml_module = zen_load_yaml_file("test_simple.yaml");
        if (yaml_module) {
            printf("✓ YAML module loaded successfully\n");
            
            Value* name = value_object_get(yaml_module, "name");
            if (name && name->type == TYPE_STRING) {
                printf("✓ Name: %s\n", name->as.string->data);
                value_unref(name);
            }
            
            value_unref(yaml_module);
        } else {
            printf("❌ Failed to load YAML module\n");
        }
        
        // Clean up
        remove("test_simple.yaml");
    }
    
    printf("\n");
}

int main(void) {
    printf("=== ZEN Module System Test ===\n\n");
    
    test_json_module_loading();
    test_module_path_resolution();
    test_yaml_module_loading();
    
    printf("=== Module System Tests Complete ===\n");
    return 0;
}