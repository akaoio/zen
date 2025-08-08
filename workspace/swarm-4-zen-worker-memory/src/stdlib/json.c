/*
 * json.c
 * JSON parsing and generation - Self-contained implementation
 * 
 * This file follows MANIFEST.json specification
 * Function signatures must match manifest exactly
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/json.h"
#include "zen/types/value.h"
#include "zen/types/array.h"
#include "zen/types/object.h"
#include "zen/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

// Maximum file size for JSON parsing (64MB) to prevent memory exhaustion
#define MAX_JSON_FILE_SIZE (64 * 1024 * 1024)

// Forward declarations for array and object functions

// JSON parsing state
typedef struct {
    const char* input;
    size_t pos;
    size_t length;
} JsonParser;

// Forward declarations
static Value* parse_value(JsonParser* parser);
static Value* parse_object(JsonParser* parser);
static Value* parse_array(JsonParser* parser);
static Value* parse_string(JsonParser* parser);
static Value* parse_number(JsonParser* parser);
static void skip_whitespace(JsonParser* parser);
static char peek_char(JsonParser* parser);
static char next_char(JsonParser* parser);
static bool expect_char(JsonParser* parser, char expected);

/**
 * @brief Parse JSON string to Value
 * @param json_string JSON string to parse
 * @return Newly allocated Value representing the JSON data, or error Value on failure
 */
Value* json_parse(const char* json_string) {
    if (!json_string) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = memory_strdup("JSON string is NULL");
            error->as.error->code = -1;
        }
        return error;
    }
    
    size_t json_length = strlen(json_string);
    
    // Check file size limit to prevent memory exhaustion
    if (json_length > MAX_JSON_FILE_SIZE) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = memory_strdup("JSON string exceeds maximum size limit (64MB)");
            error->as.error->code = -2;
        }
        return error;
    }
    
    JsonParser parser = {
        .input = json_string,
        .pos = 0,
        .length = json_length
    };
    
    skip_whitespace(&parser);
    Value* result = parse_value(&parser);
    
    if (!result) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = memory_strdup("Failed to parse JSON");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return result;
}

/**
 * @brief Convert Value to JSON string
 * @param value Value to convert to JSON
 * @return Newly allocated JSON string, or NULL on error
 */
