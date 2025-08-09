#ifndef ZEN_STDLIB_MATH_H
#define ZEN_STDLIB_MATH_H

#include "zen/core/runtime_value.h"

// Internal math functions (not exposed in public API)
/**
 * @brief Internal helper for absolute value
 * @param num_value Value to get absolute value of
 * @return Absolute value result
 */
RuntimeValue *math_abs_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for floor function
 * @param num_value Value to floor
 * @return Floor result
 */
RuntimeValue *math_floor_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for ceiling function
 * @param num_value Value to ceil
 * @return Ceiling result
 */
RuntimeValue *math_ceil_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for rounding
 * @param num_value Value to round
 * @return Rounded result
 */
RuntimeValue *math_round_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for square root
 * @param num_value Value to get square root of
 * @return Square root result
 */
RuntimeValue *math_sqrt_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for power function
 * @param base_value Base value
 * @param exp_value Exponent value
 * @return Power result
 */
RuntimeValue *math_pow_internal(const RuntimeValue *base_value, const RuntimeValue *exp_value);

/**
 * @brief Internal helper for sine function
 * @param num_value Value to get sine of
 * @return Sine result
 */
RuntimeValue *math_sin_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for cosine function
 * @param num_value Value to get cosine of
 * @return Cosine result
 */
RuntimeValue *math_cos_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for tangent function
 * @param num_value Value to get tangent of
 * @return Tangent result
 */
RuntimeValue *math_tan_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for natural logarithm
 * @param num_value Value to get logarithm of
 * @return Logarithm result
 */
RuntimeValue *math_log_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper for random number generation
 * @return Random number between 0 and 1
 */
RuntimeValue *math_random_internal(void);

/**
 * @brief Internal helper for random integer generation
 * @param min_value Minimum value
 * @param max_value Maximum value
 * @return Random integer in range
 */
RuntimeValue *math_random_int_internal(const RuntimeValue *min_value,
                                       const RuntimeValue *max_value);

/**
 * @brief Internal helper for minimum value
 * @param a_value First value
 * @param b_value Second value
 * @return Minimum value
 */
RuntimeValue *math_min_internal(const RuntimeValue *a_value, const RuntimeValue *b_value);

/**
 * @brief Internal helper for maximum value
 * @param a_value First value
 * @param b_value Second value
 * @return Maximum value
 */
RuntimeValue *math_max_internal(const RuntimeValue *a_value, const RuntimeValue *b_value);

/**
 * @brief Internal helper to check if value is NaN
 * @param num_value Value to check
 * @return Boolean result
 */
RuntimeValue *math_is_nan_internal(const RuntimeValue *num_value);

/**
 * @brief Internal helper to check if value is infinite
 * @param num_value Value to check
 * @return Boolean result
 */
RuntimeValue *math_is_infinite_internal(const RuntimeValue *num_value);

// Stdlib wrapper functions (MANIFEST.json signatures)
/**
 * @brief Absolute value function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Absolute value result
 */
RuntimeValue *math_abs(RuntimeValue **args, size_t argc);

/**
 * @brief Floor function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Floor result
 */
RuntimeValue *math_floor(RuntimeValue **args, size_t argc);

/**
 * @brief Ceiling function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Ceiling result
 */
RuntimeValue *math_ceil(RuntimeValue **args, size_t argc);

/**
 * @brief Round function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Rounded result
 */
RuntimeValue *math_round(RuntimeValue **args, size_t argc);

/**
 * @brief Square root function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Square root result
 */
RuntimeValue *math_sqrt(RuntimeValue **args, size_t argc);

/**
 * @brief Power function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Power result
 */
RuntimeValue *math_pow(RuntimeValue **args, size_t argc);

/**
 * @brief Sine function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Sine result
 */
RuntimeValue *math_sin(RuntimeValue **args, size_t argc);

/**
 * @brief Cosine function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Cosine result
 */
RuntimeValue *math_cos(RuntimeValue **args, size_t argc);

/**
 * @brief Tangent function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Tangent result
 */
RuntimeValue *math_tan(RuntimeValue **args, size_t argc);

/**
 * @brief Natural logarithm function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Logarithm result
 */
RuntimeValue *math_log(RuntimeValue **args, size_t argc);

/**
 * @brief Random number function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Random number between 0 and 1
 */
RuntimeValue *math_random(RuntimeValue **args, size_t argc);

/**
 * @brief Random integer function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Random integer in range
 */
RuntimeValue *math_random_int(RuntimeValue **args, size_t argc);

/**
 * @brief Minimum value function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Minimum value
 */
RuntimeValue *math_min(RuntimeValue **args, size_t argc);

/**
 * @brief Maximum value function - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Maximum value
 */
RuntimeValue *math_max(RuntimeValue **args, size_t argc);

/**
 * @brief Check if value is NaN - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Boolean result
 */
RuntimeValue *math_is_nan(RuntimeValue **args, size_t argc);

/**
 * @brief Check if value is infinite - stdlib wrapper
 * @param args Arguments array
 * @param argc Number of arguments
 * @return Boolean result
 */
RuntimeValue *math_is_infinite(RuntimeValue **args, size_t argc);

#endif  // ZEN_STDLIB_MATH_H