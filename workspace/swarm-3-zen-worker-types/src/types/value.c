/*
 * value.c
 * Value type system implementation for ZEN language
 * 
 * Implements reference-counted values with proper memory management
 * and type coercion according to ZEN language semantics.
 */

#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdatomic.h>

// Portable strdup implementation
static char* zen_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

// Initial capacity for arrays and object hash tables
#define INITIAL_ARRAY_CAPACITY 8
#define INITIAL_OBJECT_BUCKETS 16

// Hash function for object keys
static size_t hash_string(const char* str) {
    size_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

// Free array contents
static void value_free_array(ValueArray* array) {
    if (array->elements) {
        for (size_t i = 0; i < array->count; i++) {
            value_unref(array->elements[i]);
        }
        free(array->elements);
    }
}

// Free object contents
static void value_free_object(ValueObject* object) {
    if (object->buckets) {
        for (size_t i = 0; i < object->bucket_count; i++) {
            ObjectEntry* entry = object->buckets[i];
            while (entry) {
                ObjectEntry* next = entry->next;
                free(entry->key);
                value_unref(entry->value);
                free(entry);
                entry = next;
            }
        }
        free(object->buckets);
    }
}

/**
 * @brief Create new value of specified type
 * @param type The type of value to create
 * @return Value* New value instance with ref_count = 1, NULL on allocation failure
 */
Value* value_new(ValueType type) {
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = type;
    value->ref_count = 1;
    
    // Initialize type-specific data
    switch (type) {
        case VALUE_NULL:
            break;
        case VALUE_BOOLEAN:
            value->as.boolean = false;
            break;
        case VALUE_NUMBER:
            value->as.number = 0.0;
            break;
        case VALUE_STRING:
            value->as.string = NULL;
            break;
        case VALUE_ARRAY:
            value->as.array.elements = NULL;
            value->as.array.count = 0;
            value->as.array.capacity = 0;
            break;
        case VALUE_OBJECT:
            value->as.object.buckets = NULL;
            value->as.object.bucket_count = 0;
            value->as.object.count = 0;
            break;
        case VALUE_FUNCTION:
            value->as.function = NULL;
            break;
        case VALUE_ERROR:
            value->as.error = NULL;
            break;
    }
    
    return value;
}

/**
 * @brief Create string value
 * @param str The string content (will be copied)
 * @return Value* New string value instance, NULL on allocation failure
 */
Value* value_new_string(const char* str) {
    if (!str) {
        return NULL;
    }
    
    Value* value = value_new(VALUE_STRING);
    if (!value) {
        return NULL;
    }
    
    value->as.string = zen_strdup(str);
    if (!value->as.string) {
        free(value);
        return NULL;
    }
    
    return value;
}

/**
 * @brief Create number value
 * @param num The numeric value
 * @return Value* New number value instance, NULL on allocation failure
 */
Value* value_new_number(double num) {
    Value* value = value_new(VALUE_NUMBER);
    if (!value) {
        return NULL;
    }
    
    value->as.number = num;
    return value;
}

/**
 * @brief Create boolean value
 * @param val The boolean value
 * @return Value* New boolean value instance, NULL on allocation failure
 */
Value* value_new_boolean(bool val) {
    Value* value = value_new(VALUE_BOOLEAN);
    if (!value) {
        return NULL;
    }
    
    value->as.boolean = val;
    return value;
}

/**
 * @brief Create null value
 * @return Value* New null value instance, NULL on allocation failure
 */
Value* value_new_null(void) {
    return value_new(VALUE_NULL);
}

/**
 * @brief Deep copy a value
 * @param value The value to copy
 * @return Value* New value instance that is a deep copy, NULL on failure
 */
Value* value_copy(const Value* value) {
    if (!value) {
        return NULL;
    }
    
    Value* copy;
    
    switch (value->type) {
        case VALUE_NULL:
            return value_new_null();
            
        case VALUE_BOOLEAN:
            return value_new_boolean(value->as.boolean);
            
        case VALUE_NUMBER:
            return value_new_number(value->as.number);
            
        case VALUE_STRING:
            return value_new_string(value->as.string);
            
        case VALUE_ARRAY:
            copy = value_new(VALUE_ARRAY);
            if (!copy) return NULL;
            
            if (value->as.array.count > 0) {
                copy->as.array.capacity = value->as.array.count;
                copy->as.array.elements = malloc(copy->as.array.capacity * sizeof(Value*));
                if (!copy->as.array.elements) {
                    free(copy);
                    return NULL;
                }
                
                copy->as.array.count = value->as.array.count;
                for (size_t i = 0; i < value->as.array.count; i++) {
                    copy->as.array.elements[i] = value_copy(value->as.array.elements[i]);
                    if (!copy->as.array.elements[i]) {
                        // Clean up on failure
                        for (size_t j = 0; j < i; j++) {
                            value_unref(copy->as.array.elements[j]);
                        }
                        free(copy->as.array.elements);
                        free(copy);
                        return NULL;
                    }
                }
            }
            return copy;
            
        case VALUE_OBJECT:
            copy = value_new(VALUE_OBJECT);
            if (!copy) return NULL;
            
            if (value->as.object.count > 0) {
                copy->as.object.bucket_count = value->as.object.bucket_count;
                copy->as.object.buckets = calloc(copy->as.object.bucket_count, sizeof(ObjectEntry*));
                if (!copy->as.object.buckets) {
                    free(copy);
                    return NULL;
                }
                
                // Copy all entries
                for (size_t i = 0; i < value->as.object.bucket_count; i++) {
                    ObjectEntry* src_entry = value->as.object.buckets[i];
                    ObjectEntry** dst_entry = &copy->as.object.buckets[i];
                    
                    while (src_entry) {
                        *dst_entry = malloc(sizeof(ObjectEntry));
                        if (!*dst_entry) {
                            value_free(copy);
                            return NULL;
                        }
                        
                        (*dst_entry)->key = zen_strdup(src_entry->key);
                        (*dst_entry)->value = value_copy(src_entry->value);
                        (*dst_entry)->next = NULL;
                        
                        if (!(*dst_entry)->key || !(*dst_entry)->value) {
                            value_free(copy);
                            return NULL;
                        }
                        
                        copy->as.object.count++;
                        dst_entry = &(*dst_entry)->next;
                        src_entry = src_entry->next;
                    }
                }
            }
            return copy;
            
        case VALUE_FUNCTION:
        case VALUE_ERROR:
            // For now, these are shallow copies
            copy = value_new(value->type);
            if (!copy) return NULL;
            copy->as = value->as;
            return copy;
    }
    
    return NULL;
}

/**
 * @brief Free value memory (decrements ref count, frees if count reaches 0)
 * @param value The value to free
 */
void value_free(Value* value) {
    if (!value) {
        return;
    }
    
    // This is actually value_unref - keeping for API compatibility
    value_unref(value);
}

/**
 * @brief Convert value to string representation
 * @param value The value to convert
 * @return char* String representation (caller must free), NULL on failure
 */
char* value_to_string(const Value* value) {
    if (!value) {
        return zen_strdup("null");
    }
    
    char* result;
    
    switch (value->type) {
        case VALUE_NULL:
            return zen_strdup("null");
            
        case VALUE_BOOLEAN:
            return zen_strdup(value->as.boolean ? "true" : "false");
            
        case VALUE_NUMBER:
            // Handle integer vs float representation
            if (floor(value->as.number) == value->as.number) {
                result = malloc(32);
                if (result) {
                    snprintf(result, 32, "%.0f", value->as.number);
                }
            } else {
                result = malloc(32);
                if (result) {
                    snprintf(result, 32, "%.15g", value->as.number);
                }
            }
            return result;
            
        case VALUE_STRING:
            return value->as.string ? zen_strdup(value->as.string) : zen_strdup("");
            
        case VALUE_ARRAY: {
            // Simple array representation: [elem1, elem2, ...]
            size_t total_len = 3; // "[]"
            char** elem_strings = malloc(value->as.array.count * sizeof(char*));
            if (!elem_strings) return NULL;
            
            // Convert all elements to strings
            for (size_t i = 0; i < value->as.array.count; i++) {
                elem_strings[i] = value_to_string(value->as.array.elements[i]);
                if (!elem_strings[i]) {
                    // Clean up on failure
                    for (size_t j = 0; j < i; j++) {
                        free(elem_strings[j]);
                    }
                    free(elem_strings);
                    return NULL;
                }
                total_len += strlen(elem_strings[i]);
                if (i > 0) total_len += 2; // ", "
            }
            
            result = malloc(total_len);
            if (!result) {
                for (size_t i = 0; i < value->as.array.count; i++) {
                    free(elem_strings[i]);
                }
                free(elem_strings);
                return NULL;
            }
            
            strcpy(result, "[");
            for (size_t i = 0; i < value->as.array.count; i++) {
                if (i > 0) strcat(result, ", ");
                strcat(result, elem_strings[i]);
                free(elem_strings[i]);
            }
            strcat(result, "]");
            free(elem_strings);
            return result;
        }
        
        case VALUE_OBJECT: {
            // Simple object representation: {key1: value1, key2: value2}
            result = malloc(4096); // Start with reasonable buffer
            if (!result) return NULL;
            
            strcpy(result, "{");
            bool first = true;
            
            for (size_t i = 0; i < value->as.object.bucket_count; i++) {
                ObjectEntry* entry = value->as.object.buckets[i];
                while (entry) {
                    if (!first) strcat(result, ", ");
                    
                    strcat(result, entry->key);
                    strcat(result, ": ");
                    
                    char* value_str = value_to_string(entry->value);
                    if (value_str) {
                        strcat(result, value_str);
                        free(value_str);
                    }
                    
                    first = false;
                    entry = entry->next;
                }
            }
            strcat(result, "}");
            return result;
        }
        
        case VALUE_FUNCTION:
            return zen_strdup("<function>");
            
        case VALUE_ERROR:
            if (value->as.error) {
                result = malloc(strlen(value->as.error) + 8);
                if (result) {
                    snprintf(result, strlen(value->as.error) + 8, "Error: %s", value->as.error);
                }
                return result;
            }
            return zen_strdup("Error");
    }
    
    return zen_strdup("<unknown>");
}

/**
 * @brief Compare two values for equality
 * @param a First value to compare
 * @param b Second value to compare  
 * @return bool True if values are equal, false otherwise
 */
bool value_equals(const Value* a, const Value* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->type != b->type) return false;
    
    switch (a->type) {
        case VALUE_NULL:
            return true;
            
        case VALUE_BOOLEAN:
            return a->as.boolean == b->as.boolean;
            
        case VALUE_NUMBER:
            return a->as.number == b->as.number;
            
        case VALUE_STRING:
            if (!a->as.string && !b->as.string) return true;
            if (!a->as.string || !b->as.string) return false;
            return strcmp(a->as.string, b->as.string) == 0;
            
        case VALUE_ARRAY:
            if (a->as.array.count != b->as.array.count) return false;
            for (size_t i = 0; i < a->as.array.count; i++) {
                if (!value_equals(a->as.array.elements[i], b->as.array.elements[i])) {
                    return false;
                }
            }
            return true;
            
        case VALUE_OBJECT:
            if (a->as.object.count != b->as.object.count) return false;
            
            // Check all entries in object a exist in object b with same values
            for (size_t i = 0; i < a->as.object.bucket_count; i++) {
                ObjectEntry* entry_a = a->as.object.buckets[i];
                while (entry_a) {
                    // Find corresponding entry in object b
                    size_t hash = hash_string(entry_a->key) % b->as.object.bucket_count;
                    ObjectEntry* entry_b = b->as.object.buckets[hash];
                    bool found = false;
                    
                    while (entry_b) {
                        if (strcmp(entry_a->key, entry_b->key) == 0) {
                            if (!value_equals(entry_a->value, entry_b->value)) {
                                return false;
                            }
                            found = true;
                            break;
                        }
                        entry_b = entry_b->next;
                    }
                    
                    if (!found) return false;
                    entry_a = entry_a->next;
                }
            }
            return true;
            
        case VALUE_FUNCTION:
        case VALUE_ERROR:
            // For now, these are pointer equality
            return a->as.function == b->as.function;
    }
    
    return false;
}

