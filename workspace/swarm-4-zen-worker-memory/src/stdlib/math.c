/*
 * math.c
 * Mathematical functions for ZEN stdlib
 * 
 * These are internal stdlib functions that extend the core functionality
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/math.h"
#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

// Initialize random seed once
static bool random_initialized = false;

/**
 * @brief Initialize random number generator
 */
static void init_random() {
    if (!random_initialized) {
        srand((unsigned int)time(NULL));
        random_initialized = true;
    }
}

/**
 * @brief Absolute value function
 * @param num_value Number value to get absolute value of
 * @return Absolute value as new number value
 */
Value* zen_math_abs(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(fabs(num_value->as.number));
}

/**
 * @brief Floor function
 * @param num_value Number value to floor
 * @return Floor value as new number value
 */
Value* zen_math_floor(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(floor(num_value->as.number));
}

/**
 * @brief Ceiling function
 * @param num_value Number value to ceiling
 * @return Ceiling value as new number value
 */
Value* zen_math_ceil(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(ceil(num_value->as.number));
}

/**
 * @brief Round function
 * @param num_value Number value to round
 * @return Rounded value as new number value
 */
Value* zen_math_round(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(round(num_value->as.number));
}

/**
 * @brief Square root function
 * @param num_value Number value to get square root of
 * @return Square root as new number value, or error for negative numbers
 */
Value* zen_math_sqrt(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double val = num_value->as.number;
    if (val < 0) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Cannot take square root of negative number");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return value_new_number(sqrt(val));
}

/**
 * @brief Power function
 * @param base_value Base number value
 * @param exp_value Exponent number value
 * @return Result of base^exponent as new number value
 */
Value* zen_math_pow(const Value* base_value, const Value* exp_value) {
    if (!base_value || base_value->type != VALUE_NUMBER ||
        !exp_value || exp_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double base = base_value->as.number;
    double exp = exp_value->as.number;
    
    return value_new_number(pow(base, exp));
}

/**
 * @brief Sine function
 * @param num_value Number value (in radians)
 * @return Sine value as new number value
 */
Value* zen_math_sin(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(sin(num_value->as.number));
}

/**
 * @brief Cosine function
 * @param num_value Number value (in radians)
 * @return Cosine value as new number value
 */
Value* zen_math_cos(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(1.0);
    }
    
    return value_new_number(cos(num_value->as.number));
}

/**
 * @brief Tangent function
 * @param num_value Number value (in radians)
 * @return Tangent value as new number value
 */
Value* zen_math_tan(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(tan(num_value->as.number));
}

/**
 * @brief Natural logarithm function
 * @param num_value Number value
 * @return Natural log as new number value, or error for non-positive numbers
 */
Value* zen_math_log(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double val = num_value->as.number;
    if (val <= 0) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Cannot take logarithm of non-positive number");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return value_new_number(log(val));
}

/**
 * @brief Generate random number between 0 and 1
 * @return Random number as new number value
 */
Value* zen_math_random() {
    init_random();
    double random_val = (double)rand() / RAND_MAX;
    return value_new_number(random_val);
}

/**
 * @brief Generate random integer between min and max (inclusive)
 * @param min_value Minimum value
 * @param max_value Maximum value
 * @return Random integer as new number value
 */
Value* zen_math_random_int(const Value* min_value, const Value* max_value) {
    if (!min_value || min_value->type != VALUE_NUMBER ||
        !max_value || max_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    init_random();
    
    int min = (int)min_value->as.number;
    int max = (int)max_value->as.number;
    
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    
    int range = max - min + 1;
    int random_int = min + (rand() % range);
    
    return value_new_number((double)random_int);
}

/**
 * @brief Find minimum of two numbers
 * @param a_value First number value
 * @param b_value Second number value
 * @return Minimum value as new number value
 */
Value* zen_math_min(const Value* a_value, const Value* b_value) {
    if (!a_value || a_value->type != VALUE_NUMBER) {
        return value_copy(b_value);
    }
    if (!b_value || b_value->type != VALUE_NUMBER) {
        return value_copy(a_value);
    }
    
    double a = a_value->as.number;
    double b = b_value->as.number;
    
    return value_new_number(a < b ? a : b);
}

/**
 * @brief Find maximum of two numbers
 * @param a_value First number value
 * @param b_value Second number value
 * @return Maximum value as new number value
 */
Value* zen_math_max(const Value* a_value, const Value* b_value) {
    if (!a_value || a_value->type != VALUE_NUMBER) {
        return value_copy(b_value);
    }
    if (!b_value || b_value->type != VALUE_NUMBER) {
        return value_copy(a_value);
    }
    
    double a = a_value->as.number;
    double b = b_value->as.number;
    
    return value_new_number(a > b ? a : b);
}

/**
 * @brief Check if number is NaN
 * @param num_value Number value to check
 * @return Boolean value indicating if number is NaN
 */
Value* zen_math_is_nan(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_boolean(false);
    }
    
    return value_new_boolean(isnan(num_value->as.number));
}

/**
 * @brief Check if number is infinite
 * @param num_value Number value to check
 * @return Boolean value indicating if number is infinite
 */
Value* zen_math_is_infinite(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_boolean(false);
    }
    
    return value_new_boolean(isinf(num_value->as.number));
}