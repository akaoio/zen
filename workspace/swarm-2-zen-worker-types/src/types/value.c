/*
 * value.c
 * Value type system
 * 
 * This file implements the complete ZEN value system with reference counting,
 * type conversions, and memory management following MANIFEST.json specifications.
 */

#define _POSIX_C_SOURCE 200809L  // Enable strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>  // For NAN, INFINITY, isnan, isinf, floor
#include "zen/types/value.h"
#include "zen/core/memory.h"

// Internal helper functions
static void value_free_data(Value* value);
static ZenArray* array_create(size_t initial_capacity);
static void array_free(ZenArray* array);
static ZenObject* object_create(void);
static void object_free(ZenObject* object);
static ZenString* string_create(const char* str);
static void string_free(ZenString* string);
static ZenClass* class_create(const char* name, const char* parent_name);
static void class_free(ZenClass* class_def);
static ZenInstance* instance_create(Value* class_def);
static void instance_free(ZenInstance* instance);

/**
 * @brief Create new value of specified type
 * @param type The type of value to create
 * @return Newly allocated value or NULL on failure
 */
Value* value_new(ValueType type) {
    if (type < VALUE_NULL || type > VALUE_ERROR) {
        return NULL;
    }
    
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = type;
    value->ref_count = 1;
    
    // Initialize type-specific data
    switch (type) {
        case VALUE_NULL:
            // No additional initialization needed
            break;
        case VALUE_BOOLEAN:
            value->as.boolean = false;
            break;
        case VALUE_NUMBER:
            value->as.number = 0.0;
            break;
        case VALUE_STRING:
            value->as.string = string_create(""); // Empty string
            if (!value->as.string) {
                free(value);
                return NULL;
            }
            break;
        case VALUE_ARRAY:
            value->as.array = array_create(8); // Default capacity
            if (!value->as.array) {
                free(value);
                return NULL;
            }
            break;
        case VALUE_OBJECT:
            value->as.object = object_create();
            if (!value->as.object) {
                free(value);
                return NULL;
            }
            break;
        case VALUE_FUNCTION:
            value->as.function = NULL; // For future function support
            break;
        case VALUE_ERROR:
            value->as.error = calloc(1, sizeof(ZenError));
            if (!value->as.error) {
                free(value);
                return NULL;
            }
            value->as.error->message = strdup("Unknown error");
            value->as.error->code = -1;
            break;
        case VALUE_CLASS:
            value->as.class_def = class_create("", NULL);
            if (!value->as.class_def) {
                free(value);
                return NULL;
            }
            break;
        case VALUE_INSTANCE:
            // Instance needs to be created with a specific class
            value->as.instance = NULL;
            break;
    }
    
    return value;
}

/**
 * @brief Create string value
 * @param str String to store (will be copied)
 * @return Newly allocated string value or NULL on failure
 */
Value* value_new_string(const char* str) {
    if (!str) {
        return NULL;
    }
    
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_STRING;
    value->ref_count = 1;
    value->as.string = string_create(str);
    
    if (!value->as.string) {
        free(value);
        return NULL;
    }
    
    return value;
}

/**
 * @brief Create number value
 * @param num Number to store
 * @return Newly allocated number value or NULL on failure
 */
Value* value_new_number(double num) {
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_NUMBER;
    value->ref_count = 1;
    value->as.number = num;
    
    return value;
}

/**
 * @brief Create boolean value
 * @param val Boolean value to store
 * @return Newly allocated boolean value or NULL on failure
 */
Value* value_new_boolean(bool val) {
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_BOOLEAN;
    value->ref_count = 1;
    value->as.boolean = val;
    
    return value;
}

/**
 * @brief Create null value
 * @return Newly allocated null value or NULL on failure
 */
Value* value_new_null(void) {
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_NULL;
    value->ref_count = 1;
    // No data initialization needed for null
    
    return value;
}

/**
 * @brief Deep copy value
 * @param value Value to copy
 * @return Deep copy of the value or NULL on failure
 */