/**
 * @brief Get string name of value type
 * @param type The value type
 * @return const char* String name of the type
 */
const char* value_type_name(ValueType type) {
    switch (type) {
        case VALUE_NULL: return "null";
        case VALUE_BOOLEAN: return "boolean";
        case VALUE_NUMBER: return "number";
        case VALUE_STRING: return "string";
        case VALUE_ARRAY: return "array";
        case VALUE_OBJECT: return "object";
        case VALUE_FUNCTION: return "function";
        case VALUE_ERROR: return "error";
        default: return "unknown";
    }
}

/**
 * @brief Increment reference count (thread-safe)
 * @param value The value to reference
 */
void value_ref(Value* value) {
    if (value) {
        atomic_fetch_add(&value->ref_count, 1);
    }
}

/**
 * @brief Decrement reference count and free if zero (thread-safe)
 * @param value The value to unreference
 */
void value_unref(Value* value) {
    if (!value) {
        return;
    }
    
    // Atomic decrement and check if we should free
    if (atomic_fetch_sub(&value->ref_count, 1) == 1) {
        // Reference count reached zero, free the value
        switch (value->type) {
            case VALUE_STRING:
                free(value->as.string);
                break;
                
            case VALUE_ARRAY:
                value_free_array(&value->as.array);
                break;
                
            case VALUE_OBJECT:
                value_free_object(&value->as.object);
                break;
                
            case VALUE_ERROR:
                free(value->as.error);
                break;
                
            case VALUE_NULL:
            case VALUE_BOOLEAN:
            case VALUE_NUMBER:
            case VALUE_FUNCTION:
                // No additional cleanup needed
                break;
        }
        
        free(value);
    }
}

