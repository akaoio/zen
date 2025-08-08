#ifndef ZEN_STDLIB_MATH_H
#define ZEN_STDLIB_MATH_H

#include "zen/types/value.h"

// Internal math functions (not exposed in public API)
/**
 * @brief Internal helper for absolute value
 * @param num_value Value to get absolute value of
 * @return Absolute value result
 */
Value *math_abs_internal(const Value *num_value);

/**
 * @brief Internal helper for floor function
 * @param num_value Value to floor
 * @return Floor result
 */
Value *math_floor_internal(const Value *num_value);

/**
 * @brief Internal helper for ceiling function
 * @param num_value Value to ceil
 * @return Ceiling result
 */
Value *math_ceil_internal(const Value *num_value);

/**
 * @brief Internal helper for rounding
 * @param num_value Value to round
 * @return Rounded result
 */
Value *math_round_internal(const Value *num_value);

/**
 * @brief Internal helper for square root
 * @param num_value Value to get square root of
 * @return Square root result
 */
Value *math_sqrt_internal(const Value *num_value);

/**
 * @brief Internal helper for power function
 * @param base_value Base value
 * @param exp_value Exponent value
 * @return Power result
 */
Value *math_pow_internal(const Value *base_value, const Value *exp_value);

/**
 * @brief Internal helper for sine function
 * @param num_value Value to get sine of
 * @return Sine result
 */
Value *math_sin_internal(const Value *num_value);

/**
 * @brief Internal helper for cosine function
 * @param num_value Value to get cosine of
 * @return Cosine result
 */
Value *math_cos_internal(const Value *num_value);

/**
 * @brief Internal helper for tangent function
 * @param num_value Value to get tangent of
 * @return Tangent result
 */
Value *math_tan_internal(const Value *num_value);

/**
 * @brief Internal helper for natural logarithm
 * @param num_value Value to get logarithm of
 * @return Logarithm result
 */
Value *math_log_internal(const Value *num_value);

/**
 * @brief Internal helper for random number generation
 * @return Random number between 0 and 1
 */
Value *math_random_internal(void);

/**
 * @brief Internal helper for random integer generation
 * @param min_value Minimum value
 * @param max_value Maximum value
 * @return Random integer in range
 */
Value *math_random_int_internal(const Value *min_value, const Value *max_value);

/**
 * @brief Internal helper for minimum value
 * @param a_value First value
 * @param b_value Second value
 * @return Minimum value
 */
Value *math_min_internal(const Value *a_value, const Value *b_value);

/**
 * @brief Internal helper for maximum value
 * @param a_value First value
 * @param b_value Second value
 * @return Maximum value
 */
Value *math_max_internal(const Value *a_value, const Value *b_value);

/**
 * @brief Internal helper to check if value is NaN
 * @param num_value Value to check
 * @return Boolean result
 */
Value *math_is_nan_internal(const Value *num_value);

/**
 * @brief Internal helper to check if value is infinite
 * @param num_value Value to check
 * @return Boolean result
 */
Value *math_is_infinite_internal(const Value *num_value);

// Stdlib wrapper functions (MANIFEST.json signatures)
/**
 * @brief Absolute value function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Absolute value result
 */
Value *math_abs(Value **args, size_t argc);

/**
 * @brief Floor function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Floor result
 */
Value *math_floor(Value **args, size_t argc);

/**
 * @brief Ceiling function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Ceiling result
 */
Value *math_ceil(Value **args, size_t argc);

/**
 * @brief Round function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Rounded result
 */
Value *math_round(Value **args, size_t argc);

/**
 * @brief Square root function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Square root result
 */
Value *math_sqrt(Value **args, size_t argc);

/**
 * @brief Power function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Power result
 */
Value *math_pow(Value **args, size_t argc);

/**
 * @brief Sine function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Sine result
 */
Value *math_sin(Value **args, size_t argc);

/**
 * @brief Cosine function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Cosine result
 */
Value *math_cos(Value **args, size_t argc);

/**
 * @brief Tangent function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Tangent result
 */
Value *math_tan(Value **args, size_t argc);

/**
 * @brief Natural logarithm function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Logarithm result
 */
Value *math_log(Value **args, size_t argc);

/**
 * @brief Random number function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Random number between 0 and 1
 */
Value *math_random(Value **args, size_t argc);

/**
 * @brief Random integer function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Random integer in range
 */
Value *math_random_int(Value **args, size_t argc);

/**
 * @brief Minimum value function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Minimum value
 */
Value *math_min(Value **args, size_t argc);

/**
 * @brief Maximum value function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Maximum value
 */
Value *math_max(Value **args, size_t argc);

/**
 * @brief Check if value is NaN - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Boolean result
 */
Value *math_is_nan(Value **args, size_t argc);

/**
 * @brief Check if value is infinite - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Boolean result
 */
Value *math_is_infinite(Value **args, size_t argc);

#endif  // ZEN_STDLIB_MATH_H