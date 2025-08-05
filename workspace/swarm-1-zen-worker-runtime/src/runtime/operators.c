/*
 * operators.c
 * Operator implementations
 * 
 * This file implements all operators for the ZEN language.
 * ZEN uses dynamic typing with type coercion following JavaScript-like semantics.
 */

#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Helper functions for type coercion */

/**
 * @brief Convert value to number for arithmetic operations
 * @param value Value to convert
 * @return double Numeric representation
 */
static double value_to_number(const Value* value) {
    if (!value) return 0.0;
    
    switch (value->type) {
        case VALUE_NULL:
            return 0.0;
        case VALUE_BOOLEAN:
            return value->data.boolean ? 1.0 : 0.0;
        case VALUE_NUMBER:
            return value->data.number;
        case VALUE_STRING:
            if (!value->data.string || strlen(value->data.string) == 0) {
                return 0.0;
            }
            // Try to parse as number
            char* endptr;
            double result = strtod(value->data.string, &endptr);
            // If entire string was consumed, it's a valid number
            if (*endptr == '\0') {
                return result;
            }
            // Otherwise it's NaN
            return NAN;
        case VALUE_ARRAY:
            // Empty array is 0, array with one element converts that element
            // TODO: Implement when arrays are available
            return NAN;
        case VALUE_OBJECT:
            return NAN;
        case VALUE_FUNCTION:
            return NAN;
        case VALUE_ERROR:
            return NAN;
    }
    return NAN;
}

/**
 * @brief Convert value to boolean for logical operations
 * @param value Value to convert
 * @return bool Boolean representation
 */
static bool value_to_boolean(const Value* value) {
    if (!value) return false;
    
    switch (value->type) {
        case VALUE_NULL:
            return false;
        case VALUE_BOOLEAN:
            return value->data.boolean;
        case VALUE_NUMBER:
            // 0, NaN, -0 are false, everything else is true
            return value->data.number != 0.0 && !isnan(value->data.number);
        case VALUE_STRING:
            // Empty string is false, non-empty is true
            return value->data.string && strlen(value->data.string) > 0;
        case VALUE_ARRAY:
            // Arrays are always truthy (even empty ones in ZEN)
            return true;
        case VALUE_OBJECT:
            // Objects are always truthy
            return true;
        case VALUE_FUNCTION:
            // Functions are always truthy
            return true;
        case VALUE_ERROR:
            // Errors are truthy
            return true;
    }
    return false;
}

/* Arithmetic operators */

/**
 * @brief Addition operator
 * @param a First operand
 * @param b Second operand
 * @return Value* Result of a + b
 */
Value* op_add(Value* a, Value* b) {
    if (!a || !b) {
        return value_new_null();
    }
    
    // String concatenation has priority
    if (a->type == VALUE_STRING || b->type == VALUE_STRING) {
        char* a_str = value_to_string(a);
        char* b_str = value_to_string(b);
        
        if (!a_str || !b_str) {
            free(a_str);
            free(b_str);
            return value_new_null();
        }
        
        size_t len = strlen(a_str) + strlen(b_str) + 1;
        char* result_str = malloc(len);
        if (!result_str) {
            free(a_str);
            free(b_str);
            return value_new_null();
        }
        
        strcpy(result_str, a_str);
        strcat(result_str, b_str);
        
        Value* result = value_new_string(result_str);
        
        free(a_str);
        free(b_str);
        free(result_str);
        
        return result;
    }
    
    // Numeric addition
    double a_num = value_to_number(a);
    double b_num = value_to_number(b);
    
    return value_new_number(a_num + b_num);
}

/**
 * @brief Subtraction operator
 * @param a First operand
 * @param b Second operand
 * @return Value* Result of a - b
 */
Value* op_subtract(Value* a, Value* b) {
    if (!a || !b) {
        return value_new_null();
    }
    
    double a_num = value_to_number(a);
    double b_num = value_to_number(b);
    
    return value_new_number(a_num - b_num);
}

/**
 * @brief Multiplication operator
 * @param a First operand
 * @param b Second operand
 * @return Value* Result of a * b
 */
Value* op_multiply(Value* a, Value* b) {
    if (!a || !b) {
        return value_new_null();
    }
    
    double a_num = value_to_number(a);
    double b_num = value_to_number(b);
    
    return value_new_number(a_num * b_num);
}