char* json_stringify(const Value* value) {
    if (!value) {
        return memory_strdup("null");
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return memory_strdup("null");
            
        case VALUE_BOOLEAN:
            return memory_strdup(value->as.boolean ? "true" : "false");
            
        case VALUE_NUMBER: {
            char* buffer = memory_alloc(32);
            if (!buffer) return NULL;
            
            // Check if it's an integer
            if (value->as.number == floor(value->as.number)) {
                snprintf(buffer, 32, "%.0f", value->as.number);
            } else {
                snprintf(buffer, 32, "%g", value->as.number);
            }
            return buffer;
        }
        
        case VALUE_STRING: {
            if (!value->as.string || !value->as.string->data) {
                return memory_strdup("\"\"");
            }
            
            // Estimate escaped length (worst case: all chars need escaping)
            size_t escaped_len = value->as.string->length * 2 + 3; // +3 for quotes and null
            char* buffer = memory_alloc(escaped_len);
            if (!buffer) return NULL;
            
            buffer[0] = '"';
            size_t j = 1;
            
            for (size_t i = 0; i < value->as.string->length && value->as.string->data[i]; i++) {
                char c = value->as.string->data[i];
                switch (c) {
                    case '"':  buffer[j++] = '\\'; buffer[j++] = '"'; break;
                    case '\\': buffer[j++] = '\\'; buffer[j++] = '\\'; break;
                    case '\b': buffer[j++] = '\\'; buffer[j++] = 'b'; break;
                    case '\f': buffer[j++] = '\\'; buffer[j++] = 'f'; break;
                    case '\n': buffer[j++] = '\\'; buffer[j++] = 'n'; break;
                    case '\r': buffer[j++] = '\\'; buffer[j++] = 'r'; break;
                    case '\t': buffer[j++] = '\\'; buffer[j++] = 't'; break;
                    default:
                        if (c < 32) {
                            j += snprintf(buffer + j, escaped_len - j, "\\u%04x", (unsigned char)c);
                        } else {
                            buffer[j++] = c;
                        }
                        break;
                }
            }
            buffer[j++] = '"';
            buffer[j] = '\0';
            return buffer;
        }
        
        case VALUE_ARRAY: {
            if (!value->as.array) {
                return memory_strdup("[]");
            }
            
            // Start with opening bracket
            char* result = memory_strdup("[");
            if (!result) return NULL;
            
            for (size_t i = 0; i < value->as.array->length; i++) {
                if (i > 0) {
                    char* temp = memory_alloc(strlen(result) + 2);
                    if (!temp) {
                        memory_free(result);
                        return NULL;
                    }
                    size_t result_len = strlen(result);
                    strncpy(temp, result, result_len);
                    temp[result_len] = ',';
                    temp[result_len + 1] = '\0';
                    memory_free(result);
                    result = temp;
                }
                
                char* item_json = json_stringify(value->as.array->items[i]);
                if (!item_json) {
                    memory_free(result);
                    return NULL;
                }
                
                char* temp = memory_alloc(strlen(result) + strlen(item_json) + 1);
                if (!temp) {
                    memory_free(result);
                    memory_free(item_json);
                    return NULL;
                }
                size_t result_len = strlen(result);
                size_t item_len = strlen(item_json);
                strncpy(temp, result, result_len);
                temp[result_len] = '\0';
                strncat(temp, item_json, item_len);
                memory_free(result);
                memory_free(item_json);
                result = temp;
            }
            
            // Add closing bracket
            char* temp = memory_alloc(strlen(result) + 2);
            if (!temp) {
                memory_free(result);
                return NULL;
            }
            size_t result_len = strlen(result);
            strncpy(temp, result, result_len);
            temp[result_len] = ']';
            temp[result_len + 1] = '\0';
            memory_free(result);
            return temp;
        }
        
        case VALUE_OBJECT: {
            if (!value->as.object) {
                return memory_strdup("{}");
            }
            
            // Start with opening brace
            char* result = memory_strdup("{");
            if (!result) return NULL;
            
            for (size_t i = 0; i < value->as.object->length; i++) {
                if (i > 0) {
                    char* temp = memory_alloc(strlen(result) + 2);
                    if (!temp) {
                        memory_free(result);
                        return NULL;
                    }
                    size_t result_len = strlen(result);
                    strncpy(temp, result, result_len);
                    temp[result_len] = ',';
                    temp[result_len + 1] = '\0';
                    memory_free(result);
                    result = temp;
                }
                
                // Add key
                Value key_val = { .type = VALUE_STRING };
                ZenString key_str = { .data = value->as.object->pairs[i].key, .length = strlen(value->as.object->pairs[i].key) };
                key_val.as.string = &key_str;
                
                char* key_json = json_stringify(&key_val);
                if (!key_json) {
                    memory_free(result);
                    return NULL;
                }
                
                char* value_json = json_stringify(value->as.object->pairs[i].value);
                if (!value_json) {
                    memory_free(result);
                    memory_free(key_json);
                    return NULL;
                }
                
                char* temp = memory_alloc(strlen(result) + strlen(key_json) + strlen(value_json) + 2);
                if (!temp) {
                    memory_free(result);
                    memory_free(key_json);
                    memory_free(value_json);
                    return NULL;
                }
                strcpy(temp, result);
                strcat(temp, key_json);
                strcat(temp, ":");
                strcat(temp, value_json);
                
                memory_free(result);
                memory_free(key_json);
                memory_free(value_json);
                result = temp;
            }
            
            // Add closing brace
            char* temp = memory_alloc(strlen(result) + 2);
            if (!temp) {
                memory_free(result);
                return NULL;
            }
            size_t result_len = strlen(result);
            strncpy(temp, result, result_len);
            temp[result_len] = '}';
            temp[result_len + 1] = '\0';
            memory_free(result);
            return temp;
        }
        
        default:
            return memory_strdup("null");
    }
}

// Forward declaration for pretty printing helper
static char* json_stringify_pretty_internal(const Value* value, int indent_size, int current_depth);

/**
 * @brief Create indentation string for pretty printing
 * @param indent_size Number of spaces per level
 * @param depth Current indentation depth
 * @return Newly allocated indentation string
 */