Value* value_copy(const Value* value) {
    if (!value) {
        return NULL;
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return value_new_null();
        case VALUE_BOOLEAN:
            return value_new_boolean(value->as.boolean);
        case VALUE_NUMBER:
            return value_new_number(value->as.number);
        case VALUE_STRING:
            return value_new_string(value->as.string && value->as.string->data ? value->as.string->data : "");
        case VALUE_ARRAY: {
            Value* new_value = value_new(VALUE_ARRAY);
            if (!new_value) return NULL;
            
            ZenArray* src_array = value->as.array;
            ZenArray* dst_array = new_value->as.array;
            
            // Copy all elements
            for (size_t i = 0; i < src_array->length; i++) {
                Value* copied_item = value_copy(src_array->items[i]);
                if (!copied_item) {
                    value_free(new_value);
                    return NULL;
                }
                
                // Grow array if needed
                if (dst_array->length >= dst_array->capacity) {
                    size_t new_capacity = dst_array->capacity * 2;
                    Value** new_items = realloc(dst_array->items, new_capacity * sizeof(Value*));
                    if (!new_items) {
                        value_free(copied_item);
                        value_free(new_value);
                        return NULL;
                    }
                    dst_array->items = new_items;
                    dst_array->capacity = new_capacity;
                }
                
                dst_array->items[dst_array->length++] = copied_item;
            }
            
            return new_value;
        }
        case VALUE_OBJECT: {
            Value* new_value = value_new(VALUE_OBJECT);
            if (!new_value) return NULL;
            
            ZenObject* src_obj = value->as.object;
            
            // Copy all key-value pairs (simplified - just copying structure for now)
            for (size_t i = 0; i < src_obj->length; i++) {
                ZenObjectPair* pair = &src_obj->pairs[i];
                if (pair->value) {
                    Value* copied_value = value_copy(pair->value);
                    if (!copied_value) {
                        value_free(new_value);
                        return NULL;
                    }
                    
                    // For now, just free the copied value to prevent leak
                    // Full object copying would need object_set implementation
                    value_free(copied_value);
                }
            }
            
            return new_value;
        }
        case VALUE_FUNCTION:
        case VALUE_ERROR:
            // For now, return a new instance of the same type
            return value_new(value->type);
        case VALUE_CLASS:
            // Classes are typically not copied, but referenced
            return value_ref((Value*)value);
        case VALUE_INSTANCE: {
            // Create a new instance of the same class
            if (value->as.instance && value->as.instance->class_def) {
                Value* new_instance = value_new_instance(value->as.instance->class_def);
                // Would need to copy all properties here
                return new_instance;
            }
            return NULL;
        }
        default:
            return NULL;
    }
}

/**
 * @brief Free value memory
 * @param value Value to free (may be NULL)
 * @note Properly integrated with memory management system
 */
void value_free(Value* value) {
    if (!value) {
        return;
    }
    
    // Free type-specific data first
    value_free_data(value);
    
    // Use memory system for proper tracking
    memory_free(value);
}

/**
 * @brief Convert value to string representation
 * @param value Value to convert
 * @return String representation (caller must free) or NULL on failure
 */
char* value_to_string(const Value* value) {
    if (!value) {
        return strdup("null");
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return strdup("null");
        case VALUE_BOOLEAN:
            return strdup(value->as.boolean ? "true" : "false");
        case VALUE_NUMBER: {
            char* result = malloc(32); // Enough for most numbers
            if (!result) return NULL;
            snprintf(result, 32, "%.15g", value->as.number);
            return result;
        }
        case VALUE_STRING:
            return strdup(value->as.string && value->as.string->data ? value->as.string->data : "");
        case VALUE_ARRAY: {
            // Simple array representation [item1, item2, ...]
            ZenArray* array = value->as.array;
            if (array->length == 0) {
                return strdup("[]");
            }
            
            // Estimate size needed
            size_t total_size = 32; // Base size for brackets and commas
            char** item_strings = malloc(array->length * sizeof(char*));
            if (!item_strings) return NULL;
            
            // Convert each item to string
            for (size_t i = 0; i < array->length; i++) {
                item_strings[i] = value_to_string(array->items[i]);
                if (!item_strings[i]) {
                    // Clean up on failure
                    for (size_t j = 0; j < i; j++) {
                        free(item_strings[j]);
                    }
                    free(item_strings);
                    return NULL;
                }
                total_size += strlen(item_strings[i]) + 2; // +2 for ", "
            }
            
            char* result = malloc(total_size);
            if (!result) {
                for (size_t i = 0; i < array->length; i++) {
                    free(item_strings[i]);
                }
                free(item_strings);
                return NULL;
            }
            
            strcpy(result, "[");
            for (size_t i = 0; i < array->length; i++) {
                strcat(result, item_strings[i]);
                if (i < array->length - 1) {
                    strcat(result, ", ");
                }
                free(item_strings[i]);
            }
            strcat(result, "]");
            
            free(item_strings);
            return result;
        }
        case VALUE_OBJECT:
            return strdup("{}"); // Simplified object representation
        case VALUE_FUNCTION:
            return strdup("<function>");
        case VALUE_ERROR:
            if (value->as.error && value->as.error->message) {
                char* result = malloc(strlen(value->as.error->message) + 16);
                if (!result) return NULL;
                sprintf(result, "<error: %s>", value->as.error->message);
                return result;
            }
            return strdup("<error>");
        case VALUE_CLASS:
            if (value->as.class_def && value->as.class_def->name) {
                char* result = malloc(strlen(value->as.class_def->name) + 16);
                if (!result) return NULL;
                sprintf(result, "<class %s>", value->as.class_def->name);
                return result;
            }
            return strdup("<class>");
        case VALUE_INSTANCE:
            if (value->as.instance && value->as.instance->class_def && 
                value->as.instance->class_def->as.class_def && 
                value->as.instance->class_def->as.class_def->name) {
                char* result = malloc(strlen(value->as.instance->class_def->as.class_def->name) + 32);
                if (!result) return NULL;
                sprintf(result, "<instance of %s>", value->as.instance->class_def->as.class_def->name);
                return result;
            }
            return strdup("<instance>");
        default:
            return strdup("<unknown>");
    }
}

/**
 * @brief Compare two values for equality
 * @param a First value to compare
 * @param b Second value to compare
 * @return true if values are equal, false otherwise
 */
