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

// ============================================================================
// ADVANCED MATHEMATICAL FUNCTIONS
// ============================================================================

/**
 * @brief Arcsine function
 * @param num_value Number value (between -1 and 1)
 * @return Arcsine value in radians as new number value
 */
Value* zen_math_asin(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double val = num_value->as.number;
    if (val < -1.0 || val > 1.0) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("asin domain error: input must be between -1 and 1");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return value_new_number(asin(val));
}

/**
 * @brief Arccosine function
 * @param num_value Number value (between -1 and 1)
 * @return Arccosine value in radians as new number value
 */
Value* zen_math_acos(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(M_PI_2);
    }
    
    double val = num_value->as.number;
    if (val < -1.0 || val > 1.0) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("acos domain error: input must be between -1 and 1");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return value_new_number(acos(val));
}

/**
 * @brief Arctangent function
 * @param num_value Number value
 * @return Arctangent value in radians as new number value
 */
Value* zen_math_atan(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(atan(num_value->as.number));
}

/**
 * @brief Two-argument arctangent function
 * @param y_value Y coordinate value
 * @param x_value X coordinate value
 * @return Arctangent of y/x in radians as new number value
 */
Value* zen_math_atan2(const Value* y_value, const Value* x_value) {
    if (!y_value || y_value->type != VALUE_NUMBER ||
        !x_value || x_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double y = y_value->as.number;
    double x = x_value->as.number;
    
    return value_new_number(atan2(y, x));
}

/**
 * @brief Base-10 logarithm function
 * @param num_value Number value
 * @return Base-10 log as new number value, or error for non-positive numbers
 */
Value* zen_math_log10(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double val = num_value->as.number;
    if (val <= 0) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Cannot take log10 of non-positive number");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return value_new_number(log10(val));
}

/**
 * @brief Base-2 logarithm function
 * @param num_value Number value
 * @return Base-2 log as new number value, or error for non-positive numbers
 */
Value* zen_math_log2(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double val = num_value->as.number;
    if (val <= 0) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Cannot take log2 of non-positive number");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return value_new_number(log2(val));
}

/**
 * @brief Exponential function (e^x)
 * @param num_value Number value (exponent)
 * @return e^x as new number value
 */
Value* zen_math_exp(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(1.0);
    }
    
    return value_new_number(exp(num_value->as.number));
}

/**
 * @brief Hyperbolic sine function
 * @param num_value Number value
 * @return Hyperbolic sine as new number value
 */
Value* zen_math_sinh(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(sinh(num_value->as.number));
}

/**
 * @brief Hyperbolic cosine function
 * @param num_value Number value
 * @return Hyperbolic cosine as new number value
 */
Value* zen_math_cosh(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(1.0);
    }
    
    return value_new_number(cosh(num_value->as.number));
}

/**
 * @brief Hyperbolic tangent function
 * @param num_value Number value
 * @return Hyperbolic tangent as new number value
 */
Value* zen_math_tanh(const Value* num_value) {
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    return value_new_number(tanh(num_value->as.number));
}

/**
 * @brief Convert degrees to radians
 * @param degrees_value Number value in degrees
 * @return Value in radians as new number value
 */
Value* zen_math_radians(const Value* degrees_value) {
    if (!degrees_value || degrees_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double degrees = degrees_value->as.number;
    return value_new_number(degrees * M_PI / 180.0);
}

/**
 * @brief Convert radians to degrees
 * @param radians_value Number value in radians
 * @return Value in degrees as new number value
 */
Value* zen_math_degrees(const Value* radians_value) {
    if (!radians_value || radians_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double radians = radians_value->as.number;
    return value_new_number(radians * 180.0 / M_PI);
}

/**
 * @brief Generate random number with seed
 * @param seed_value Seed value for random number generator
 * @return Random number as new number value
 */
Value* zen_math_random_seed(const Value* seed_value) {
    if (!seed_value || seed_value->type != VALUE_NUMBER) {
        return zen_math_random();
    }
    
    unsigned int seed = (unsigned int)seed_value->as.number;
    srand(seed);
    random_initialized = true;
    
    double random_val = (double)rand() / RAND_MAX;
    return value_new_number(random_val);
}

// ============================================================================
// STATISTICAL FUNCTIONS 
// ============================================================================

/**
 * @brief Statistical mean function for arrays
 * @param array_value Array of numbers
 * @return Mean value as new number value
 */
Value* zen_math_mean(const Value* array_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return value_new_number(0.0);
    }
    
    size_t length = array_value->as.array.length;
    if (length == 0) {
        return value_new_number(0.0);
    }
    
    double sum = 0.0;
    size_t count = 0;
    
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        if (elem && elem->type == VALUE_NUMBER) {
            sum += elem->as.number;
            count++;
        }
    }
    
    if (count == 0) {
        return value_new_number(0.0);
    }
    
    return value_new_number(sum / count);
}

/**
 * @brief Statistical median function for arrays
 * @param array_value Array of numbers
 * @return Median value as new number value
 */
Value* zen_math_median(const Value* array_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return value_new_number(0.0);
    }
    
    size_t length = array_value->as.array.length;
    if (length == 0) {
        return value_new_number(0.0);
    }
    
    // Collect numeric values
    double* numbers = malloc(length * sizeof(double));
    if (!numbers) {
        return value_new_number(0.0);
    }
    
    size_t count = 0;
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        if (elem && elem->type == VALUE_NUMBER) {
            numbers[count++] = elem->as.number;
        }
    }
    
    if (count == 0) {
        free(numbers);
        return value_new_number(0.0);
    }
    
    // Sort the numbers (simple bubble sort for now)
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            if (numbers[j] > numbers[j + 1]) {
                double temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }
    
    double median;
    if (count % 2 == 0) {
        median = (numbers[count/2 - 1] + numbers[count/2]) / 2.0;
    } else {
        median = numbers[count/2];
    }
    
    free(numbers);
    return value_new_number(median);
}

/**
 * @brief Statistical standard deviation function for arrays
 * @param array_value Array of numbers
 * @return Standard deviation as new number value
 */
Value* zen_math_stddev(const Value* array_value) {
    if (!array_value || array_value->type != VALUE_ARRAY) {
        return value_new_number(0.0);
    }
    
    size_t length = array_value->as.array.length;
    if (length <= 1) {
        return value_new_number(0.0);
    }
    
    // Calculate mean first
    Value* mean_val = zen_math_mean(array_value);
    double mean = mean_val->as.number;
    value_unref(mean_val);
    
    double sum_squared_diff = 0.0;
    size_t count = 0;
    
    for (size_t i = 0; i < length; i++) {
        Value* elem = array_value->as.array.data[i];
        if (elem && elem->type == VALUE_NUMBER) {
            double diff = elem->as.number - mean;
            sum_squared_diff += diff * diff;
            count++;
        }
    }
    
    if (count <= 1) {
        return value_new_number(0.0);
    }
    
    double variance = sum_squared_diff / (count - 1);
    return value_new_number(sqrt(variance));
}