static char* create_indent(int indent_size, int depth) {
    int total_spaces = indent_size * depth;
    char* indent = memory_alloc(total_spaces + 1);
    if (!indent) return NULL;
    
    for (int i = 0; i < total_spaces; i++) {
        indent[i] = ' ';
    }
    indent[total_spaces] = '\0';
    return indent;
}

/**
 * @brief Internal recursive function for pretty JSON formatting
 * @param value Value to convert to JSON
 * @param indent_size Number of spaces per indentation level
 * @param current_depth Current depth level for indentation
 * @return Newly allocated formatted JSON string, or NULL on error
 */
static char* json_stringify_pretty_internal(const Value* value, int indent_size, int current_depth) {
    if (!value) {
        return memory_strdup("null");
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return memory_strdup("null");
            
        case VALUE_BOOLEAN:
            return memory_strdup(value->as.boolean ? "true" : "false");
            
        case VALUE_NUMBER: {
            char* buffer = memory_alloc(32);
            if (!buffer) return NULL;
            
            // Check if it's an integer
            if (value->as.number == floor(value->as.number)) {
                snprintf(buffer, 32, "%.0f", value->as.number);
            } else {
                snprintf(buffer, 32, "%g", value->as.number);
            }
            return buffer;
        }
        
        case VALUE_STRING: {
            if (!value->as.string || !value->as.string->data) {
                return memory_strdup("\"\"");
            }
            
            // Estimate escaped length (worst case: all chars need escaping)
            size_t escaped_len = value->as.string->length * 2 + 3; // +3 for quotes and null
            char* buffer = memory_alloc(escaped_len);
            if (!buffer) return NULL;
            
            buffer[0] = '"';
            size_t j = 1;
            
            for (size_t i = 0; i < value->as.string->length && value->as.string->data[i]; i++) {
                char c = value->as.string->data[i];
                switch (c) {
                    case '"':  buffer[j++] = '\\'; buffer[j++] = '"'; break;
                    case '\\': buffer[j++] = '\\'; buffer[j++] = '\\'; break;
                    case '\b': buffer[j++] = '\\'; buffer[j++] = 'b'; break;
                    case '\f': buffer[j++] = '\\'; buffer[j++] = 'f'; break;
                    case '\n': buffer[j++] = '\\'; buffer[j++] = 'n'; break;
                    case '\r': buffer[j++] = '\\'; buffer[j++] = 'r'; break;
                    case '\t': buffer[j++] = '\\'; buffer[j++] = 't'; break;
                    default:
                        if (c < 32) {
                            j += snprintf(buffer + j, escaped_len - j, "\\u%04x", (unsigned char)c);
                        } else {
                            buffer[j++] = c;
                        }
                        break;
                }
            }
            buffer[j++] = '"';
            buffer[j] = '\0';
            return buffer;
        }
        
        case VALUE_ARRAY: {
            if (!value->as.array || value->as.array->length == 0) {
                return memory_strdup("[]");
            }
            
            // Start with opening bracket and newline
            char* result = memory_strdup("[\n");
            if (!result) return NULL;
            
            for (size_t i = 0; i < value->as.array->length; i++) {
                // Add indentation for array item
                char* indent = create_indent(indent_size, current_depth + 1);
                if (!indent) {
                    memory_free(result);
                    return NULL;
                }
                
                char* item_json = json_stringify_pretty_internal(value->as.array->items[i], indent_size, current_depth + 1);
                if (!item_json) {
                    memory_free(result);
                    memory_free(indent);
                    return NULL;
                }
                
                // Calculate needed space: current result + indent + item + comma + newline + null
                size_t new_size = strlen(result) + strlen(indent) + strlen(item_json) + 3;
                char* temp = memory_alloc(new_size);
                if (!temp) {
                    memory_free(result);
                    memory_free(indent);
                    memory_free(item_json);
                    return NULL;
                }
                
                // Build the new result
                strcpy(temp, result);
                strcat(temp, indent);
                strcat(temp, item_json);
                
                // Add comma if not last item
                if (i < value->as.array->length - 1) {
                    strcat(temp, ",");
                }
                strcat(temp, "\n");
                
                memory_free(result);
                memory_free(indent);
                memory_free(item_json);
                result = temp;
            }
            
            // Add closing bracket with proper indentation
            char* closing_indent = create_indent(indent_size, current_depth);
            if (!closing_indent) {
                memory_free(result);
                return NULL;
            }
            
            size_t final_size = strlen(result) + strlen(closing_indent) + 2;
            char* temp = memory_alloc(final_size);
            if (!temp) {
                memory_free(result);
                memory_free(closing_indent);
                return NULL;
            }
            
            strcpy(temp, result);
            strcat(temp, closing_indent);
            strcat(temp, "]");
            
            memory_free(result);
            memory_free(closing_indent);
            return temp;
        }
        
        case VALUE_OBJECT: {
            if (!value->as.object || value->as.object->length == 0) {
                return memory_strdup("{}");
            }
            
            // Start with opening brace and newline
            char* result = memory_strdup("{\n");
            if (!result) return NULL;
            
            for (size_t i = 0; i < value->as.object->length; i++) {
                // Add indentation for object property
                char* indent = create_indent(indent_size, current_depth + 1);
                if (!indent) {
                    memory_free(result);
                    return NULL;
                }
                
                // Add key (quoted)
                Value key_val = { .type = VALUE_STRING };
                ZenString key_str = { .data = value->as.object->pairs[i].key, .length = strlen(value->as.object->pairs[i].key) };
                key_val.as.string = &key_str;
                
                char* key_json = json_stringify_pretty_internal(&key_val, indent_size, current_depth + 1);
                if (!key_json) {
                    memory_free(result);
                    memory_free(indent);
                    return NULL;
                }
                
                // Add value (recursively formatted)
                char* value_json = json_stringify_pretty_internal(value->as.object->pairs[i].value, indent_size, current_depth + 1);
                if (!value_json) {
                    memory_free(result);
                    memory_free(indent);
                    memory_free(key_json);
                    return NULL;
                }
                
                // Calculate needed space: current result + indent + key + ": " + value + comma + newline + null
                size_t new_size = strlen(result) + strlen(indent) + strlen(key_json) + strlen(value_json) + 5;
                char* temp = memory_alloc(new_size);
                if (!temp) {
                    memory_free(result);
                    memory_free(indent);
                    memory_free(key_json);
                    memory_free(value_json);
                    return NULL;
                }
                
                // Build the new result
                strcpy(temp, result);
                strcat(temp, indent);
                strcat(temp, key_json);
                strcat(temp, ": ");
                strcat(temp, value_json);
                
                // Add comma if not last property
                if (i < value->as.object->length - 1) {
                    strcat(temp, ",");
                }
                strcat(temp, "\n");
                
                memory_free(result);
                memory_free(indent);
                memory_free(key_json);
                memory_free(value_json);
                result = temp;
            }
            
            // Add closing brace with proper indentation
            char* closing_indent = create_indent(indent_size, current_depth);
            if (!closing_indent) {
                memory_free(result);
                return NULL;
            }
            
            size_t final_size = strlen(result) + strlen(closing_indent) + 2;
            char* temp = memory_alloc(final_size);
            if (!temp) {
                memory_free(result);
                memory_free(closing_indent);
                return NULL;
            }
            
            strcpy(temp, result);
            strcat(temp, closing_indent);
            strcat(temp, "}");
            
            memory_free(result);
            memory_free(closing_indent);
            return temp;
        }
        
        default:
            return memory_strdup("null");
    }
}

