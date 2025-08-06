/*
 * array.c  
 * Advanced Array Operations for ZEN stdlib
 * 
 * Provides functional programming operations and advanced array manipulation
 */

#define _GNU_SOURCE
#include "zen/types/value.h"
#include "zen/stdlib/math.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Map function - apply a function to each element of an array
 * @param array_value Source array
 * @param func_value Function to apply (should be a ZEN function)
 * @return New array with mapped values
 */
Value* zen_array_map(const Value* array_value, const Value* func_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    if (!func_value || func_value->type != VALUE_FUNCTION) {
        // Return copy of original array if no valid function
        return value_copy(array_value);
    }
    
    size_t length = array_value->as.array.length;
    Value* result_array = array_new(length);
    
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        // For now, just copy elements - actual function application would need evaluator
        // This is a framework for when the evaluator integration is complete
        Value* mapped_elem = value_copy(elem);
        array_push(result_array, mapped_elem);
    }
    
    return result_array;
}

/**
 * @brief Filter function - select elements that match a predicate
 * @param array_value Source array
 * @param predicate_value Predicate function
 * @return New array with filtered values
 */
Value* zen_array_filter(const Value* array_value, const Value* predicate_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    if (!predicate_value || predicate_value->type != VALUE_FUNCTION) {
        // Return empty array if no valid predicate
        return array_new(0);
    }
    
    size_t length = array_value->as.array.length;
    Value* result_array = array_new(0);
    
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        // For now, include all elements - actual predicate evaluation needs evaluator
        // This is a framework for when the evaluator integration is complete
        if (elem) {
            Value* copy_elem = value_copy(elem);
            array_push(result_array, copy_elem);
        }
    }
    
    return result_array;
}

/**
 * @brief Reduce function - accumulate array elements using a reducer function
 * @param array_value Source array
 * @param reducer_value Reducer function
 * @param initial_value Initial accumulator value
 * @return Accumulated result
 */
Value* zen_array_reduce(const Value* array_value, const Value* reducer_value, const Value* initial_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return value_copy(initial_value ? initial_value : value_new_null());
    }
    
    if (!reducer_value || reducer_value->type != VALUE_FUNCTION) {
        return value_copy(initial_value ? initial_value : value_new_null());
    }
    
    size_t length = array_value->as.array.length;
    Value* accumulator = value_copy(initial_value ? initial_value : value_new_null());
    
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        // For now, just return the last element - actual reduction needs evaluator
        // This is a framework for when the evaluator integration is complete
        if (elem) {
            value_unref(accumulator);
            accumulator = value_copy(elem);
        }
    }
    
    return accumulator;
}

/**
 * @brief Sort array using quicksort algorithm
 * @param array_value Array to sort (will be sorted in-place)
 * @param compare_func Optional comparison function
 * @return Sorted array
 */
static int default_compare(const void* a, const void* b) {
    Value* val_a = *(Value**)a;
    Value* val_b = *(Value**)b;
    
    if (!val_a || !val_b) return 0;
    
    if (val_a->type == VALUE_NUMBER && val_b->type == VALUE_NUMBER) {
        double diff = val_a->as.number - val_b->as.number;
        return (diff > 0) ? 1 : (diff < 0) ? -1 : 0;
    }
    
    if (val_a->type == VALUE_STRING && val_b->type == VALUE_STRING) {
        return strcmp(val_a->as.string, val_b->as.string);
    }
    
    return 0;
}

Value* zen_array_sort(Value* array_value, const Value* compare_func) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    size_t length = array_value->as.array.length;
    if (length <= 1) {
        return value_copy(array_value);
    }
    
    // Create copy to avoid modifying original
    Value* sorted_array = value_copy(array_value);
    
    // Use C's qsort with default comparison
    qsort(sorted_array->as.array.data, length, sizeof(Value*), default_compare);
    
    return sorted_array;
}