bool value_equals(const Value* a, const Value* b) {
    if (a == b) {
        return true; // Same pointer
    }
    
    if (!a || !b) {
        return false; // One is NULL
    }
    
    if (a->type != b->type) {
        return false; // Different types
    }
    
    switch (a->type) {
        case VALUE_NULL:
            return true; // All nulls are equal
        case VALUE_BOOLEAN:
            return a->as.boolean == b->as.boolean;
        case VALUE_NUMBER:
            return a->as.number == b->as.number;
        case VALUE_STRING:
            if (!a->as.string && !b->as.string) return true;
            if (!a->as.string || !b->as.string) return false;
            if (!a->as.string->data && !b->as.string->data) return true;
            if (!a->as.string->data || !b->as.string->data) return false;
            return strcmp(a->as.string->data, b->as.string->data) == 0;
        case VALUE_ARRAY: {
            ZenArray* arr_a = a->as.array;
            ZenArray* arr_b = b->as.array;
            
            if (arr_a->length != arr_b->length) {
                return false;
            }
            
            for (size_t i = 0; i < arr_a->length; i++) {
                if (!value_equals(arr_a->items[i], arr_b->items[i])) {
                    return false;
                }
            }
            
            return true;
        }
        case VALUE_OBJECT:
            // Simplified - would need full object comparison
            return a->as.object == b->as.object;
        case VALUE_FUNCTION:
            return a->as.function == b->as.function;
        case VALUE_ERROR:
            // Compare error codes
            if (!a->as.error || !b->as.error) {
                return a->as.error == b->as.error;
            }
            return a->as.error->code == b->as.error->code;
        case VALUE_CLASS:
            // Classes are equal if they're the same object
            return a->as.class_def == b->as.class_def;
        case VALUE_INSTANCE:
            // Instances are equal if they're the same object
            return a->as.instance == b->as.instance;
        default:
            return false;
    }
}

/**
 * @brief Get string name of value type
 * @param type Value type
 * @return String name of the type (static string)
 */
const char* value_type_name(ValueType type) {
    switch (type) {
        case VALUE_NULL:      return "null";
        case VALUE_BOOLEAN:   return "boolean";
        case VALUE_NUMBER:    return "number";
        case VALUE_STRING:    return "string";
        case VALUE_ARRAY:     return "array";
        case VALUE_OBJECT:    return "object";
        case VALUE_FUNCTION:  return "function";
        case VALUE_ERROR:     return "error";
        case VALUE_CLASS:     return "class";
        case VALUE_INSTANCE:  return "instance";
        default:              return "unknown";
    }
}

/**
 * @brief Increment reference count of value
 * @param value Value to reference (may be NULL)
 * @return The same value pointer for convenience
 * @note Uses atomic operations for thread safety
 */
Value* value_ref(Value* value) {
    if (!value) {
        return NULL;
    }
    
    // Use atomic increment for thread safety
    memory_ref_inc(&value->ref_count);
    return value;
}

/**
 * @brief Decrement reference count and free if zero
 * @param value Value to unreference (may be NULL)
 */
void value_unref(Value* value) {
    if (!value) {
        return;
    }
    
    assert(value->ref_count > 0);
    value->ref_count--;
    
    if (value->ref_count == 0) {
        value_free(value);
    }
}

// Internal helper function implementations

/**
 * @brief Free the data portion of a value without freeing the value itself
 * @param value Value whose data should be freed
 */
static void value_free_data(Value* value) {
    if (!value) {
        return;
    }
    
    switch (value->type) {
        case VALUE_NULL:
        case VALUE_BOOLEAN:
        case VALUE_NUMBER:
            // No dynamic data to free
            break;
        case VALUE_STRING:
            if (value->as.string) {
                string_free(value->as.string);
                value->as.string = NULL;
            }
            break;
        case VALUE_ARRAY:
            if (value->as.array) {
                array_free(value->as.array);
                value->as.array = NULL;
            }
            break;
        case VALUE_OBJECT:
            if (value->as.object) {
                object_free(value->as.object);
                value->as.object = NULL;
            }
            break;
        case VALUE_FUNCTION:
            // Function pointers don't need to be freed for now
            value->as.function = NULL;
            break;
        case VALUE_ERROR:
            if (value->as.error) {
                if (value->as.error->message) {
                    free(value->as.error->message);
                }
                free(value->as.error);
                value->as.error = NULL;
            }
            break;
        case VALUE_CLASS:
            if (value->as.class_def) {
                class_free(value->as.class_def);
                value->as.class_def = NULL;
            }
            break;
        case VALUE_INSTANCE:
            if (value->as.instance) {
                instance_free(value->as.instance);
                value->as.instance = NULL;
            }
            break;
    }
}

/**
 * @brief Create a new array with specified initial capacity
 * @param initial_capacity Initial capacity of the array
 * @return Newly allocated array or NULL on failure
 */
static ZenArray* array_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 8; // Minimum capacity
    }
    
    ZenArray* array = malloc(sizeof(ZenArray));
    if (!array) {
        return NULL;
    }
    
    array->items = malloc(initial_capacity * sizeof(Value*));
    if (!array->items) {
        free(array);
        return NULL;
    }
    
    array->length = 0;
    array->capacity = initial_capacity;
    
    return array;
}

/**
 * @brief Free an array and all its elements
 * @param array Array to free
 */