/**
 * @brief Convert Value to formatted JSON string with indentation
 * @param value Value to convert to JSON
 * @param indent_size Number of spaces per indentation level
 * @return Newly allocated formatted JSON string, or NULL on error
 */
char* json_stringify_pretty(const Value* value, int indent_size) {
    if (!value) {
        return memory_strdup("null");
    }
    
    if (indent_size <= 0) {
        // If invalid indent size, fall back to regular stringify
        return json_stringify(value);
    }
    
    return json_stringify_pretty_internal(value, indent_size, 0);
}

// Helper functions for cJSON management
static cJSON* cjson_create(cJSON_Type type) {
    cJSON* item = memory_alloc(sizeof(cJSON));
    if (!item) return NULL;
    
    item->next = NULL;
    item->prev = NULL;
    item->child = NULL;
    item->type = type;
    item->valuestring = NULL;
    item->valuedouble = 0.0;
    item->string = NULL;
    
    return item;
}

static void cjson_delete(cJSON* item) {
    if (!item) return;
    
    cJSON* child = item->child;
    while (child) {
        cJSON* next = child->next;
        cjson_delete(child);
        child = next;
    }
    
    if (item->valuestring) memory_free(item->valuestring);
    if (item->string) memory_free(item->string);
    memory_free(item);
}

