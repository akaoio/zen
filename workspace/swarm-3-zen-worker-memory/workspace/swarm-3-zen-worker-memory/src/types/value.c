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
#include <math.h>
#include <float.h>
#include <errno.h>
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

// Enhanced helper functions for error system and stdlib support
static bool value_validate_type(const Value* value, ValueType expected_type);
static Value* value_new_error_with_message(const char* message, int error_code);
static bool value_is_truthy(const Value* value);
static double value_to_number_safe(const Value* value, bool* success);
static bool value_is_valid_number(double num);

/**
 * @brief Create new value of specified type
 * @param type The type of value to create
 * @return Newly allocated value or NULL on failure
 */
Value* value_new(ValueType type) {
    if (type < VALUE_NULL || type > VALUE_INSTANCE) {
        return NULL;
    }
    
    Value* value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    // Initialize with zeros first
    memset(value, 0, sizeof(Value));
    
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
                memory_free(value);
                return NULL;
            }
            break;
        case VALUE_ARRAY:
            value->as.array = array_create(8); // Default capacity
            if (!value->as.array) {
                memory_free(value);
                return NULL;
            }
            break;
        case VALUE_OBJECT:
            value->as.object = object_create();
            if (!value->as.object) {
                memory_free(value);
                return NULL;
            }
            break;
        case VALUE_FUNCTION:
            value->as.function = NULL; // For future function support
            break;
        case VALUE_ERROR:
            value->as.error = memory_alloc(sizeof(ZenError));
            if (!value->as.error) {
                memory_free(value);
                return NULL;
            }
            memset(value->as.error, 0, sizeof(ZenError));
            value->as.error->message = memory_strdup("Unknown error");
            value->as.error->code = -1;
            break;
        case VALUE_CLASS:
            value->as.class_def = class_create("", NULL);
            if (!value->as.class_def) {
                memory_free(value);
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
    
    Value* value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_STRING;
    value->ref_count = 1;
    value->as.string = string_create(str);
    
    if (!value->as.string) {
        memory_free(value);
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
    Value* value = memory_alloc(sizeof(Value));
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
    Value* value = memory_alloc(sizeof(Value));
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
    Value* value = memory_alloc(sizeof(Value));
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
 */
void value_free(Value* value) {
    if (!value) {
        return;
    }
    
    // Safety check: ensure reference count is 0 before freeing
    if (value->ref_count > 0) {
        #ifdef DEBUG
        fprintf(stderr, "WARNING: Freeing value with ref_count %zu > 0\n", value->ref_count);
        #endif
        // Set ref_count to 0 to prevent further issues
        value->ref_count = 0;
    }
    
    value_free_data(value);
    memory_free(value);
}

/**
 * @brief Convert value to string representation
 * @param value Value to convert
 * @return String representation (caller must free) or NULL on failure
 */
char* value_to_string(const Value* value) {
    if (!value) {
        return memory_strdup("null");
    }
    
    switch (value->type) {
        case VALUE_NULL:
            return memory_strdup("null");
        case VALUE_BOOLEAN:
            return memory_strdup(value->as.boolean ? "true" : "false");
        case VALUE_NUMBER: {
            char* result = memory_alloc(32); // Enough for most numbers
            if (!result) return NULL;
            snprintf(result, 32, "%.15g", value->as.number);
            return result;
        }
        case VALUE_STRING:
            return memory_strdup(value->as.string && value->as.string->data ? value->as.string->data : "");
        case VALUE_ARRAY: {
            // Simple array representation [item1, item2, ...]
            ZenArray* array = value->as.array;
            if (array->length == 0) {
                return memory_strdup("[]");
            }
            
            // Estimate size needed
            size_t total_size = 32; // Base size for brackets and commas
            char** item_strings = memory_alloc(array->length * sizeof(char*));
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
            
            char* result = memory_alloc(total_size);
            if (!result) {
                for (size_t i = 0; i < array->length; i++) {
                    memory_free(item_strings[i]);
                }
                memory_free(item_strings);
                return NULL;
            }
            
            strcpy(result, "[");
            for (size_t i = 0; i < array->length; i++) {
                strcat(result, item_strings[i]);
                if (i < array->length - 1) {
                    strcat(result, ", ");
                }
                memory_free(item_strings[i]);
            }
            strcat(result, "]");
            
            memory_free(item_strings);
            return result;
        }
        case VALUE_OBJECT:
            return memory_strdup("{}"); // Simplified object representation
        case VALUE_FUNCTION:
            return memory_strdup("<function>");
        case VALUE_ERROR:
            if (value->as.error && value->as.error->message) {
                char* result = memory_alloc(strlen(value->as.error->message) + 16);
                if (!result) return NULL;
                sprintf(result, "<error: %s>", value->as.error->message);
                return result;
            }
            return memory_strdup("<error>");
        case VALUE_CLASS:
            if (value->as.class_def && value->as.class_def->name) {
                char* result = memory_alloc(strlen(value->as.class_def->name) + 16);
                if (!result) return NULL;
                sprintf(result, "<class %s>", value->as.class_def->name);
                return result;
            }
            return memory_strdup("<class>");
        case VALUE_INSTANCE:
            if (value->as.instance && value->as.instance->class_def && 
                value->as.instance->class_def->as.class_def && 
                value->as.instance->class_def->as.class_def->name) {
                char* result = memory_alloc(strlen(value->as.instance->class_def->as.class_def->name) + 32);
                if (!result) return NULL;
                sprintf(result, "<instance of %s>", value->as.instance->class_def->as.class_def->name);
                return result;
            }
            return memory_strdup("<instance>");
        default:
            return memory_strdup("<unknown>");
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
 */
Value* value_ref(Value* value) {
    if (value) {
        // Use atomic reference counting for thread safety
        memory_ref_inc(&value->ref_count);
    }
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
    
    // Use atomic decrement for thread safety
    size_t new_count = memory_ref_dec(&value->ref_count);
    
    if (new_count == 0) {
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
    
    ZenArray* array = memory_alloc(sizeof(ZenArray));
    if (!array) {
        return NULL;
    }
    
    array->items = memory_alloc(initial_capacity * sizeof(Value*));
    if (!array->items) {
        memory_free(array);
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
    
    memory_free(array->items);
    memory_free(array);
}

/**
 * @brief Create a new object (hash map)
 * @return Newly allocated object or NULL on failure
 */
static ZenObject* object_create(void) {
    ZenObject* object = memory_alloc(sizeof(ZenObject));
    if (!object) {
        return NULL;
    }
    
    object->capacity = 8; // Initial capacity
    object->pairs = memory_alloc(object->capacity * sizeof(ZenObjectPair));
    if (!object->pairs) {
        memory_free(object);
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
            memory_free(object->pairs[i].key);
        }
        if (object->pairs[i].value) {
            value_unref(object->pairs[i].value);
        }
    }
    
    memory_free(object->pairs);
    memory_free(object);
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
    
    ZenString* zen_str = memory_alloc(sizeof(ZenString));
    if (!zen_str) {
        return NULL;
    }
    
    zen_str->length = strlen(str);
    zen_str->capacity = zen_str->length + 1; // +1 for null terminator
    if (zen_str->capacity < 8) {
        zen_str->capacity = 8; // Minimum capacity
    }
    
    zen_str->data = memory_alloc(zen_str->capacity);
    if (!zen_str->data) {
        memory_free(zen_str);
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
        memory_free(zen_str->data);
    }
    memory_free(zen_str);
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
        memory_free(value);
        return NULL;
    }
    
    return value;
}

/**
 * @brief Create a new instance of a class
 * @param class_def Class definition value
 * @return Newly allocated instance value or NULL on failure
 */
Value* value_new_instance(Value* class_def) {
    if (!class_def || class_def->type != VALUE_CLASS) {
        return NULL;
    }
    
    Value* value = calloc(1, sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    value->type = VALUE_INSTANCE;
    value->ref_count = 1;
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

// Enhanced helper function implementations for error system and stdlib support

/**
 * @brief Validate that a value is of the expected type
 * @param value Value to validate
 * @param expected_type Expected ValueType
 * @return true if value is valid and matches expected type
 */
__attribute__((unused)) static bool value_validate_type(const Value* value, ValueType expected_type) {
    if (!value) {
        return false;
    }
    
    return value->type == expected_type;
}

/**
 * @brief Create a new error value with a specific message and code
 * @param message Error message (will be copied)
 * @param error_code Error code number
 * @return New error Value or NULL on failure
 */
static Value* value_new_error_with_message(const char* message, int error_code) {
    Value* value = memory_alloc(sizeof(Value));
    if (!value) {
        return NULL;
    }
    
    memset(value, 0, sizeof(Value));
    value->type = VALUE_ERROR;
    value->ref_count = 1;
    
    value->as.error = memory_alloc(sizeof(ZenError));
    if (!value->as.error) {
        memory_free(value);
        return NULL;
    }
    
    value->as.error->message = message ? memory_strdup(message) : memory_strdup("Unknown error");
    value->as.error->code = error_code;
    
    if (!value->as.error->message) {
        memory_free(value->as.error);
        memory_free(value);
        return NULL;
    }
    
    return value;
}

/**
 * @brief Determine if a value is "truthy" in ZEN context
 * @param value Value to test
 * @return true if value is truthy, false otherwise
 */
static bool value_is_truthy(const Value* value) {
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
            return value->as.string && value->as.string->data && 
                   strlen(value->as.string->data) > 0;
        case VALUE_ARRAY:
            return value->as.array && value->as.array->length > 0;
        case VALUE_OBJECT:
            return value->as.object && value->as.object->length > 0;
        case VALUE_FUNCTION:
            return value->as.function != NULL;
        case VALUE_ERROR:
            return false; // Errors are falsy
        case VALUE_CLASS:
        case VALUE_INSTANCE:
            return true; // Objects are truthy
        default:
            return false;
    }
}

/**
 * @brief Safely convert a value to a number
 * @param value Value to convert
 * @param success Output parameter indicating if conversion succeeded
 * @return Converted number (0.0 if conversion failed)
 */
static double value_to_number_safe(const Value* value, bool* success) {
    if (success) *success = false;
    
    if (!value) {
        return 0.0;
    }
    
    switch (value->type) {
        case VALUE_NULL:
            if (success) *success = true;
            return 0.0;
        case VALUE_BOOLEAN:
            if (success) *success = true;
            return value->as.boolean ? 1.0 : 0.0;
        case VALUE_NUMBER:
            if (success) *success = true;
            return value->as.number;
        case VALUE_STRING: {
            if (!value->as.string || !value->as.string->data) {
                return 0.0;
            }
            
            char* endptr;
            errno = 0;
            double result = strtod(value->as.string->data, &endptr);
            
            // Check if entire string was consumed and no errors occurred
            if (endptr != value->as.string->data && *endptr == '\0' && errno == 0) {
                if (success) *success = true;
                return result;
            }
            return 0.0;
        }
        default:
            return 0.0; // Can't convert other types
    }
}

/**
 * @brief Check if a double value is a valid number (not NaN or infinity)
 * @param num Number to validate
 * @return true if number is valid and finite
 */
__attribute__((unused)) static bool value_is_valid_number(double num) {
    return !isnan(num) && isfinite(num);
}

// Public utility functions for enhanced type operations

/**
 * @brief Create an error value for stdlib functions
 * @param message Error message
 * @param error_code Error code
 * @return New error Value
 */
Value* value_new_error(const char* message, int error_code) {
    return value_new_error_with_message(message, error_code);
}

/**
 * @brief Check if value is truthy (for conditional expressions)
 * @param value Value to test
 * @return true if truthy, false otherwise
 */
bool value_is_truthy_public(const Value* value) {
    return value_is_truthy(value);
}

/**
 * @brief Safe conversion to number for stdlib operations
 * @param value Value to convert
 * @return Converted number or NaN if conversion fails
 */
double value_to_number_or_nan(const Value* value) {
    bool success;
    double result = value_to_number_safe(value, &success);
    return success ? result : NAN;
}

/**
 * @brief Enhanced string conversion with better error handling
 * @param value Value to convert
 * @return String representation or NULL on critical failure
 */
char* value_to_string_safe(const Value* value) {
    if (!value) {
        return memory_strdup("null");
    }
    
    // Use existing value_to_string but with memory_alloc/memory_strdup
    char* result = value_to_string(value);
    
    // Ensure we always return something, even on failure
    if (!result) {
        result = memory_strdup("<error converting to string>");
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
        return false;
    }
    
    // Same types are always comparable
    if (a->type == b->type) {
        return true;
    }
    
    // Numbers and booleans are comparable with each other
    if ((a->type == VALUE_NUMBER || a->type == VALUE_BOOLEAN) && 
        (b->type == VALUE_NUMBER || b->type == VALUE_BOOLEAN)) {
        return true;
    }
    
    // Strings can be compared with numbers if the string is numeric
    if (a->type == VALUE_STRING && b->type == VALUE_NUMBER) {
        bool success;
        value_to_number_safe(a, &success);
        return success;
    }
    
    if (a->type == VALUE_NUMBER && b->type == VALUE_STRING) {
        bool success;
        value_to_number_safe(b, &success);
        return success;
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
        case VALUE_STRING:
            return value->as.string ? value->as.string->length : 0;
        case VALUE_ARRAY:
            return value->as.array ? value->as.array->length : 0;
        case VALUE_OBJECT:
            return value->as.object ? value->as.object->length : 0;
        default:
            return 0;
    }
}

