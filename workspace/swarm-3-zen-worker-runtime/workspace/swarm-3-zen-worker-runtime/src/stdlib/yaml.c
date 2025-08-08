/**
 * @file yaml.c
 * @brief YAML parsing and generation using libyaml
 * 
 * This file implements YAML parsing and generation for the ZEN language
 * using the libyaml library. Functions follow MANIFEST.json specifications.
 * 
 * Pattern follows json.c for consistency with ZEN stdlib conventions.
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/yaml.h"
#include "zen/stdlib/io.h"
#include "zen/types/value.h"
#include "zen/types/array.h"
#include "zen/types/object.h"
#include "zen/core/memory.h"
#include "zen/core/error.h"
#include <yaml.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static Value* parse_yaml_document(yaml_parser_t* parser);
static Value* parse_yaml_node(yaml_parser_t* parser, yaml_event_t* event);
static Value* parse_yaml_sequence(yaml_parser_t* parser);
static Value* parse_yaml_mapping(yaml_parser_t* parser);
static Value* parse_yaml_scalar(yaml_event_t* event);
static bool emit_yaml_value(yaml_emitter_t* emitter, const Value* value);
static bool emit_yaml_sequence(yaml_emitter_t* emitter, const Value* array);
static bool emit_yaml_mapping(yaml_emitter_t* emitter, const Value* object);

/**
 * @brief Parse YAML string to Value
 * @param yaml_string YAML string to parse
 * @return Newly allocated Value representing the YAML data, or error Value on failure
 */
Value* yaml_parse(const char* yaml_string) {
    if (!yaml_string) {
        return error_new("YAML string is NULL");
    }
    
    yaml_parser_t parser;
    Value* result = NULL;
    
    // Initialize parser
    if (!yaml_parser_initialize(&parser)) {
        return error_new("Failed to initialize YAML parser");
    }
    
    // Set input string
    yaml_parser_set_input_string(&parser, (const unsigned char*)yaml_string, strlen(yaml_string));
    
    // Parse the document
    result = parse_yaml_document(&parser);
    
    // Clean up parser
    yaml_parser_delete(&parser);
    
    if (!result) {
        return error_new("Failed to parse YAML document");
    }
    
    return result;
}

/**
 * @brief Convert Value to YAML string
 * @param value Value to convert to YAML
 * @return Newly allocated YAML string, or NULL on error
 */
char* yaml_stringify(const Value* value) {
    if (!value) {
        return memory_strdup("null");
    }
    
    yaml_emitter_t emitter;
    char* buffer = NULL;
    size_t buffer_size = 0;
    FILE* output = open_memstream(&buffer, &buffer_size);
    
    if (!output) {
        return NULL;
    }
    
    // Initialize emitter
    if (!yaml_emitter_initialize(&emitter)) {
        fclose(output);
        return NULL;
    }
    
    // Set output to memory stream
    yaml_emitter_set_output_file(&emitter, output);
    
    // Start document
    yaml_event_t event;
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        return NULL;
    }
    
    yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        return NULL;
    }
    
    // Emit the value
    if (!emit_yaml_value(&emitter, value)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        return NULL;
    }
    
    // End document
    yaml_document_end_event_initialize(&event, 1);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        return NULL;
    }
    
    yaml_stream_end_event_initialize(&event);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        return NULL;
    }
    
    // Clean up
    yaml_emitter_delete(&emitter);
    fclose(output);
    
    return buffer;
}

/**
 * @brief Load and parse a YAML file
 * @param filepath Path to YAML file
 * @return Newly allocated Value representing the YAML data, or error Value on failure
 */
Value* yaml_load_file(const char* filepath) {
    if (!filepath) {
        return error_new("File path is NULL");
    }
    
    // Check if file exists using io functions
    extern bool io_file_exists_internal(const char* filepath);
    if (!io_file_exists_internal(filepath)) {
        return error_new_formatted("File does not exist: %s", filepath);
    }
    
    // Read file content
    extern char* io_read_file_internal(const char* filepath);
    char* content = io_read_file_internal(filepath);
    if (!content) {
        return error_new_formatted("Failed to read file: %s", filepath);
    }
    
    // Parse YAML content
    Value* result = yaml_parse(content);
    memory_free(content);
    
    return result;
}