static void cjson_add_item_to_object(cJSON* object, const char* name, cJSON* item) {
    if (!object || !name || !item) return;
    
    item->string = memory_strdup(name);
    
    if (!object->child) {
        object->child = item;
    } else {
        cJSON* child = object->child;
        while (child->next) {
            child = child->next;
        }
        child->next = item;
        item->prev = child;
    }
}

static void cjson_add_item_to_array(cJSON* array, cJSON* item) {
    if (!array || !item) return;
    
    if (!array->child) {
        array->child = item;
    } else {
        cJSON* child = array->child;
        while (child->next) {
            child = child->next;
        }
        child->next = item;
        item->prev = child;
    }
}

/**
 * @brief Convert Value to cJSON object
 * @param value Value to convert
 * @return cJSON object representing the value, or NULL on error
 */
cJSON* json_value_to_cjson(const Value* value) {
    if (!value) {
        return cjson_create(cJSON_NULL);
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return cjson_create(cJSON_NULL);
            
        case VALUE_BOOLEAN: {
            cJSON* item = cjson_create(value->as.boolean ? cJSON_True : cJSON_False);
            return item;
        }
        
        case VALUE_NUMBER: {
            cJSON* item = cjson_create(cJSON_Number);
            if (item) {
                item->valuedouble = value->as.number;
            }
            return item;
        }
        
        case VALUE_STRING: {
            if (!value->as.string || !value->as.string->data) {
                cJSON* item = cjson_create(cJSON_String);
                if (item) {
                    item->valuestring = memory_strdup("");
                }
                return item;
            }
            
            cJSON* item = cjson_create(cJSON_String);
            if (item) {
                item->valuestring = memory_strdup(value->as.string->data);
            }
            return item;
        }
        
        case VALUE_ARRAY: {
            cJSON* array_item = cjson_create(cJSON_Array);
            if (!array_item) return NULL;
            
            if (value->as.array) {
                for (size_t i = 0; i < value->as.array->length; i++) {
                    cJSON* child_item = json_value_to_cjson(value->as.array->items[i]);
                    if (!child_item) {
                        cjson_delete(array_item);
                        return NULL;
                    }
                    cjson_add_item_to_array(array_item, child_item);
                }
            }
            return array_item;
        }
        
        case VALUE_OBJECT: {
            cJSON* object_item = cjson_create(cJSON_Object);
            if (!object_item) return NULL;
            
            if (value->as.object) {
                for (size_t i = 0; i < value->as.object->length; i++) {
                    const char* key = value->as.object->pairs[i].key;
                    Value* val = value->as.object->pairs[i].value;
                    
                    cJSON* child_item = json_value_to_cjson(val);
                    if (!child_item) {
                        cjson_delete(object_item);
                        return NULL;
                    }
                    cjson_add_item_to_object(object_item, key, child_item);
                }
            }
            return object_item;
        }
        
        case VALUE_FUNCTION:
        case VALUE_ERROR:
        case VALUE_UNDECIDABLE:
        default:
            // For non-JSON-serializable types, return null
            return cjson_create(cJSON_NULL);
    }
}

/**
 * @brief Convert cJSON to Value
 * @param json cJSON object to convert
 * @return Newly allocated Value representing the JSON data
 */
Value* json_cjson_to_value(const cJSON* json) {
    if (!json) {
        return value_new_null();
    }
    
    switch (json->type) {
        case cJSON_NULL:
            return value_new_null();
            
        case cJSON_False:
            return value_new_boolean(false);
            
        case cJSON_True:
            return value_new_boolean(true);
            
        case cJSON_Number:
            return value_new_number(json->valuedouble);
            
        case cJSON_String: {
            if (!json->valuestring) {
                return value_new_string("");
            }
            return value_new_string(json->valuestring);
        }
        
        case cJSON_Array: {
            Value* array = value_new(VALUE_ARRAY);
            if (!array) return NULL;
            
            cJSON* child = json->child;
            while (child) {
                Value* child_value = json_cjson_to_value(child);
                if (!child_value) {
                    value_unref(array);
                    return NULL;
                }
                array_push(array, child_value);
                child = child->next;
            }
            return array;
        }
        
        case cJSON_Object: {
            Value* object = value_new(VALUE_OBJECT);
            if (!object) return NULL;
            
            cJSON* child = json->child;
            while (child) {
                if (child->string) {
                    Value* child_value = json_cjson_to_value(child);
                    if (!child_value) {
                        value_unref(object);
                        return NULL;
                    }
                    object_set(object, child->string, child_value);
                }
                child = child->next;
            }
            return object;
        }
        
        default:
            return value_new_null();
    }
}