/**
 * @brief Division operator
 * @param a First operand (dividend)
 * @param b Second operand (divisor) 
 * @return Value* Result of a / b
 */
Value* op_divide(Value* a, Value* b) {
    if (!a || !b) {
        return value_new_null();
    }
    
    double a_num = value_to_number(a);
    double b_num = value_to_number(b);
    
    // Division by zero returns Infinity or -Infinity
    return value_new_number(a_num / b_num);
}

/**
 * @brief Modulo operator
 * @param a First operand
 * @param b Second operand
 * @return Value* Result of a % b
 */
Value* op_modulo(Value* a, Value* b) {
    if (!a || !b) {
        return value_new_null();
    }
    
    double a_num = value_to_number(a);
    double b_num = value_to_number(b);
    
    // Use fmod for floating point modulo
    return value_new_number(fmod(a_num, b_num));
}

/* Comparison operators */

/**
 * @brief Equality comparison
 * @param a First operand
 * @param b Second operand
 * @return Value* Boolean result of a = b (note: ZEN uses = for equality)
 */
Value* op_equals(Value* a, Value* b) {
    return value_new_boolean(value_equals(a, b));
}

/**
 * @brief Inequality comparison
 * @param a First operand
 * @param b Second operand
 * @return Value* Boolean result of a != b
 */
Value* op_not_equals(Value* a, Value* b) {
    return value_new_boolean(!value_equals(a, b));
}

/**
 * @brief Less than comparison
 * @param a First operand
 * @param b Second operand
 * @return Value* Boolean result of a < b
 */
Value* op_less_than(Value* a, Value* b) {
    if (!a || !b) {
        return value_new_boolean(false);
    }
    
    // For strings, use lexicographic comparison
    if (a->type == VALUE_STRING && b->type == VALUE_STRING) {
        if (!a->data.string || !b->data.string) {
            return value_new_boolean(false);
        }
        return value_new_boolean(strcmp(a->data.string, b->data.string) < 0);
    }
    
    // For numbers, convert both to numbers
    double a_num = value_to_number(a);
    double b_num = value_to_number(b);
    
    // Handle NaN: any comparison with NaN is false
    if (isnan(a_num) || isnan(b_num)) {
        return value_new_boolean(false);
    }
    
    return value_new_boolean(a_num < b_num);
}

/**
 * @brief Greater than comparison
 * @param a First operand
 * @param b Second operand
 * @return Value* Boolean result of a > b
 */
Value* op_greater_than(Value* a, Value* b) {
    if (!a || !b) {
        return value_new_boolean(false);
    }
    
    // For strings, use lexicographic comparison
    if (a->type == VALUE_STRING && b->type == VALUE_STRING) {
        if (!a->data.string || !b->data.string) {
            return value_new_boolean(false);
        }
        return value_new_boolean(strcmp(a->data.string, b->data.string) > 0);
    }
    
    // For numbers, convert both to numbers
    double a_num = value_to_number(a);
    double b_num = value_to_number(b);
    
    // Handle NaN: any comparison with NaN is false
    if (isnan(a_num) || isnan(b_num)) {
        return value_new_boolean(false);
    }
    
    return value_new_boolean(a_num > b_num);
}

/* Logical operators */

/**
 * @brief Logical AND
 * @param a First operand
 * @param b Second operand
 * @return Value* Result of a & b (ZEN uses & for logical AND)
 */
Value* op_logical_and(Value* a, Value* b) {
    if (!a) {
        return value_new_boolean(false);
    }
    
    // Short-circuit: if a is falsy, return a
    if (!value_to_boolean(a)) {
        return value_ref(a);
    }
    
    // Otherwise return b
    return b ? value_ref(b) : value_new_null();
}

/**
 * @brief Logical OR
 * @param a First operand
 * @param b Second operand
 * @return Value* Result of a | b (ZEN uses | for logical OR)
 */
Value* op_logical_or(Value* a, Value* b) {
    if (!a) {
        return b ? value_ref(b) : value_new_null();
    }
    
    // Short-circuit: if a is truthy, return a
    if (value_to_boolean(a)) {
        return value_ref(a);
    }
    
    // Otherwise return b
    return b ? value_ref(b) : value_new_null();
}

/**
 * @brief Logical NOT
 * @param a Operand
 * @return Value* Boolean result of !a
 */
Value* op_logical_not(Value* a) {
    return value_new_boolean(!value_to_boolean(a));
}