/**
 * @brief Load and parse YAML file wrapper function for stdlib
 * @param args Array of Value arguments (filename)
 * @param argc Number of arguments
 * @return Parsed YAML Value or error
 */
Value* yaml_load_file_wrapper(Value** args, size_t argc) {
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return error_new("loadYamlFile requires a filename string");
    }
    
    return yaml_load_file(args[0]->as.string->data);
}

// Helper functions for parsing

static Value* parse_yaml_document(yaml_parser_t* parser) {
    yaml_event_t event;
    Value* result = NULL;
    
    // Parse through events to find document start
    while (true) {
        if (!yaml_parser_parse(parser, &event)) {
            return error_new("YAML parser error");
        }
        
        switch (event.type) {
            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
                yaml_event_delete(&event);
                continue;
                
            case YAML_DOCUMENT_END_EVENT:
            case YAML_STREAM_END_EVENT:
                yaml_event_delete(&event);
                return result ? result : value_new_null();
                
            case YAML_SCALAR_EVENT:
            case YAML_SEQUENCE_START_EVENT:
            case YAML_MAPPING_START_EVENT:
                result = parse_yaml_node(parser, &event);
                yaml_event_delete(&event);
                return result;
                
            case YAML_ALIAS_EVENT:
                // Aliases not supported in this simplified implementation
                yaml_event_delete(&event);
                return error_new("YAML aliases not supported");
                
            default:
                yaml_event_delete(&event);
                continue;
        }
    }
}

static Value* parse_yaml_node(yaml_parser_t* parser, yaml_event_t* event) {
    switch (event->type) {
        case YAML_SCALAR_EVENT:
            return parse_yaml_scalar(event);
            
        case YAML_SEQUENCE_START_EVENT:
            return parse_yaml_sequence(parser);
            
        case YAML_MAPPING_START_EVENT:
            return parse_yaml_mapping(parser);
            
        default:
            return error_new("Unexpected YAML event");
    }
}

static Value* parse_yaml_sequence(yaml_parser_t* parser) {
    Value* array = value_new(VALUE_ARRAY);
    if (!array) {
        return error_memory_allocation();
    }
    
    yaml_event_t event;
    
    while (true) {
        if (!yaml_parser_parse(parser, &event)) {
            value_unref(array);
            return error_new("YAML parser error in sequence");
        }
        
        if (event.type == YAML_SEQUENCE_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }
        
        Value* item = parse_yaml_node(parser, &event);
        yaml_event_delete(&event);
        
        if (!item) {
            value_unref(array);
            return error_new("Failed to parse sequence item");
        }
        
        if (error_is_error(item)) {
            value_unref(array);
            return item;
        }
        
        array_push(array, item);
    }
    
    return array;
}

static Value* parse_yaml_mapping(yaml_parser_t* parser) {
    Value* object = value_new(VALUE_OBJECT);
    if (!object) {
        return error_memory_allocation();
    }
    
    yaml_event_t event;
    
    while (true) {
        // Check for mapping end
        if (!yaml_parser_parse(parser, &event)) {
            value_unref(object);
            return error_new("YAML parser error in mapping");
        }
        
        if (event.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }
        
        // Parse key (must be scalar)
        if (event.type != YAML_SCALAR_EVENT) {
            yaml_event_delete(&event);
            value_unref(object);
            return error_new("YAML mapping key must be scalar");
        }
        
        Value* key = parse_yaml_scalar(&event);
        yaml_event_delete(&event);
        
        if (!key || key->type != VALUE_STRING) {
            if (key) value_unref(key);
            value_unref(object);
            return error_new("Invalid YAML mapping key");
        }
        
        // Parse value
        if (!yaml_parser_parse(parser, &event)) {
            value_unref(key);
            value_unref(object);
            return error_new("YAML parser error parsing mapping value");
        }
        
        Value* value = parse_yaml_node(parser, &event);
        yaml_event_delete(&event);
        
        if (!value) {
            value_unref(key);
            value_unref(object);
            return error_new("Failed to parse mapping value");
        }
        
        if (error_is_error(value)) {
            value_unref(key);
            value_unref(object);
            return value;
        }
        
        // Add to object
        object_set(object, key->as.string->data, value);
        value_unref(key);
    }
    
    return object;
}