// Helper functions for parsing

static void skip_whitespace(JsonParser* parser) {
    while (parser->pos < parser->length && isspace(parser->input[parser->pos])) {
        parser->pos++;
    }
}

static char peek_char(JsonParser* parser) {
    if (parser->pos >= parser->length) {
        return '\0';
    }
    return parser->input[parser->pos];
}

static char next_char(JsonParser* parser) {
    if (parser->pos >= parser->length) {
        return '\0';
    }
    return parser->input[parser->pos++];
}

static bool expect_char(JsonParser* parser, char expected) {
    skip_whitespace(parser);
    if (peek_char(parser) == expected) {
        next_char(parser);
        return true;
    }
    return false;
}

static Value* parse_value(JsonParser* parser) {
    skip_whitespace(parser);
    char c = peek_char(parser);
    
    switch (c) {
        case '{':
            return parse_object(parser);
        case '[':
            return parse_array(parser);
        case '"':
            return parse_string(parser);
        case 't':
            if (parser->pos + 4 <= parser->length && strncmp(&parser->input[parser->pos], "true", 4) == 0) {
                parser->pos += 4;
                return value_new_boolean(true);
            }
            break;
        case 'f':
            if (parser->pos + 5 <= parser->length && strncmp(&parser->input[parser->pos], "false", 5) == 0) {
                parser->pos += 5;
                return value_new_boolean(false);
            }
            break;
        case 'n':
            if (parser->pos + 4 <= parser->length && strncmp(&parser->input[parser->pos], "null", 4) == 0) {
                parser->pos += 4;
                return value_new_null();
            }
            break;
        default:
            if (c == '-' || isdigit(c)) {
                return parse_number(parser);
            }
            break;
    }
    
    return NULL; // Parse error
}

static Value* parse_object(JsonParser* parser) {
    if (!expect_char(parser, '{')) {
        return NULL;
    }
    
    Value* obj = value_new(VALUE_OBJECT);
    if (!obj) return NULL;
    
    skip_whitespace(parser);
    if (peek_char(parser) == '}') {
        next_char(parser);
        return obj; // Empty object
    }
    
    while (true) {
        skip_whitespace(parser);
        
        // Parse key
        Value* key_val = parse_string(parser);
        if (!key_val || key_val->type != VALUE_STRING) {
            value_unref(obj);
            if (key_val) value_unref(key_val);
            return NULL;
        }
        
        // Expect colon
        if (!expect_char(parser, ':')) {
            value_unref(obj);
            value_unref(key_val);
            return NULL;
        }
        
        // Parse value
        Value* val = parse_value(parser);
        if (!val) {
            value_unref(obj);
            value_unref(key_val);
            return NULL;
        }
        
        // Add to object
        if (key_val->as.string && key_val->as.string->data) {
            object_set(obj, key_val->as.string->data, val);
        }
        value_unref(key_val);
        
        skip_whitespace(parser);
        char c = peek_char(parser);
        if (c == '}') {
            next_char(parser);
            break;
        } else if (c == ',') {
            next_char(parser);
            continue;
        } else {
            value_unref(obj);
            return NULL; // Parse error
        }
    }
    
    return obj;
}

static Value* parse_array(JsonParser* parser) {
    if (!expect_char(parser, '[')) {
        return NULL;
    }
    
    Value* arr = value_new(VALUE_ARRAY);
    if (!arr) return NULL;
    
    skip_whitespace(parser);
    if (peek_char(parser) == ']') {
        next_char(parser);
        return arr; // Empty array
    }
    
    while (true) {
        Value* item = parse_value(parser);
        if (!item) {
            value_unref(arr);
            return NULL;
        }
        
        // Add to array
        array_push(arr, item);
        
        skip_whitespace(parser);
        char c = peek_char(parser);
        if (c == ']') {
            next_char(parser);
            break;
        } else if (c == ',') {
            next_char(parser);
            continue;
        } else {
            value_unref(arr);
            return NULL; // Parse error
        }
    }
    
    return arr;
}