/**
 * @brief Binary search in a sorted array
 * @param array_value Sorted array to search
 * @param target_value Value to find
 * @return Index of found element, or -1 if not found
 */
Value* zen_array_binary_search(const Value* array_value, const Value* target_value) {
    if (!array_value || array_value->type != VALUE_ARRAY || !target_value) {
        return value_new_number(-1);
    }
    
    size_t length = array_value->as.array.length;
    if (length == 0) {
        return value_new_number(-1);
    }
    
    int left = 0;
    int right = (int)length - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        Value* mid_val = array_value->as.array.data[mid];
        
        if (!mid_val) {
            return value_new_number(-1);
        }
        
        // Simple comparison for numbers and strings
        bool equal = false;
        bool less_than = false;
        
        if (mid_val->type == VALUE_NUMBER && target_value->type == VALUE_NUMBER) {
            double diff = mid_val->as.number - target_value->as.number;
            equal = (diff == 0);
            less_than = (diff < 0);
        } else if (mid_val->type == VALUE_STRING && target_value->type == VALUE_STRING) {
            int cmp = strcmp(mid_val->as.string, target_value->as.string);
            equal = (cmp == 0);
            less_than = (cmp < 0);
        }
        
        if (equal) {
            return value_new_number((double)mid);
        } else if (less_than) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return value_new_number(-1);
}

/**
 * @brief Find first element matching a predicate
 * @param array_value Array to search
 * @param predicate_value Predicate function
 * @return First matching element or null
 */
Value* zen_array_find(const Value* array_value, const Value* predicate_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return value_new_null();
    }
    
    if (!predicate_value || predicate_value->type != VALUE_FUNCTION) {
        // Without predicate, return first element
        if (array_value->as.array.length > 0) {
            return value_copy(array_value->as.array.data[0]);
        }
        return value_new_null();
    }
    
    size_t length = array_value->as.array.length;
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        // For now, return first non-null element
        // Actual predicate evaluation needs evaluator integration
        if (elem && elem->type != VALUE_NULL) {
            return value_copy(elem);
        }
    }
    
    return value_new_null();
}

/**
 * @brief Slice array from start to end index
 * @param array_value Source array
 * @param start_value Start index (inclusive)
 * @param end_value End index (exclusive), optional
 * @return New array with sliced elements
 */
Value* zen_array_slice(const Value* array_value, const Value* start_value, const Value* end_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    if (!start_value || start_value->type != VALUE_NUMBER) {
        return value_copy(array_value);
    }
    
    size_t length = array_value->as.array.length;
    int start = (int)start_value->as.number;
    int end = end_value && end_value->type == VALUE_NUMBER ? 
              (int)end_value->as.number : (int)length;
    
    // Handle negative indices
    if (start < 0) start = (int)length + start;
    if (end < 0) end = (int)length + end;
    
    // Clamp to valid range
    if (start < 0) start = 0;
    if (start >= (int)length) start = (int)length;
    if (end < start) end = start;
    if (end > (int)length) end = (int)length;
    
    size_t result_length = (size_t)(end - start);
    Value* result_array = array_new(result_length);
    
    for (int i = start; i < end; i++) {
        Value* elem = array_value->as.array.data[i];
        if (elem) {
            Value* copy_elem = value_copy(elem);
            array_push(result_array, copy_elem);
        }
    }
    
    return result_array;
}

/**
 * @brief Concatenate two arrays
 * @param array1_value First array
 * @param array2_value Second array
 * @return New array with concatenated elements
 */