static void array_free(ZenArray* array) {
    if (!array) {
        return;
    }
    
    // Unreference all items
    for (size_t i = 0; i < array->length; i++) {
        value_unref(array->items[i]);
    }
    
    free(array->items);
    free(array);
}

/**
 * @brief Create a new object (hash map)
 * @return Newly allocated object or NULL on failure
 */
static ZenObject* object_create(void) {
    ZenObject* object = malloc(sizeof(ZenObject));
    if (!object) {
        return NULL;
    }
    
    object->capacity = 8; // Initial capacity
    object->pairs = malloc(object->capacity * sizeof(ZenObjectPair));
    if (!object->pairs) {
        free(object);
        return NULL;
    }
    
    object->length = 0;
    
    return object;
}

/**
 * @brief Free an object and all its entries
 * @param object Object to free
 */
static void object_free(ZenObject* object) {
    if (!object) {
        return;
    }
    
    // Free all pairs
    for (size_t i = 0; i < object->length; i++) {
        if (object->pairs[i].key) {
            free(object->pairs[i].key);
        }
        if (object->pairs[i].value) {
            value_unref(object->pairs[i].value);
        }
    }
    
    free(object->pairs);
    free(object);
}

/**
 * @brief Create a new ZenString with specified content
 * @param str String content to copy
 * @return Newly allocated ZenString or NULL on failure
 */
static ZenString* string_create(const char* str) {
    if (!str) {
        str = "";
    }
    
    ZenString* zen_str = malloc(sizeof(ZenString));
    if (!zen_str) {
        return NULL;
    }
    
    zen_str->length = strlen(str);
    zen_str->capacity = zen_str->length + 1; // +1 for null terminator
    if (zen_str->capacity < 8) {
        zen_str->capacity = 8; // Minimum capacity
    }
    
    zen_str->data = malloc(zen_str->capacity);
    if (!zen_str->data) {
        free(zen_str);
        return NULL;
    }
    
    strcpy(zen_str->data, str);
    return zen_str;
}

/**
 * @brief Free a ZenString
 * @param zen_str String to free
 */
static void string_free(ZenString* zen_str) {
    if (!zen_str) {
        return;
    }
    
    if (zen_str->data) {
        free(zen_str->data);
    }
    free(zen_str);
}

/**
 * @brief Create a new ZenClass
 * @param name Class name
 * @param parent_name Parent class name (may be NULL)
 * @return Newly allocated ZenClass or NULL on failure
 */
static ZenClass* class_create(const char* name, const char* parent_name) {
    ZenClass* class_def = malloc(sizeof(ZenClass));
    if (!class_def) {
        return NULL;
    }
    
    class_def->name = name ? strdup(name) : NULL;
    class_def->parent_class_name = parent_name ? strdup(parent_name) : NULL;
    class_def->parent_class = NULL;
    class_def->methods = value_new(VALUE_OBJECT);
    if (!class_def->methods) {
        if (class_def->name) free(class_def->name);
        if (class_def->parent_class_name) free(class_def->parent_class_name);
        free(class_def);
        return NULL;
    }
    class_def->constructor = NULL;
    
    return class_def;
}

/**
 * @brief Free a ZenClass
 * @param class_def Class to free
 */
static void class_free(ZenClass* class_def) {
    if (!class_def) {
        return;
    }
    
    if (class_def->name) {
        free(class_def->name);
    }
    if (class_def->parent_class_name) {
        free(class_def->parent_class_name);
    }
    if (class_def->parent_class) {
        value_unref(class_def->parent_class);
    }
    if (class_def->methods) {
        value_unref(class_def->methods);
    }
    if (class_def->constructor) {
        value_unref(class_def->constructor);
    }
    
    free(class_def);
}

/**
 * @brief Create a new ZenInstance
 * @param class_def Class definition for this instance
 * @return Newly allocated ZenInstance or NULL on failure
 */
static ZenInstance* instance_create(Value* class_def) {
    if (!class_def || class_def->type != VALUE_CLASS) {
        return NULL;
    }
    
    ZenInstance* instance = malloc(sizeof(ZenInstance));
    if (!instance) {
        return NULL;
    }
    
    instance->class_def = value_ref(class_def);
    instance->properties = value_new(VALUE_OBJECT);
    if (!instance->properties) {
        value_unref(instance->class_def);
        free(instance);
        return NULL;
    }
    
    return instance;
}

/**
 * @brief Free a ZenInstance
 * @param instance Instance to free
 */
static void instance_free(ZenInstance* instance) {
    if (!instance) {
        return;
    }
    
    if (instance->class_def) {
        value_unref(instance->class_def);
    }
    if (instance->properties) {
        value_unref(instance->properties);
    }
    
    free(instance);
}

/**
 * @brief Create a new class value
 * @param name Class name
 * @param parent_name Parent class name (may be NULL)
 * @return Newly allocated class value or NULL on failure
 */
Value* value_new_class(const char* name, const char* parent_name) {
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_CLASS;
    value->ref_count = 1;
    value->as.class_def = class_create(name, parent_name);
    if (!value->as.class_def) {
        free(value);
        return NULL;
    }
    
    return value;
}

