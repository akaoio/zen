/*
 * operators.c
 * Operator implementations
 * 
 * This file implements all ZEN operators with proper type coercion and error handling
 * DO NOT modify function signatures without updating the manifest
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>
#include "../include/zen/types/value.h"

/* Forward declarations for helper functions */
static Value* create_error(const char* message);
static bool to_number(const Value* value, double* result);
static char* concat_strings(const char* a, const char* b);
static bool is_truthy(const Value* value);
static int compare_values(const Value* a, const Value* b);

/**
 * @brief Addition operator
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of addition operation or error value
 */
Value* op_add(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in addition");
    }
    
    /* String concatenation has highest priority */
    if (a->type == VALUE_STRING || b->type == VALUE_STRING) {
        char* a_str = value_to_string(a);
        char* b_str = value_to_string(b);
        
        if (!a_str || !b_str) {
            free(a_str);
            free(b_str);
            return create_error("Failed to convert operand to string");
        }
        
        char* result = concat_strings(a_str, b_str);
        free(a_str);
        free(b_str);
        
        if (!result) {
            return create_error("String concatenation failed");
        }
        
        Value* str_val = value_new_string(result);
        free(result);
        return str_val;
    }
    
    /* Numeric addition */
    double a_num, b_num;
    if (!to_number(a, &a_num) || !to_number(b, &b_num)) {
        return create_error("Cannot convert operands to numbers for addition");
    }
    
    return value_new_number(a_num + b_num);
}

/**
 * @brief Subtraction operator
 * @param a Left operand value
 * @param b Right operand value  
 * @return Result of subtraction operation or error value
 */
Value* op_subtract(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in subtraction");
    }
    
    double a_num, b_num;
    if (!to_number(a, &a_num) || !to_number(b, &b_num)) {
        return create_error("Cannot convert operands to numbers for subtraction");
    }
    
    return value_new_number(a_num - b_num);
}

/**
 * @brief Multiplication operator
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of multiplication operation or error value
 */
Value* op_multiply(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in multiplication");
    }
    
    double a_num, b_num;
    if (!to_number(a, &a_num) || !to_number(b, &b_num)) {
        return create_error("Cannot convert operands to numbers for multiplication");
    }
    
    return value_new_number(a_num * b_num);
}

/**
 * @brief Division operator
 * @param a Left operand value (dividend)
 * @param b Right operand value (divisor)
 * @return Result of division operation or error value
 */
Value* op_divide(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in division");
    }
    
    double a_num, b_num;
    if (!to_number(a, &a_num) || !to_number(b, &b_num)) {
        return create_error("Cannot convert operands to numbers for division");
    }
    
    if (b_num == 0.0) {
        return create_error("Division by zero");
    }
    
    return value_new_number(a_num / b_num);
}

/**
 * @brief Modulo operator
 * @param a Left operand value (dividend)
 * @param b Right operand value (divisor)
 * @return Result of modulo operation or error value
 */
Value* op_modulo(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in modulo");
    }
    
    double a_num, b_num;
    if (!to_number(a, &a_num) || !to_number(b, &b_num)) {
        return create_error("Cannot convert operands to numbers for modulo");
    }
    
    if (b_num == 0.0) {
        return create_error("Modulo by zero");
    }
    
    return value_new_number(fmod(a_num, b_num));
}

/**
 * @brief Equality comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating equality or error value
 */
Value* op_equals(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in equality comparison");
    }
    
    return value_new_boolean(value_equals(a, b));
}

/**
 * @brief Inequality comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating inequality or error value
 */
Value* op_not_equals(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in inequality comparison");
    }
    
    return value_new_boolean(!value_equals(a, b));
}

/**
 * @brief Less than comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating if a < b or error value
 */
Value* op_less_than(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in less than comparison");
    }
    
    int cmp = compare_values(a, b);
    if (cmp == INT_MIN) {
        return create_error("Cannot compare values of incompatible types");
    }
    
    return value_new_boolean(cmp < 0);
}

/**
 * @brief Greater than comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating if a > b or error value
 */
Value* op_greater_than(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in greater than comparison");
    }
    
    int cmp = compare_values(a, b);
    if (cmp == INT_MIN) {
        return create_error("Cannot compare values of incompatible types");
    }
    
    return value_new_boolean(cmp > 0);
}

/**
 * @brief Logical AND
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical AND operation or error value
 */
