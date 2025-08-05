/*
 * value.c
 * Value type system
 * 
 * This file implements the core value system for the ZEN language.
 * All values use reference counting for memory management.
 */

#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/**
 * @brief Create new uninitialized value
 * @param type The type of value to create
 * @return Value* New value instance with ref_count = 1, or NULL on failure
 */
Value* value_new(ValueType type) {
    Value* value = malloc(sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = type;
    value->ref_count = 1;
    
    // Initialize union based on type
    switch (type) {
        case VALUE_NULL:
            break;
        case VALUE_BOOLEAN:
            value->data.boolean = false;
            break;
        case VALUE_NUMBER:
            value->data.number = 0.0;
            break;
        case VALUE_STRING:
            value->data.string = NULL;
            break;
        case VALUE_ARRAY:
            value->data.array = NULL;
            break;
        case VALUE_OBJECT:
            value->data.object = NULL;
            break;
        case VALUE_FUNCTION:
            value->data.function = NULL;
            break;
        case VALUE_ERROR:
            value->data.error = NULL;
            break;
    }
    
    return value;
}

/**
 * @brief Create string value
 * @param str String to store (will be copied)
 * @return Value* New string value with ref_count = 1, or NULL on failure
 */
Value* value_new_string(const char* str) {
    if (!str) {
        return NULL;
    }
    
    Value* value = value_new(VALUE_STRING);
    if (!value) {
        return NULL;
    }
    
    // Allocate and copy the string
    size_t len = strlen(str);
    value->data.string = malloc(len + 1);
    if (!value->data.string) {
        free(value);
        return NULL;
    }
    
    strcpy(value->data.string, str);
    return value;
}

/**
 * @brief Create number value
 * @param num Number to store
 * @return Value* New number value with ref_count = 1, or NULL on failure
 */
Value* value_new_number(double num) {
    Value* value = value_new(VALUE_NUMBER);
    if (!value) {
        return NULL;
    }
    
    value->data.number = num;
    return value;
}

/**
 * @brief Create boolean value
 * @param val Boolean value to store
 * @return Value* New boolean value with ref_count = 1, or NULL on failure
 */
Value* value_new_boolean(bool val) {
    Value* value = value_new(VALUE_BOOLEAN);
    if (!value) {
        return NULL;
    }
    
    value->data.boolean = val;
    return value;
}

/**
 * @brief Create null value
 * @return Value* New null value with ref_count = 1, or NULL on failure
 */
Value* value_new_null(void) {
    return value_new(VALUE_NULL);
}

/**
 * @brief Deep copy value
 * @param value Value to copy (can be NULL)
 * @return Value* New copy with ref_count = 1, or NULL if input is NULL or on failure
 */
Value* value_copy(const Value* value) {
    if (!value) {
        return NULL;
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return value_new_null();
            
        case VALUE_BOOLEAN:
            return value_new_boolean(value->data.boolean);
            
        case VALUE_NUMBER:
            return value_new_number(value->data.number);
            
        case VALUE_STRING:
            return value_new_string(value->data.string);
            
        case VALUE_ARRAY:
            // TODO: Implement array copying when array.c is ready
            return NULL;
            
        case VALUE_OBJECT:
            // TODO: Implement object copying when object.c is ready  
            return NULL;
            
        case VALUE_FUNCTION:
            // TODO: Implement function copying when functions are implemented
            return NULL;
            
        case VALUE_ERROR:
            return value_new_string(value->data.error);
    }
    
    return NULL;
}

/**
 * @brief Free value memory
 * @param value Value to free (can be NULL)
 * @note This decrements the reference count and only frees if count reaches 0
 */
void value_free(Value* value) {
    if (!value) {
        return;
    }
    
    // Decrement reference count
    value->ref_count--;
    
    // Only free if no more references
    if (value->ref_count > 0) {
        return;
    }
    
    // Free type-specific data
    switch (value->type) {
        case VALUE_STRING:
            free(value->data.string);
            break;
            
        case VALUE_ERROR:
            free(value->data.error);
            break;
            
        case VALUE_ARRAY:
            // TODO: Free array when array.c is implemented
            break;
            
        case VALUE_OBJECT:
            // TODO: Free object when object.c is implemented
            break;
            
        case VALUE_FUNCTION:
            // TODO: Free function when functions are implemented
            break;
            
        case VALUE_NULL:
        case VALUE_BOOLEAN:
        case VALUE_NUMBER:
            // No additional cleanup needed
            break;
    }
    
    // Free the value structure itself
    free(value);
}

/**
 * @brief Convert value to string representation
 * @param value Value to convert (can be NULL)
 * @return char* String representation (caller must free), or NULL on failure
 */
char* value_to_string(const Value* value) {
    if (!value) {
        return NULL;
    }
    
    char* result = NULL;
    char buffer[64];  // Buffer for numeric conversions
    
    switch (value->type) {
        case VALUE_NULL:
            result = malloc(5);
            if (result) strcpy(result, "null");
            break;
            
        case VALUE_BOOLEAN:
            if (value->data.boolean) {
                result = malloc(5);
                if (result) strcpy(result, "true");
            } else {
                result = malloc(6);
                if (result) strcpy(result, "false");
            }
            break;
            
        case VALUE_NUMBER:
            // Handle special cases
            if (isnan(value->data.number)) {
                result = malloc(4);
                if (result) strcpy(result, "NaN");
            } else if (isinf(value->data.number)) {
                if (value->data.number > 0) {
                    result = malloc(9);
                    if (result) strcpy(result, "Infinity");
                } else {
                    result = malloc(10);
                    if (result) strcpy(result, "-Infinity");
                }
            } else {
                // Check if it's effectively an integer
                if (floor(value->data.number) == value->data.number && 
                    value->data.number >= -9007199254740991.0 && 
                    value->data.number <= 9007199254740991.0) {
                    snprintf(buffer, sizeof(buffer), "%.0f", value->data.number);
                } else {
                    snprintf(buffer, sizeof(buffer), "%.15g", value->data.number);
                }
                size_t len = strlen(buffer);
                result = malloc(len + 1);
                if (result) strcpy(result, buffer);
            }
            break;
            
        case VALUE_STRING:
            if (value->data.string) {
                size_t len = strlen(value->data.string);
                result = malloc(len + 1);
                if (result) strcpy(result, value->data.string);
            }
            break;
            
        case VALUE_ARRAY:
            // TODO: Implement array to string when array.c is ready
            result = malloc(8);
            if (result) strcpy(result, "[Array]");
            break;
            
        case VALUE_OBJECT:
            // TODO: Implement object to string when object.c is ready
            result = malloc(9);
            if (result) strcpy(result, "[Object]");
            break;
            
        case VALUE_FUNCTION:
            result = malloc(11);
            if (result) strcpy(result, "[Function]");
            break;
            
        case VALUE_ERROR:
            if (value->data.error) {
                size_t len = strlen(value->data.error);
                result = malloc(len + 1);
                if (result) strcpy(result, value->data.error);
            }
            break;
    }
    
    return result;
}

/**
 * @brief Compare two values for equality
 * @param a First value (can be NULL)
 * @param b Second value (can be NULL)
 * @return bool true if values are equal, false otherwise
 */
bool value_equals(const Value* a, const Value* b) {
    // Handle NULL cases
    if (a == NULL && b == NULL) {
        return true;
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    
    // Same reference
    if (a == b) {
        return true;
    }
    
    // Different types are never equal
    if (a->type != b->type) {
        return false;
    }
    
    // Compare based on type
    switch (a->type) {
        case VALUE_NULL:
            return true;  // All nulls are equal
            
        case VALUE_BOOLEAN:
            return a->data.boolean == b->data.boolean;
            
        case VALUE_NUMBER:
            // Handle NaN: NaN != NaN
            if (isnan(a->data.number) || isnan(b->data.number)) {
                return false;
            }
            return a->data.number == b->data.number;
            
        case VALUE_STRING:
            if (a->data.string == NULL && b->data.string == NULL) {
                return true;
            }
            if (a->data.string == NULL || b->data.string == NULL) {
                return false;
            }
            return strcmp(a->data.string, b->data.string) == 0;
            
        case VALUE_ERROR:
            if (a->data.error == NULL && b->data.error == NULL) {
                return true;
            }
            if (a->data.error == NULL || b->data.error == NULL) {
                return false;
            }
            return strcmp(a->data.error, b->data.error) == 0;
            
        case VALUE_ARRAY:
            // TODO: Implement array equality when array.c is ready
            return a->data.array == b->data.array;  // Reference equality for now
            
        case VALUE_OBJECT:
            // TODO: Implement object equality when object.c is ready
            return a->data.object == b->data.object;  // Reference equality for now
            
        case VALUE_FUNCTION:
            // Functions are compared by reference
            return a->data.function == b->data.function;
    }
    
    return false;
}

/**
 * @brief Get string name of value type
 * @param type Value type
 * @return const char* String name of the type (static string, don't free)
 */
const char* value_type_name(ValueType type) {
    switch (type) {
        case VALUE_NULL:     return "null";
        case VALUE_BOOLEAN:  return "boolean";
        case VALUE_NUMBER:   return "number";
        case VALUE_STRING:   return "string";
        case VALUE_ARRAY:    return "array";
        case VALUE_OBJECT:   return "object";
        case VALUE_FUNCTION: return "function";
        case VALUE_ERROR:    return "error";
    }
    return "unknown";
}

/* Reference counting helpers */

/**
 * @brief Increment reference count of value
 * @param value Value to reference (can be NULL)
 * @return Value* The same value for convenience
 */
Value* value_ref(Value* value) {
    if (value) {
        value->ref_count++;
    }
    return value;
}

/**
 * @brief Decrement reference count and free if zero
 * @param value Value to unreference (can be NULL)
 */
void value_unref(Value* value) {
    value_free(value);  // value_free already handles ref counting
}