static Value* parse_yaml_scalar(yaml_event_t* event) {
    const char* value_str = (const char*)event->data.scalar.value;
    size_t length = event->data.scalar.length;
    
    if (!value_str || length == 0) {
        return value_new_string("");
    }
    
    // Handle null values
    if (strcmp(value_str, "null") == 0 || strcmp(value_str, "~") == 0 || 
        strcmp(value_str, "") == 0) {
        return value_new_null();
    }
    
    // Handle boolean values
    if (strcmp(value_str, "true") == 0 || strcmp(value_str, "yes") == 0 || 
        strcmp(value_str, "on") == 0) {
        return value_new_boolean(true);
    }
    if (strcmp(value_str, "false") == 0 || strcmp(value_str, "no") == 0 || 
        strcmp(value_str, "off") == 0) {
        return value_new_boolean(false);
    }
    
    // Try to parse as number
    char* endptr;
    double num_value = strtod(value_str, &endptr);
    if (*endptr == '\0') {
        return value_new_number(num_value);
    }
    
    // Default to string
    return value_new_string(value_str);
}

// Helper functions for emitting YAML

static bool emit_yaml_value(yaml_emitter_t* emitter, const Value* value) {
    if (!value) {
        yaml_event_t event;
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)"null", -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        return yaml_emitter_emit(emitter, &event);
    }
    
    switch (value->type) {
        case VALUE_NULL: {
            yaml_event_t event;
            yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)"null", -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
            return yaml_emitter_emit(emitter, &event);
        }
        
        case VALUE_BOOLEAN: {
            yaml_event_t event;
            const char* bool_str = value->as.boolean ? "true" : "false";
            yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)bool_str, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
            return yaml_emitter_emit(emitter, &event);
        }
        
        case VALUE_NUMBER: {
            yaml_event_t event;
            char num_buffer[32];
            if (value->as.number == floor(value->as.number)) {
                snprintf(num_buffer, sizeof(num_buffer), "%.0f", value->as.number);
            } else {
                snprintf(num_buffer, sizeof(num_buffer), "%g", value->as.number);
            }
            yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)num_buffer, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
            return yaml_emitter_emit(emitter, &event);
        }
        
        case VALUE_STRING: {
            yaml_event_t event;
            const char* str = (value->as.string && value->as.string->data) ? value->as.string->data : "";
            yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)str, -1, 1, 1, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
            return yaml_emitter_emit(emitter, &event);
        }
        
        case VALUE_ARRAY:
            return emit_yaml_sequence(emitter, value);
            
        case VALUE_OBJECT:
            return emit_yaml_mapping(emitter, value);
            
        default:
            return false;
    }
}

static bool emit_yaml_sequence(yaml_emitter_t* emitter, const Value* array) {
    if (!array || !array->as.array) {
        return false;
    }
    
    yaml_event_t event;
    
    // Start sequence
    yaml_sequence_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) {
        return false;
    }
    
    // Emit array items
    for (size_t i = 0; i < array->as.array->length; i++) {
        if (!emit_yaml_value(emitter, array->as.array->items[i])) {
            return false;
        }
    }
    
    // End sequence
    yaml_sequence_end_event_initialize(&event);
    return yaml_emitter_emit(emitter, &event);
}

static bool emit_yaml_mapping(yaml_emitter_t* emitter, const Value* object) {
    if (!object || !object->as.object) {
        return false;
    }
    
    yaml_event_t event;
    
    // Start mapping
    yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) {
        return false;
    }
    
    // Emit object pairs
    for (size_t i = 0; i < object->as.object->length; i++) {
        const char* key = object->as.object->pairs[i].key;
        const Value* value = object->as.object->pairs[i].value;
        
        // Emit key
        yaml_scalar_event_initialize(&event, NULL, NULL, (unsigned char*)key, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) {
            return false;
        }
        
        // Emit value
        if (!emit_yaml_value(emitter, value)) {
            return false;
        }
    }
    
    // End mapping
    yaml_mapping_end_event_initialize(&event);
    return yaml_emitter_emit(emitter, &event);
}