static Value* parse_string(JsonParser* parser) {
    if (!expect_char(parser, '"')) {
        return NULL;
    }
    
    size_t start = parser->pos;
    size_t len = 0;
    
    // First pass: find end and calculate unescaped length
    while (parser->pos < parser->length) {
        char c = parser->input[parser->pos];
        if (c == '"') {
            break;
        } else if (c == '\\') {
            parser->pos++;
            if (parser->pos >= parser->length) {
                return NULL; // Unterminated escape
            }
            parser->pos++;
            len++;
        } else {
            parser->pos++;
            len++;
        }
    }
    
    if (parser->pos >= parser->length) {
        return NULL; // Unterminated string
    }
    
    // Allocate buffer for unescaped string
    char* str_data = memory_alloc(len + 1);
    if (!str_data) return NULL;
    
    // Second pass: unescape
    size_t str_pos = 0;
    parser->pos = start;
    
    while (parser->pos < parser->length && parser->input[parser->pos] != '"') {
        char c = parser->input[parser->pos++];
        if (c == '\\') {
            if (parser->pos >= parser->length) {
                memory_free(str_data);
                return NULL;
            }
            char escape = parser->input[parser->pos++];
            switch (escape) {
                case '"':  str_data[str_pos++] = '"'; break;
                case '\\': str_data[str_pos++] = '\\'; break;
                case '/':  str_data[str_pos++] = '/'; break;
                case 'b':  str_data[str_pos++] = '\b'; break;
                case 'f':  str_data[str_pos++] = '\f'; break;
                case 'n':  str_data[str_pos++] = '\n'; break;
                case 'r':  str_data[str_pos++] = '\r'; break;
                case 't':  str_data[str_pos++] = '\t'; break;
                default:
                    str_data[str_pos++] = escape; // Keep unknown escapes as-is
                    break;
            }
        } else {
            str_data[str_pos++] = c;
        }
    }
    
    str_data[str_pos] = '\0';
    parser->pos++; // Skip closing quote
    
    Value* result = value_new_string(str_data);
    memory_free(str_data);
    return result;
}

/**
 * @brief Parse a JSON number with comprehensive validation
 * 
 * This function parses a JSON number and performs comprehensive validation:
 * - Checks for valid number format using strtod endptr
 * - Rejects infinity and NaN values (not allowed in JSON specification)  
 * - Detects overflow/underflow conditions using errno
 * - Ensures JSON compliance by only accepting finite numbers
 *
 * @param parser JSON parser state
 * @return Value containing parsed number, or NULL if invalid
 */
static Value* parse_number(JsonParser* parser) {
    size_t start = parser->pos;
    
    // Handle negative sign
    if (peek_char(parser) == '-') {
        next_char(parser);
    }
    
    // Handle digits before decimal point
    if (!isdigit(peek_char(parser))) {
        return NULL;
    }
    
    while (isdigit(peek_char(parser))) {
        next_char(parser);
    }
    
    // Handle decimal point
    if (peek_char(parser) == '.') {
        next_char(parser);
        if (!isdigit(peek_char(parser))) {
            return NULL;
        }
        while (isdigit(peek_char(parser))) {
            next_char(parser);
        }
    }
    
    // Handle exponent
    char c = peek_char(parser);
    if (c == 'e' || c == 'E') {
        next_char(parser);
        c = peek_char(parser);
        if (c == '+' || c == '-') {
            next_char(parser);
        }
        if (!isdigit(peek_char(parser))) {
            return NULL;
        }
        while (isdigit(peek_char(parser))) {
            next_char(parser);
        }
    }
    
    // Extract number string and convert
    size_t num_len = parser->pos - start;
    char* num_str = memory_alloc(num_len + 1);
    if (!num_str) return NULL;
    
    strncpy(num_str, &parser->input[start], num_len);
    num_str[num_len] = '\0';
    
    // Clear errno before conversion to detect overflow/underflow
    errno = 0;
    char* endptr = NULL;
    double value = strtod(num_str, &endptr);
    
    // Check for parsing errors
    if (endptr == num_str || *endptr != '\0') {
        memory_free(num_str);
        return NULL; // Invalid number format
    }
    
    // Check for infinity, NaN, or overflow
    if (!isfinite(value)) {
        memory_free(num_str);
        return NULL; // JSON doesn't support infinity or NaN
    }
    
    // Check for overflow/underflow (ERANGE set by strtod)
    if (errno == ERANGE) {
        memory_free(num_str);
        return NULL; // Number too large or too small to represent
    }
    
    memory_free(num_str);
    return value_new_number(value);
}