Value* op_logical_and(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in logical AND");
    }
    
    /* Short-circuit evaluation: if a is falsy, return a */
    if (!is_truthy(a)) {
        return value_ref(a);
    }
    
    /* Otherwise return b */
    return value_ref(b);
}

/**
 * @brief Logical OR
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical OR operation or error value
 */
Value* op_logical_or(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in logical OR");
    }
    
    /* Short-circuit evaluation: if a is truthy, return a */
    if (is_truthy(a)) {
        return value_ref(a);
    }
    
    /* Otherwise return b */
    return value_ref(b);
}

/**
 * @brief Logical NOT
 * @param a Operand value to negate
 * @return Boolean value indicating logical negation or error value
 */
Value* op_logical_not(Value* a) {
    if (!a) {
        return create_error("Null operand in logical NOT");
    }
    
    return value_new_boolean(!is_truthy(a));
}

/* Helper function implementations */

/**
 * @brief Create an error value with a message
 * @param message Error message
 * @return Error value
 */
static Value* create_error(const char* message) {
    Value* error = value_new(VALUE_ERROR);
    if (error && message) {
        error->as.error = malloc(sizeof(ZenError));
        if (error->as.error) {
            error->as.error->message = strdup(message);
            error->as.error->code = -1;
        }
    }
    return error;
}

/**
 * @brief Convert a value to a number
 * @param value Value to convert
 * @param result Pointer to store the result
 * @return true if conversion successful, false otherwise
 */
static bool to_number(const Value* value, double* result) {
    if (!value || !result) {
        return false;
    }
    
    switch (value->type) {
        case VALUE_NUMBER:
            *result = value->as.number;
            return true;
            
        case VALUE_BOOLEAN:
            *result = value->as.boolean ? 1.0 : 0.0;
            return true;
            
        case VALUE_NULL:
            *result = 0.0;
            return true;
            
        case VALUE_STRING:
            if (value->as.string && value->as.string->data) {
                char* endptr;
                *result = strtod(value->as.string->data, &endptr);
                return *endptr == '\0' && endptr != value->as.string->data;
            }
            return false;
            
        default:
            return false;
    }
}

/**
 * @brief Concatenate two strings
 * @param a First string
 * @param b Second string
 * @return Concatenated string (caller must free)
 */
static char* concat_strings(const char* a, const char* b) {
    if (!a || !b) {
        return NULL;
    }
    
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    size_t total_len = len_a + len_b + 1;
    
    char* result = malloc(total_len);
    if (!result) {
        return NULL;
    }
    
    strcpy(result, a);
    strcat(result, b);
    
    return result;
}

/**
 * @brief Check if a value is truthy
 * @param value Value to check
 * @return true if truthy, false if falsy
 */
static bool is_truthy(const Value* value) {
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
            return value->as.string && value->as.string->data && strlen(value->as.string->data) > 0;
            
        case VALUE_ARRAY:
            return value->as.array && value->as.array->length > 0;
            
        case VALUE_OBJECT:
            return value->as.object && value->as.object->length > 0;
            
        case VALUE_ERROR:
            return false;
            
        default:
            return true;
    }
}

/**
 * @brief Compare two values for ordering
 * @param a First value
 * @param b Second value
 * @return -1 if a < b, 0 if a == b, 1 if a > b, INT_MIN on error
 */
static int compare_values(const Value* a, const Value* b) {
    if (!a || !b) {
        return INT_MIN;
    }
    
    /* Same type comparisons */
    if (a->type == b->type) {
        switch (a->type) {
            case VALUE_NULL:
                return 0;
                
            case VALUE_BOOLEAN:
                if (a->as.boolean == b->as.boolean) return 0;
                return a->as.boolean ? 1 : -1;
                
            case VALUE_NUMBER: {
                double diff = a->as.number - b->as.number;
                if (diff < 0) return -1;
                if (diff > 0) return 1;
                return 0;
            }
            
            case VALUE_STRING:
                if (!a->as.string || !a->as.string->data || !b->as.string || !b->as.string->data) {
                    return INT_MIN;
                }
                return strcmp(a->as.string->data, b->as.string->data);
                
            default:
                return INT_MIN; /* Cannot compare arrays, objects, etc. */
        }
    }
    
    /* Different type comparisons - try to convert to numbers */
    double a_num, b_num;
    if (to_number(a, &a_num) && to_number(b, &b_num)) {
        double diff = a_num - b_num;
        if (diff < 0) return -1;
        if (diff > 0) return 1;
        return 0;
    }
    
    /* Cannot compare */
    return INT_MIN;
}