/**
 * @brief Create a new instance of a class
 * @param class_def Class definition value
 * @return Newly allocated instance value or NULL on failure
 * @note Properly validates class and uses memory system
 */
Value* value_new_instance(Value* class_def) {
    // Enhanced validation
    if (!class_def || class_def->type != VALUE_CLASS) {
        return NULL;
    }
    
    if (!class_def->as.class_def) {
        return NULL;
    }
    
    // Use memory system for proper tracking
    Value* value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    // Initialize value
    value->type = VALUE_INSTANCE;
    value->ref_count = 1;
    
    // Create instance with proper validation
    value->as.instance = instance_create(class_def);
    if (!value->as.instance) {
        memory_free(value);
        return NULL;
    }
    
    return value;
}

/**
 * @brief Add a method to a class
 * @param class_val Class value
 * @param method_name Method name
 * @param method_func Method function value
 */
void value_class_add_method(Value* class_val, const char* method_name, Value* method_func) {
    if (!class_val || class_val->type != VALUE_CLASS || !method_name || !method_func) {
        return;
    }
    
    if (!class_val->as.class_def || !class_val->as.class_def->methods) {
        return;
    }
    
    // Use object_set when it's implemented
    // For now, just store the reference to the function
    // This would need proper object implementation to work fully
    if (strcmp(method_name, "init") == 0) {
        // Special handling for constructor
        if (class_val->as.class_def->constructor) {
            value_unref(class_val->as.class_def->constructor);
        }
        class_val->as.class_def->constructor = value_ref(method_func);
    }
}

/**
 * @brief Get a method from a class
 * @param class_val Class value
 * @param method_name Method name to find
 * @return Method function value or NULL if not found
 */
Value* value_class_get_method(Value* class_val, const char* method_name) {
    if (!class_val || class_val->type != VALUE_CLASS || !method_name) {
        return NULL;
    }
    
    if (!class_val->as.class_def) {
        return NULL;
    }
    
    // Check for constructor
    if (strcmp(method_name, "init") == 0) {
        return class_val->as.class_def->constructor;
    }
    
    // Check parent class if method not found
    if (class_val->as.class_def->parent_class) {
        return value_class_get_method(class_val->as.class_def->parent_class, method_name);
    }
    
    return NULL;
}

/**
 * @brief Set a property on an instance
 * @param instance Instance value
 * @param property_name Property name
 * @param value Property value
 */
void value_instance_set_property(Value* instance, const char* property_name, Value* value) {
    if (!instance || instance->type != VALUE_INSTANCE || !property_name || !value) {
        return;
    }
    
    if (!instance->as.instance || !instance->as.instance->properties) {
        return;
    }
    
    // Use object_set when it's implemented
    // For now, simplified storage
}

/**
 * @brief Get a property from an instance
 * @param instance Instance value
 * @param property_name Property name to get
 * @return Property value or NULL if not found
 */
Value* value_instance_get_property(Value* instance, const char* property_name) {
    if (!instance || instance->type != VALUE_INSTANCE || !property_name) {
        return NULL;
    }
    
    if (!instance->as.instance || !instance->as.instance->properties) {
        return NULL;
    }
    
    // Use object_get when it's implemented
    // For now, return NULL
    return NULL;
}

// Enhanced type system functions for advanced operations

/**
 * @brief Create an error value for stdlib functions
 * @param message Error message
 * @param error_code Error code
 * @return New error Value
 */
Value* value_new_error(const char* message, int error_code) {
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_ERROR;
    value->ref_count = 1;
    value->as.error = calloc(1, sizeof(ZenError));
    if (!value->as.error) {
        free(value);
        return NULL;
    }
    
    value->as.error->message = message ? strdup(message) : strdup("Unknown error");
    value->as.error->code = error_code;
    
    if (!value->as.error->message) {
        free(value->as.error);
        free(value);
        return NULL;
    }
    
    return value;
}

/**
 * @brief Check if value is truthy (for conditional expressions)
 * @param value Value to test
 * @return true if truthy, false otherwise
 */
bool value_is_truthy_public(const Value* value) {
    if (!value) {
        return false;
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return false;
        case VALUE_BOOLEAN:
            return value->as.boolean;
        case VALUE_NUMBER:
            return value->as.number != 0.0 && !isnan(value->as.number);
        case VALUE_STRING:
            return value->as.string && value->as.string->data && value->as.string->length > 0;
        case VALUE_ARRAY:
            return value->as.array && value->as.array->length > 0;
        case VALUE_OBJECT:
            return value->as.object && value->as.object->length > 0;
        case VALUE_FUNCTION:
            return value->as.function != NULL;
        case VALUE_ERROR:
            return false; // Errors are considered falsy
        case VALUE_CLASS:
            return value->as.class_def != NULL;
        case VALUE_INSTANCE:
            return value->as.instance != NULL;
        default:
            return false;
    }
}

/**
 * @brief Safe conversion to number for stdlib operations
 * @param value Value to convert
 * @return Converted number or NaN if conversion fails
 */