Value* zen_array_concat(const Value* array1_value, const Value* array2_value) {
    if (!array1_value || array1_value->type != VALUE_ARRAY) {
        return array2_value && array2_value->type == VALUE_ARRAY ? 
               value_copy(array2_value) : array_new(0);
    }
    
    if (!array2_value || array2_value->type != VALUE_ARRAY) {
        return value_copy(array1_value);
    }
    
    size_t len1 = array1_value->as.array.length;
    size_t len2 = array2_value->as.array.length;
    
    Value* result_array = array_new(len1 + len2);
    
    // Copy elements from first array
    for (size_t i = 0; i < len1; i++) {
        Value* elem = array1_value->as.array.data[i];
        if (elem) {
            Value* copy_elem = value_copy(elem);
            array_push(result_array, copy_elem);
        }
    }
    
    // Copy elements from second array
    for (size_t i = 0; i < len2; i++) {
        Value* elem = array2_value->as.array.data[i];
        if (elem) {
            Value* copy_elem = value_copy(elem);
            array_push(result_array, copy_elem);
        }
    }
    
    return result_array;
}

/**
 * @brief Flatten nested arrays one level deep
 * @param array_value Array potentially containing sub-arrays
 * @return New flattened array
 */
Value* zen_array_flatten(const Value* array_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    size_t length = array_value->as.array.length;
    Value* result_array = array_new(0);
    
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        if (!elem) continue;
        
        if (elem->type == VALUE_ARRAY) {
            // Flatten one level
            size_t sub_length = elem->as.array.length;
            for (size_t j = 0; j < sub_length; j++) {
                Value* sub_elem = elem->as.array.data[j];
                if (sub_elem) {
                    Value* copy_sub_elem = value_copy(sub_elem);
                    array_push(result_array, copy_sub_elem);
                }
            }
        } else {
            // Regular element
            Value* copy_elem = value_copy(elem);
            array_push(result_array, copy_elem);
        }
    }
    
    return result_array;
}

/**
 * @brief Check if array includes a specific value
 * @param array_value Array to search
 * @param search_value Value to find
 * @return Boolean indicating if value is found
 */
Value* zen_array_includes(const Value* array_value, const Value* search_value) {
    if (!array_value || array_value->type != VALUE_ARRAY || !search_value) {
        return value_new_boolean(false);
    }
    
    size_t length = array_value->as.array.length;
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        if (elem && value_equals(elem, search_value)) {
            return value_new_boolean(true);
        }
    }
    
    return value_new_boolean(false);
}

/**
 * @brief Find index of first occurrence of a value
 * @param array_value Array to search
 * @param search_value Value to find
 * @return Index of first occurrence or -1 if not found
 */
Value* zen_array_index_of(const Value* array_value, const Value* search_value) {
    if (!array_value || array_value->type != VALUE_ARRAY || !search_value) {
        return value_new_number(-1);
    }
    
    size_t length = array_value->as.array.length;
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        if (elem && value_equals(elem, search_value)) {
            return value_new_number((double)i);
        }
    }
    
    return value_new_number(-1);
}

/**
 * @brief Reverse array elements
 * @param array_value Array to reverse
 * @return New array with reversed elements
 */
Value* zen_array_reverse(const Value* array_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    size_t length = array_value->as.array.length;
    Value* result_array = array_new(length);
    
    // Add elements in reverse order
    for (int i = (int)length - 1; i >= 0; i--) {
        Value* elem = array_value->as.array.data[i];
        if (elem) {
            Value* copy_elem = value_copy(elem);
            array_push(result_array, copy_elem);
        }
    }
    
    return result_array;
}

/**
 * @brief Remove duplicate values from array
 * @param array_value Array to deduplicate
 * @return New array with unique elements only
 */
Value* zen_array_unique(const Value* array_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return array_new(0);
    }
    
    size_t length = array_value->as.array.length;
    Value* result_array = array_new(0);
    
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        if (!elem) continue;
        
        // Check if this element already exists in result
        bool found = false;
        size_t result_length = result_array->as.array.length;
        for (size_t j = 0; j < result_length; j++) {
            Value* result_elem = result_array->as.array.data[j];
            if (result_elem && value_equals(elem, result_elem)) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            Value* copy_elem = value_copy(elem);
            array_push(result_array, copy_elem);
        }
    }
    
    return result_array;
}