// Stdlib wrapper functions

/**
 * @brief Load JSON file wrapper function for stdlib
 * @param args Array of Value arguments (filename)
 * @param argc Number of arguments
 * @return Parsed JSON Value or error
 */
Value* json_load_file(Value** args, size_t argc) {
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("loadJsonFile requires a filename string", -1);
    }
    
    // Use the io_load_json_file_internal function from io.c
    extern Value* io_load_json_file_internal(const char* filepath);
    return io_load_json_file_internal(args[0]->as.string->data);
}

/**
 * @brief Parse JSON file with proper error handling and size limits
 * @param filename Path to JSON file to parse
 * @return Parsed JSON Value or error Value on failure
 */
Value* json_parse_file_safe(const char* filename) {
    if (!filename) {
        return value_new_error("Filename is NULL", -1);
    }
    
    // Check if file exists using io functions
    extern bool io_file_exists_internal(const char* filepath);
    if (!io_file_exists_internal(filename)) {
        return value_new_error("File does not exist", -1);
    }
    
    // Read file content first, then check size limit
    extern char* io_read_file_internal(const char* filepath);
    char* content = io_read_file_internal(filename);
    if (!content) {
        return value_new_error("Failed to read file", -1);
    }
    
    // Check file size after reading to prevent memory exhaustion from huge files
    size_t content_length = strlen(content);
    if (content_length > MAX_JSON_FILE_SIZE) {
        memory_free(content);
        return value_new_error("File exceeds maximum size limit (64MB)", -2);
    }
    
    // Parse JSON content (will also check size, providing double protection)
    Value* result = json_parse(content);
    memory_free(content);
    
    return result;
}

/**
 * @brief Parse JSON string - stdlib wrapper
 * @param args Arguments array containing JSON string
 * @param argc Number of arguments
 * @return Parsed value or error
 */
Value* json_parse_stdlib(Value** args, size_t argc) {
    if (argc != 1) {
        return value_new_error("jsonParse requires exactly 1 argument", -1);
    }
    
    if (!args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("jsonParse requires a string argument", -1);
    }
    
    return json_parse(args[0]->as.string->data);
}

/**
 * @brief Convert value to JSON string - stdlib wrapper
 * @param args Arguments array containing value to stringify
 * @param argc Number of arguments
 * @return JSON string or error
 */
Value* json_stringify_stdlib(Value** args, size_t argc) {
    if (argc != 1) {
        return value_new_error("jsonStringify requires exactly 1 argument", -1);
    }
    
    if (!args[0]) {
        return value_new_string("null");
    }
    
    char* json_str = json_stringify(args[0]);
    if (!json_str) {
        return value_new_error("Failed to stringify value", -1);
    }
    
    Value* result = value_new_string(json_str);
    memory_free(json_str);
    return result;
}

/**
 * @brief Convert value to formatted JSON with indentation - stdlib wrapper
 * @param args Arguments array containing value and optional indent size
 * @param argc Number of arguments
 * @return Formatted JSON string or error
 */
Value* json_stringify_pretty_stdlib(Value** args, size_t argc) {
    if (argc < 1) {
        return value_new_error("jsonPretty requires at least 1 argument", -1);
    }
    
    if (!args[0]) {
        return value_new_string("null");
    }
    
    // For now, just use regular stringify - pretty printing can be added later
    char* json_str = json_stringify(args[0]);
    if (!json_str) {
        return value_new_error("Failed to stringify value", -1);
    }
    
    Value* result = value_new_string(json_str);
    memory_free(json_str);
    return result;
}