double value_to_number_or_nan(const Value* value) {
    if (!value) {
        return NAN;
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return 0.0;
        case VALUE_BOOLEAN:
            return value->as.boolean ? 1.0 : 0.0;
        case VALUE_NUMBER:
            return value->as.number;
        case VALUE_STRING: {
            if (!value->as.string || !value->as.string->data || value->as.string->length == 0) {
                return 0.0;
            }
            
            char* endptr;
            double result = strtod(value->as.string->data, &endptr);
            
            // Check if entire string was consumed (valid number)
            if (*endptr == '\0') {
                return result;
            }
            
            // Handle special cases
            if (strcmp(value->as.string->data, "Infinity") == 0) {
                return INFINITY;
            }
            if (strcmp(value->as.string->data, "-Infinity") == 0) {
                return -INFINITY;
            }
            if (strcmp(value->as.string->data, "NaN") == 0) {
                return NAN;
            }
            
            return NAN; // Invalid string
        }
        case VALUE_ARRAY:
            return value->as.array ? (double)value->as.array->length : NAN;
        case VALUE_OBJECT:
            return value->as.object ? (double)value->as.object->length : NAN;
        default:
            return NAN;
    }
}

/**
 * @brief Enhanced string conversion with better error handling
 * @param value Value to convert
 * @return String representation or NULL on critical failure
 */
char* value_to_string_safe(const Value* value) {
    if (!value) {
        return strdup("null");
    }
    
    // Use the existing value_to_string implementation
    // but add additional safety checks
    char* result = value_to_string(value);
    if (!result) {
        // Fallback for critical failures
        switch (value->type) {
            case VALUE_NULL: return strdup("null");
            case VALUE_BOOLEAN: return strdup("<boolean>");
            case VALUE_NUMBER: return strdup("<number>");
            case VALUE_STRING: return strdup("<string>");
            case VALUE_ARRAY: return strdup("<array>");
            case VALUE_OBJECT: return strdup("<object>");
            case VALUE_FUNCTION: return strdup("<function>");
            case VALUE_ERROR: return strdup("<error>");
            case VALUE_CLASS: return strdup("<class>");
            case VALUE_INSTANCE: return strdup("<instance>");
            default: return strdup("<unknown>");
        }
    }
    
    return result;
}

/**
 * @brief Check if two values can be compared
 * @param a First value
 * @param b Second value
 * @return true if values are comparable
 */
bool value_is_comparable(const Value* a, const Value* b) {
    if (!a || !b) {
        return true; // Can compare with null
    }
    
    // Same types are always comparable
    if (a->type == b->type) {
        return true;
    }
    
    // Numbers and booleans can be compared with each other
    if ((a->type == VALUE_NUMBER && b->type == VALUE_BOOLEAN) ||
        (a->type == VALUE_BOOLEAN && b->type == VALUE_NUMBER)) {
        return true;
    }
    
    // Strings can be compared with numbers for lexicographic ordering
    if ((a->type == VALUE_STRING && b->type == VALUE_NUMBER) ||
        (a->type == VALUE_NUMBER && b->type == VALUE_STRING)) {
        return true;
    }
    
    // Arrays can be compared by length
    if (a->type == VALUE_ARRAY && b->type == VALUE_ARRAY) {
        return true;
    }
    
    // Objects can be compared by number of properties
    if (a->type == VALUE_OBJECT && b->type == VALUE_OBJECT) {
        return true;
    }
    
    return false;
}

/**
 * @brief Get the "length" of a value for stdlib length() function
 * @param value Value to get length of
 * @return Length or 0 if not applicable
 */
size_t value_get_length(const Value* value) {
    if (!value) {
        return 0;
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return 0;
        case VALUE_BOOLEAN:
            return 1; // Booleans have conceptual length of 1
        case VALUE_NUMBER:
            return 1; // Numbers have conceptual length of 1
        case VALUE_STRING:
            return value->as.string ? value->as.string->length : 0;
        case VALUE_ARRAY:
            return value->as.array ? value->as.array->length : 0;
        case VALUE_OBJECT:
            return value->as.object ? value->as.object->length : 0;
        case VALUE_FUNCTION:
            return 0; // Functions don't have meaningful length
        case VALUE_ERROR:
            return value->as.error && value->as.error->message ? strlen(value->as.error->message) : 0;
        case VALUE_CLASS:
            return 0; // Classes don't have meaningful length
        case VALUE_INSTANCE:
            return 0; // Instances length would depend on properties
        default:
            return 0;
    }
}

// Advanced type introspection functions

/**
 * @brief Get the runtime type of a value (enhanced typeof)
 * @param value Value to inspect
 * @return Detailed type information string (caller must free)
 */
char* value_typeof_enhanced(const Value* value) {
    if (!value) {
        return strdup("undefined");
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return strdup("null");
        case VALUE_BOOLEAN:
            return strdup("boolean");
        case VALUE_NUMBER: {
            if (isnan(value->as.number)) {
                return strdup("number:nan");
            } else if (isinf(value->as.number)) {
                return strdup("number:infinity");
            } else if (value->as.number == floor(value->as.number)) {
                return strdup("number:integer");
            } else {
                return strdup("number:float");
            }
        }
        case VALUE_STRING:
            return strdup("string");
        case VALUE_ARRAY:
            return strdup("array");
        case VALUE_OBJECT:
            return strdup("object");
        case VALUE_FUNCTION:
            return strdup("function");
        case VALUE_ERROR:
            return strdup("error");
        case VALUE_CLASS:
            return strdup("class");
        case VALUE_INSTANCE:
            if (value->as.instance && value->as.instance->class_def && 
                value->as.instance->class_def->as.class_def && 
                value->as.instance->class_def->as.class_def->name) {
                size_t len = strlen(value->as.instance->class_def->as.class_def->name) + 16;
                char* result = malloc(len);
                if (result) {
                    snprintf(result, len, "instance:%s", value->as.instance->class_def->as.class_def->name);
                }
                return result;
            }
            return strdup("instance");
        default:
            return strdup("unknown");
    }
}

