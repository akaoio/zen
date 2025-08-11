/*
 * math.c
 * Mathematical functions for ZEN stdlib
 *
 * This file follows MANIFEST.json specification
 * Function signatures must match manifest exactly
 */

#include "zen/stdlib/math.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Initialize random seed once
static bool rand_initialized = false;

static void ensure_random_initialized()
{
    if (!rand_initialized) {
        srand((unsigned int)time(NULL));
        rand_initialized = true;
    }
}

// Internal math functions
RuntimeValue *math_abs_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("abs() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("abs() requires a numeric argument", -1);
    }
    return rv_new_number(fabs(num_value->data.number));
}

RuntimeValue *math_floor_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("floor() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("floor() requires a numeric argument", -1);
    }
    return rv_new_number(floor(num_value->data.number));
}

RuntimeValue *math_ceil_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("ceil() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("ceil() requires a numeric argument", -1);
    }
    return rv_new_number(ceil(num_value->data.number));
}

RuntimeValue *math_round_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("round() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("round() requires a numeric argument", -1);
    }
    return rv_new_number(round(num_value->data.number));
}

RuntimeValue *math_sqrt_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("sqrt() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("sqrt() requires a numeric argument", -1);
    }
    if (num_value->data.number < 0) {
        return rv_new_error("sqrt() requires a non-negative number", -1);
    }
    return rv_new_number(sqrt(num_value->data.number));
}

RuntimeValue *math_pow_internal(const RuntimeValue *base_value, const RuntimeValue *exp_value)
{
    if (!base_value || !exp_value) {
        return rv_new_error("pow() requires two non-null arguments", -1);
    }
    if (base_value->type != RV_NUMBER || exp_value->type != RV_NUMBER) {
        return rv_new_error("pow() requires numeric arguments", -1);
    }
    return rv_new_number(pow(base_value->data.number, exp_value->data.number));
}

RuntimeValue *math_sin_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("sin() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("sin() requires a numeric argument", -1);
    }
    return rv_new_number(sin(num_value->data.number));
}

RuntimeValue *math_cos_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("cos() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("cos() requires a numeric argument", -1);
    }
    return rv_new_number(cos(num_value->data.number));
}

RuntimeValue *math_tan_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("tan() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("tan() requires a numeric argument", -1);
    }
    return rv_new_number(tan(num_value->data.number));
}

RuntimeValue *math_log_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("log() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_error("log() requires a numeric argument", -1);
    }
    if (num_value->data.number <= 0) {
        return rv_new_error("log() requires a positive number", -1);
    }
    return rv_new_number(log(num_value->data.number));
}

RuntimeValue *math_is_nan_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("isNaN() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_boolean(false);  // Non-numbers are not NaN
    }
    return rv_new_boolean(isnan(num_value->data.number));
}

RuntimeValue *math_is_infinite_internal(const RuntimeValue *num_value)
{
    if (!num_value) {
        return rv_new_error("isInfinite() requires a non-null argument", -1);
    }
    if (num_value->type != RV_NUMBER) {
        return rv_new_boolean(false);  // Non-numbers are not infinite
    }
    return rv_new_boolean(isinf(num_value->data.number));
}

// Stdlib wrapper functions
RuntimeValue *math_abs(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("abs() requires exactly 1 argument", -1);
    }
    return math_abs_internal(args[0]);
}

RuntimeValue *math_floor(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("floor() requires exactly 1 argument", -1);
    }
    return math_floor_internal(args[0]);
}

RuntimeValue *math_ceil(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("ceil() requires exactly 1 argument", -1);
    }
    return math_ceil_internal(args[0]);
}

RuntimeValue *math_round(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("round() requires exactly 1 argument", -1);
    }
    return math_round_internal(args[0]);
}

RuntimeValue *math_sqrt(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("sqrt() requires exactly 1 argument", -1);
    }
    return math_sqrt_internal(args[0]);
}

RuntimeValue *math_pow(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("pow() requires exactly 2 arguments", -1);
    }
    return math_pow_internal(args[0], args[1]);
}

RuntimeValue *math_sin(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("sin() requires exactly 1 argument", -1);
    }
    return math_sin_internal(args[0]);
}

RuntimeValue *math_cos(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("cos() requires exactly 1 argument", -1);
    }
    return math_cos_internal(args[0]);
}

RuntimeValue *math_tan(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("tan() requires exactly 1 argument", -1);
    }
    return math_tan_internal(args[0]);
}

RuntimeValue *math_log(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("log() requires exactly 1 argument", -1);
    }
    return math_log_internal(args[0]);
}

RuntimeValue *math_random(RuntimeValue **args, size_t argc)
{
    (void)args;  // Unused parameter
    if (argc != 0) {
        return rv_new_error("random() requires no arguments", -1);
    }
    ensure_random_initialized();
    return rv_new_number((double)rand() / RAND_MAX);
}

RuntimeValue *math_random_int(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("randomInt() requires exactly 2 arguments", -1);
    }
    if (!args[0] || !args[1] || args[0]->type != RV_NUMBER || args[1]->type != RV_NUMBER) {
        return rv_new_error("randomInt() requires two numeric arguments", -1);
    }

    ensure_random_initialized();
    int min_val = (int)args[0]->data.number;
    int max_val = (int)args[1]->data.number;

    if (min_val > max_val) {
        return rv_new_error("randomInt() min value cannot be greater than max", -1);
    }

    int range = max_val - min_val + 1;
    int result = min_val + (rand() % range);
    return rv_new_number((double)result);
}

RuntimeValue *math_min(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("min() requires exactly 2 arguments", -1);
    }
    if (!args[0] || !args[1] || args[0]->type != RV_NUMBER || args[1]->type != RV_NUMBER) {
        return rv_new_error("min() requires two numeric arguments", -1);
    }

    double a = args[0]->data.number;
    double b = args[1]->data.number;
    return rv_new_number(a < b ? a : b);
}

RuntimeValue *math_max(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("max() requires exactly 2 arguments", -1);
    }
    if (!args[0] || !args[1] || args[0]->type != RV_NUMBER || args[1]->type != RV_NUMBER) {
        return rv_new_error("max() requires two numeric arguments", -1);
    }

    double a = args[0]->data.number;
    double b = args[1]->data.number;
    return rv_new_number(a > b ? a : b);
}

RuntimeValue *math_is_nan(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("isNaN() requires exactly 1 argument", -1);
    }
    return math_is_nan_internal(args[0]);
}

RuntimeValue *math_is_infinite(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("isInfinite() requires exactly 1 argument", -1);
    }
    return math_is_infinite_internal(args[0]);
}