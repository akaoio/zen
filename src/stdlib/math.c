/*
 * math.c
 * Mathematical functions for ZEN stdlib
 *
 * These are internal stdlib functions that extend the core functionality
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/math.h"

#include "zen/core/error.h"
#include "zen/core/runtime_value.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Initialize random seed once
static bool random_initialized = false;

/**
 * @brief Initialize random number generator
 * @return void (no return value)
 */
static void init_random()
{
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
Value *math_abs_internal(const Value *num_value)
{
    if (!num_value) {
        return error_new("abs() requires a non-null argument");
    }

    if (num_value->type != VALUE_NUMBER) {
        return error_new("abs() requires a numeric argument");
    }

    return value_new_number(fabs(num_value->as.number));
}

/**
 * @brief Floor function
 * @param num_value Number value to floor
 * @return Floor value as new number value
 */
Value *math_floor_internal(const Value *num_value)
{
    if (!num_value) {
        return error_new("floor() requires a non-null argument");
    }

    if (num_value->type != VALUE_NUMBER) {
        return error_new("floor() requires a numeric argument");
    }

    return value_new_number(floor(num_value->as.number));
}

/**
 * @brief Ceiling function
 * @param num_value Number value to ceiling
 * @return Ceiling value as new number value
 */
Value *math_ceil_internal(const Value *num_value)
{
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
Value *math_round_internal(const Value *num_value)
{
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
Value *math_sqrt_internal(const Value *num_value)
{
    if (!num_value) {
        return error_new("sqrt() requires a non-null argument");
    }

    if (num_value->type != VALUE_NUMBER) {
        return error_new("sqrt() requires a numeric argument");
    }

    double val = num_value->as.number;
    if (val < 0) {
        return error_new("sqrt() cannot compute square root of negative number");
    }

    return value_new_number(sqrt(val));
}

/**
 * @brief Power function
 * @param base_value Base value
 * @param exp_value Exponent value
 * @return Result of base^exponent as new number value
 */
Value *math_pow_internal(const Value *base_value, const Value *exp_value)
{
    if (!base_value || base_value->type != VALUE_NUMBER || !exp_value ||
        exp_value->type != VALUE_NUMBER) {
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
Value *math_sin_internal(const Value *num_value)
{
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
Value *math_cos_internal(const Value *num_value)
{
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }

    return value_new_number(cos(num_value->as.number));
}

/**
 * @brief Tangent function
 * @param num_value Number value (in radians)
 * @return Tangent value as new number value
 */
Value *math_tan_internal(const Value *num_value)
{
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
Value *math_log_internal(const Value *num_value)
{
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }

    double val = num_value->as.number;
    if (val <= 0) {
        return value_new_number(0.0);  // Return 0 for non-positive numbers instead of error
    }

    return value_new_number(log(val));
}

/**
 * @brief Generate random number between 0 and 1
 * @return Random number as new number value
 */
Value *math_random_internal(void)
{
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
Value *math_random_int_internal(const Value *min_value, const Value *max_value)
{
    if (!min_value || min_value->type != VALUE_NUMBER || !max_value ||
        max_value->type != VALUE_NUMBER) {
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
Value *math_min_internal(const Value *a_value, const Value *b_value)
{
    if (!a_value || a_value->type != VALUE_NUMBER || !b_value || b_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
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
Value *math_max_internal(const Value *a_value, const Value *b_value)
{
    if (!a_value || a_value->type != VALUE_NUMBER || !b_value || b_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
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
Value *math_is_nan_internal(const Value *num_value)
{
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
Value *math_is_infinite_internal(const Value *num_value)
{
    if (!num_value || num_value->type != VALUE_NUMBER) {
        return value_new_boolean(false);
    }

    return value_new_boolean(isinf(num_value->as.number));
}

// Stdlib wrapper functions (MANIFEST.json stdlib signatures)

/**
 * @brief Absolute value wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Absolute value as new number value
 */
Value *math_abs(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("abs() requires exactly 1 argument");
    }
    return math_abs_internal(args[0]);
}

/**
 * @brief Floor wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Floor value as new number value
 */
Value *math_floor(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("floor() requires exactly 1 argument");
    }
    return math_floor_internal(args[0]);
}

/**
 * @brief Ceiling wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Ceiling value as new number value
 */
Value *math_ceil(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("ceil() requires exactly 1 argument");
    }
    return math_ceil_internal(args[0]);
}

/**
 * @brief Round wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Rounded value as new number value
 */
Value *math_round(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("round() requires exactly 1 argument");
    }
    return math_round_internal(args[0]);
}

/**
 * @brief Square root wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Square root as new number value, or error for negative numbers
 */
Value *math_sqrt(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("sqrt() requires exactly 1 argument");
    }
    return math_sqrt_internal(args[0]);
}

/**
 * @brief Power wrapper for stdlib
 * @param args Arguments array containing base and exponent values
 * @param argc Number of arguments
 * @return Result of base^exponent as new number value
 */
Value *math_pow(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("pow() requires exactly 2 arguments");
    }
    return math_pow_internal(args[0], args[1]);
}

/**
 * @brief Sine wrapper for stdlib
 * @param args Arguments array containing number value (in radians)
 * @param argc Number of arguments
 * @return Sine value as new number value
 */
Value *math_sin(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("sin() requires exactly 1 argument");
    }
    return math_sin_internal(args[0]);
}

/**
 * @brief Cosine wrapper for stdlib
 * @param args Arguments array containing number value (in radians)
 * @param argc Number of arguments
 * @return Cosine value as new number value
 */
Value *math_cos(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("cos() requires exactly 1 argument");
    }
    return math_cos_internal(args[0]);
}

/**
 * @brief Tangent wrapper for stdlib
 * @param args Arguments array containing number value (in radians)
 * @param argc Number of arguments
 * @return Tangent value as new number value
 */
Value *math_tan(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("tan() requires exactly 1 argument");
    }
    return math_tan_internal(args[0]);
}

/**
 * @brief Natural logarithm wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Natural log as new number value, or error for non-positive numbers
 */
Value *math_log(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("log() requires exactly 1 argument");
    }
    return math_log_internal(args[0]);
}

/**
 * @brief Random number wrapper for stdlib
 * @param args Arguments array (should be empty)
 * @param argc Number of arguments (should be 0)
 * @return Random number as new number value
 */
Value *math_random(Value **args, size_t argc)
{
    (void)args;  // Suppress unused parameter warning
    if (argc != 0) {
        return error_new("random() takes no arguments");
    }
    return math_random_internal();
}

/**
 * @brief Random integer wrapper for stdlib
 * @param args Arguments array containing min and max values
 * @param argc Number of arguments
 * @return Random integer as new number value
 */
Value *math_random_int(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("randomInt() requires exactly 2 arguments");
    }
    return math_random_int_internal(args[0], args[1]);
}

/**
 * @brief Minimum wrapper for stdlib
 * @param args Arguments array containing two number values
 * @param argc Number of arguments
 * @return Minimum value as new number value
 */
Value *math_min(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("min() requires exactly 2 arguments");
    }
    return math_min_internal(args[0], args[1]);
}

/**
 * @brief Maximum wrapper for stdlib
 * @param args Arguments array containing two number values
 * @param argc Number of arguments
 * @return Maximum value as new number value
 */
Value *math_max(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("max() requires exactly 2 arguments");
    }
    return math_max_internal(args[0], args[1]);
}

/**
 * @brief NaN check wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Boolean value indicating if number is NaN
 */
Value *math_is_nan(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("isNaN() requires exactly 1 argument");
    }
    return math_is_nan_internal(args[0]);
}

/**
 * @brief Infinite check wrapper for stdlib
 * @param args Arguments array containing number value
 * @param argc Number of arguments
 * @return Boolean value indicating if number is infinite
 */
Value *math_is_infinite(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("isInfinite() requires exactly 1 argument");
    }
    return math_is_infinite_internal(args[0]);
}