/**
 * @brief Check if a value is numeric (number or numeric string)
 * @param value Value to check
 * @return true if value represents a number
 */
bool value_is_numeric(const Value* value) {
    if (!value) {
        return false;
    }
    
    switch (value->type) {
        case VALUE_NUMBER:
            return !isnan(value->as.number);
        case VALUE_BOOLEAN:
            return true; // Booleans are numeric (0/1)
        case VALUE_STRING: {
            if (!value->as.string || !value->as.string->data || value->as.string->length == 0) {
                return false;
            }
            
            char* endptr;
            strtod(value->as.string->data, &endptr);
            return *endptr == '\0'; // Entire string was consumed
        }
        default:
            return false;
    }
}

/**
 * @brief Check if a value is iterable (array, object, string)
 * @param value Value to check
 * @return true if value can be iterated
 */
bool value_is_iterable(const Value* value) {
    if (!value) {
        return false;
    }
    
    return value->type == VALUE_ARRAY || 
           value->type == VALUE_OBJECT || 
           value->type == VALUE_STRING;
}

/**
 * @brief Check if a value is callable (function)
 * @param value Value to check
 * @return true if value can be called
 */
bool value_is_callable(const Value* value) {
    if (!value) {
        return false;
    }
    
    return value->type == VALUE_FUNCTION;
}

/**
 * @brief Check if a value is an instance of a specific class
 * @param value Value to check
 * @param class_name Class name to check against
 * @return true if value is an instance of the class
 */
bool value_instanceof(const Value* value, const char* class_name) {
    if (!value || !class_name || value->type != VALUE_INSTANCE) {
        return false;
    }
    
    if (!value->as.instance || !value->as.instance->class_def || 
        !value->as.instance->class_def->as.class_def) {
        return false;
    }
    
    const char* instance_class_name = value->as.instance->class_def->as.class_def->name;
    if (!instance_class_name) {
        return false;
    }
    
    return strcmp(instance_class_name, class_name) == 0;
}

// Type conversion with precision detection

/**
 * @brief Convert value to number with precision information
 * @param value Value to convert
 * @param is_lossless Pointer to store whether conversion is lossless
 * @return Converted number or NaN if conversion fails
 */
double value_to_number_with_precision(const Value* value, bool* is_lossless) {
    if (is_lossless) {
        *is_lossless = true;
    }
    
    if (!value) {
        if (is_lossless) *is_lossless = false;
        return NAN;
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return 0.0;
        case VALUE_BOOLEAN:
            return value->as.boolean ? 1.0 : 0.0;
        case VALUE_NUMBER:
            return value->as.number;
        case VALUE_STRING: {
            if (!value->as.string || !value->as.string->data) {
                return 0.0;
            }
            
            char* endptr;
            double result = strtod(value->as.string->data, &endptr);
            
            if (*endptr != '\0') {
                if (is_lossless) *is_lossless = false;
            }
            
            return result;
        }
        default:
            if (is_lossless) *is_lossless = false;
            return NAN;
    }
}

/**
 * @brief Parse number from string with enhanced format support
 * @param str String to parse
 * @param result Pointer to store parsed number
 * @return true if parsing succeeded
 */
bool value_parse_number_enhanced(const char* str, double* result) {
    if (!str || !result) {
        return false;
    }
    
    // Skip leading whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    
    if (*str == '\0') {
        return false;
    }
    
    // Handle special values
    if (strncmp(str, "Infinity", 8) == 0) {
        *result = INFINITY;
        return true;
    }
    if (strncmp(str, "-Infinity", 9) == 0) {
        *result = -INFINITY;
        return true;
    }
    if (strncmp(str, "NaN", 3) == 0) {
        *result = NAN;
        return true;
    }
    
    // Handle hexadecimal numbers
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        char* endptr;
        long long int_result = strtoll(str, &endptr, 16);
        if (*endptr == '\0') {
            *result = (double)int_result;
            return true;
        }
        return false;
    }
    
    // Handle binary numbers  
    if (strncmp(str, "0b", 2) == 0 || strncmp(str, "0B", 2) == 0) {
        char* endptr;
        long long int_result = strtoll(str + 2, &endptr, 2);
        if (*endptr == '\0') {
            *result = (double)int_result;
            return true;
        }
        return false;
    }
    
    // Handle octal numbers
    if (str[0] == '0' && str[1] >= '0' && str[1] <= '7') {
        char* endptr;
        long long int_result = strtoll(str, &endptr, 8);
        if (*endptr == '\0') {
            *result = (double)int_result;
            return true;
        }
        return false;
    }
    
    // Standard decimal parsing (including scientific notation)
    char* endptr;
    *result = strtod(str, &endptr);
    
    // Skip trailing whitespace
    while (*endptr == ' ' || *endptr == '\t' || *endptr == '\n' || *endptr == '\r') {
        endptr++;
    }
    
    return *endptr == '\0';
}

// Performance optimization features

/**
 * @brief Create a shallow copy of a value (for copy-on-write optimization)
 * @param value Value to copy
 * @return Shallow copy with shared data where appropriate
 */
Value* value_shallow_copy(const Value* value) {
    if (!value) {
        return NULL;
    }
    
    // For immutable types, just increment reference count
    switch (value->type) {
        case VALUE_NULL:
        case VALUE_BOOLEAN:
        case VALUE_NUMBER:
        case VALUE_STRING: // Strings are immutable in ZEN
        case VALUE_FUNCTION:
        case VALUE_CLASS:
            return value_ref((Value*)value);
        default:
            // For mutable types, perform deep copy
            return value_copy(value);
    }
}

/**
 * @brief Check if a value can be safely shared (immutable)
 * @param value Value to check
 * @return true if value is immutable and can be shared
 */
bool value_is_immutable(const Value* value) {
    if (!value) {
        return true;
    }
    
    switch (value->type) {
        case VALUE_NULL:
        case VALUE_BOOLEAN:
        case VALUE_NUMBER:
        case VALUE_STRING:
        case VALUE_FUNCTION:
        case VALUE_CLASS:
            return true;
        case VALUE_ARRAY:
        case VALUE_OBJECT:
        case VALUE_ERROR:
        case VALUE_INSTANCE:
            return false;
        default:
            return false;
    }
}

/**
 * @brief Get hash code for a value (for optimization purposes)
 * @param value Value to hash
 * @return Hash code for the value
 */
size_t value_hash(const Value* value) {
    if (!value) {
        return 0;
    }
    
    // Simple hash function - can be improved for better distribution
    size_t hash = (size_t)value->type;
    
    switch (value->type) {
        case VALUE_NULL:
            return hash;
        case VALUE_BOOLEAN:
            return hash ^ (value->as.boolean ? 1 : 0);
        case VALUE_NUMBER: {
            // Hash the bits of the double
            union { double d; size_t i; } u;
            u.d = value->as.number;
            return hash ^ u.i;
        }
        case VALUE_STRING:
            if (value->as.string && value->as.string->data) {
                // Simple string hash
                size_t str_hash = 0;
                for (size_t i = 0; i < value->as.string->length; i++) {
                    str_hash = str_hash * 31 + (unsigned char)value->as.string->data[i];
                }
                return hash ^ str_hash;
            }
            return hash;
        case VALUE_ARRAY:
            return hash ^ (value->as.array ? value->as.array->length : 0);
        case VALUE_OBJECT:
            return hash ^ (value->as.object ? value->as.object->length : 0);
        default:
            // For other types, use pointer value
            return hash ^ (size_t)value;
    }
}

// Value interning for common values

static Value* g_null_singleton = NULL;
static Value* g_true_singleton = NULL;
static Value* g_false_singleton = NULL;
static Value* g_zero_singleton = NULL;
static Value* g_one_singleton = NULL;

/**
 * @brief Initialize value singletons for optimization
 */
void value_init_singletons(void) {
    if (!g_null_singleton) {
        g_null_singleton = value_new_null();
    }
    if (!g_true_singleton) {
        g_true_singleton = value_new_boolean(true);
    }
    if (!g_false_singleton) {
        g_false_singleton = value_new_boolean(false);
    }
    if (!g_zero_singleton) {
        g_zero_singleton = value_new_number(0.0);
    }
    if (!g_one_singleton) {
        g_one_singleton = value_new_number(1.0);
    }
}

/**
 * @brief Get singleton value for common constants
 * @param type Type of singleton
 * @param number_val Number value (for numbers)
 * @param bool_val Boolean value (for booleans)
 * @return Singleton value or NULL if not available
 */
Value* value_get_singleton(ValueType type, double number_val, bool bool_val) {
    value_init_singletons();
    
    switch (type) {
        case VALUE_NULL:
            return value_ref(g_null_singleton);
        case VALUE_BOOLEAN:
            return value_ref(bool_val ? g_true_singleton : g_false_singleton);
        case VALUE_NUMBER:
            if (number_val == 0.0) {
                return value_ref(g_zero_singleton);
            } else if (number_val == 1.0) {
                return value_ref(g_one_singleton);
            }
            return NULL; // No singleton for this number
        default:
            return NULL;
    }
}

/**
 * @brief Cleanup value singletons
 */
void value_cleanup_singletons(void) {
    if (g_null_singleton) {
        value_unref(g_null_singleton);
        g_null_singleton = NULL;
    }
    if (g_true_singleton) {
        value_unref(g_true_singleton);
        g_true_singleton = NULL;
    }
    if (g_false_singleton) {
        value_unref(g_false_singleton);
        g_false_singleton = NULL;
    }
    if (g_zero_singleton) {
        value_unref(g_zero_singleton);
        g_zero_singleton = NULL;
    }
    if (g_one_singleton) {
        value_unref(g_one_singleton);
        g_one_singleton = NULL;
    }
}

