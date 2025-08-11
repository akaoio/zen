/*
 * operators.c
 * Comprehensive operator implementations for ZEN
 *
 * This file implements all ZEN operators including:
 * - Basic arithmetic and logical operators (+, -, *, /, %, =, ≠, <, >)
 * - Standard logical operators (AND, OR, NOT) with undecidable support
 * - Three-valued logic operators (Lukasiewicz ⊗ ⊕, Kleene ∧₃ ∨₃)
 * - Specialized operators for global applications:
 *   - Probabilistic logic for quantum computing
 *   - Consensus operators for distributed systems
 *   - Temporal logic for AI planning
 *   - Fuzzy membership for vague concepts
 *
 * Total: 21 operators (751 lines)
 *
 * DO NOT modify function signatures without updating the manifest
 */

#define _GNU_SOURCE
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/zen/core/memory.h"
#include "../include/zen/core/runtime_value.h"

/* Forward declarations for helper functions */
static RuntimeValue *operators_create_error(const char *message);
static bool operators_to_number(const RuntimeValue *value, double *result);
static char *operators_concat_strings(const char *a, const char *b);
static bool operators_is_truthy(const RuntimeValue *value);
static int operators_compare_values(const RuntimeValue *a, const RuntimeValue *b);
static int operators_value_to_three_valued(const RuntimeValue *value);
static RuntimeValue *operators_three_valued_to_value(int three_val);

/**
 * @brief Addition operator
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of addition operation or error value
 */
RuntimeValue *op_add(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in addition");
    }

    /* String concatenation has highest priority */
    if (a->type == RV_STRING || b->type == RV_STRING) {
        char *a_str = rv_to_string(a);
        char *b_str = rv_to_string(b);

        if (!a_str || !b_str) {
            memory_free(a_str);
            memory_free(b_str);
            return operators_create_error("Failed to convert operand to string");
        }

        char *result = operators_concat_strings(a_str, b_str);
        memory_free(a_str);
        memory_free(b_str);

        if (!result) {
            return operators_create_error("String concatenation failed");
        }

        RuntimeValue *str_val = rv_new_string(result);
        memory_free(result);
        return str_val;
    }

    /* Numeric addition */
    double a_num, b_num;
    if (!operators_to_number(a, &a_num) || !operators_to_number(b, &b_num)) {
        return operators_create_error("Cannot convert operands to numbers for addition");
    }

    // Check for NaN or infinity in operands
    if (isnan(a_num) || isnan(b_num)) {
        return rv_new_number(NAN);
    }

    // Perform addition with overflow detection
    double result = a_num + b_num;

    // Check for overflow/underflow
    if (isinf(result) && !isinf(a_num) && !isinf(b_num)) {
        return operators_create_error("Numeric overflow in addition");
    }

    return rv_new_number(result);
}

/**
 * @brief Subtraction operator
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of subtraction operation or error value
 */
RuntimeValue *op_subtract(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in subtraction");
    }

    double a_num, b_num;
    if (!operators_to_number(a, &a_num) || !operators_to_number(b, &b_num)) {
        return operators_create_error("Cannot convert operands to numbers for subtraction");
    }

    return rv_new_number(a_num - b_num);
}

/**
 * @brief Multiplication operator
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of multiplication operation or error value
 */
RuntimeValue *op_multiply(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in multiplication");
    }

    double a_num, b_num;
    if (!operators_to_number(a, &a_num) || !operators_to_number(b, &b_num)) {
        return operators_create_error("Cannot convert operands to numbers for multiplication");
    }

    // Handle NaN cases
    if (isnan(a_num) || isnan(b_num)) {
        return rv_new_number(NAN);
    }

    // Handle zero multiplication optimization
    if (a_num == 0.0 || b_num == 0.0) {
        return rv_new_number(0.0);
    }

    double result = a_num * b_num;

    // Check for overflow
    if (isinf(result) && !isinf(a_num) && !isinf(b_num)) {
        return operators_create_error("Numeric overflow in multiplication");
    }

    return rv_new_number(result);
}

/**
 * @brief Division operator
 * @param a Left operand value (dividend)
 * @param b Right operand value (divisor)
 * @return Result of division operation or error value
 */
RuntimeValue *op_divide(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in division");
    }

    double a_num, b_num;
    if (!operators_to_number(a, &a_num) || !operators_to_number(b, &b_num)) {
        return operators_create_error("Cannot convert operands to numbers for division");
    }

    if (b_num == 0.0) {
        // Return infinity for division by zero (mathematical standard)
        if (a_num > 0.0) {
            return rv_new_number(INFINITY);
        } else if (a_num < 0.0) {
            return rv_new_number(-INFINITY);
        } else {
            // 0/0 case
            return rv_new_number(NAN);
        }
    }

    return rv_new_number(a_num / b_num);
}

/**
 * @brief Modulo operator
 * @param a Left operand value (dividend)
 * @param b Right operand value (divisor)
 * @return Result of modulo operation or error value
 */
RuntimeValue *op_modulo(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in modulo");
    }

    double a_num, b_num;
    if (!operators_to_number(a, &a_num) || !operators_to_number(b, &b_num)) {
        return operators_create_error("Cannot convert operands to numbers for modulo");
    }

    if (b_num == 0.0) {
        return operators_create_error("Modulo by zero");
    }

    return rv_new_number(fmod(a_num, b_num));
}

/**
 * @brief Equality comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating equality or error value
 */
RuntimeValue *op_equals(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in equality comparison");
    }

    return rv_new_boolean(rv_equals(a, b));
}

/**
 * @brief Inequality comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating inequality or error value
 */
RuntimeValue *op_not_equals(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in inequality comparison");
    }

    return rv_new_boolean(!rv_equals(a, b));
}

/**
 * @brief Less than comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating if a < b or error value
 */
RuntimeValue *op_less_than(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in less than comparison");
    }

    int cmp = operators_compare_values(a, b);
    if (cmp == INT_MIN) {
        return operators_create_error("Cannot compare values of incompatible types");
    }

    return rv_new_boolean(cmp < 0);
}

/**
 * @brief Greater than comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating if a > b or error value
 */
RuntimeValue *op_greater_than(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in greater than comparison");
    }

    int cmp = operators_compare_values(a, b);
    if (cmp == INT_MIN) {
        return operators_create_error("Cannot compare values of incompatible types");
    }

    return rv_new_boolean(cmp > 0);
}

/**
 * @brief Logical AND with undecidable support
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical AND operation or error value
 */
RuntimeValue *op_logical_and(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in logical AND");
    }

    /* Undecidable logic: undecidable AND anything = undecidable */
    if (a->type == RV_NULL || b->type == RV_NULL) {
        return rv_new_null();
    }

    /* Short-circuit evaluation: if a is falsy, return a */
    if (!operators_is_truthy(a)) {
        return rv_ref(a);
    }

    /* Otherwise return b */
    return rv_ref(b);
}

/**
 * @brief Logical OR with undecidable support
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical OR operation or error value
 */
RuntimeValue *op_logical_or(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in logical OR");
    }

    /* Undecidable logic: undecidable OR anything = undecidable */
    if (a->type == RV_NULL || b->type == RV_NULL) {
        return rv_new_null();
    }

    /* Short-circuit evaluation: if a is truthy, return a */
    if (operators_is_truthy(a)) {
        return rv_ref(a);
    }

    /* Otherwise return b */
    return rv_ref(b);
}

/**
 * @brief Logical NOT with undecidable support
 * @param a Operand value to negate
 * @return Boolean value indicating logical negation or error value
 */
RuntimeValue *op_logical_not(RuntimeValue *a)
{
    if (!a) {
        return operators_create_error("Null operand in logical NOT");
    }

    /* Undecidable logic: NOT undecidable = undecidable */
    if (a->type == RV_NULL) {
        return rv_new_null();
    }

    return rv_new_boolean(!operators_is_truthy(a));
}

/* Helper function implementations */

/**
 * @brief Create an error value with a message
 * @param message Error message
 * @return Error value
 */
static RuntimeValue *operators_create_error(const char *message)
{
    RuntimeValue *error = rv_new_error(message, -1);
    // Error creation handled by rv_new_error
    return error;
}

/**
 * @brief Convert a value to a number
 * @param value Value to convert
 * @param result Pointer to store the result
 * @return true if conversion successful, false otherwise
 */
static bool operators_to_number(const RuntimeValue *value, double *result)
{
    if (!value || !result) {
        return false;
    }

    switch (value->type) {
    case RV_NUMBER:
        *result = value->data.number;
        return true;

    case RV_BOOLEAN:
        *result = value->data.boolean ? 1.0 : 0.0;
        return true;

    case RV_NULL:
        *result = 0.0;
        return true;

    case RV_STRING:
        if (value->data.string.data) {
            char *endptr;
            *result = strtod(value->data.string.data, &endptr);
            return *endptr == '\0' && endptr != value->data.string.data;
        }
        return false;

        // Removed duplicate RV_NULL case

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
static char *operators_concat_strings(const char *a, const char *b)
{
    if (!a || !b) {
        return NULL;
    }

    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    size_t total_len = len_a + len_b + 1;

    char *result = memory_alloc(total_len);
    if (!result) {
        return NULL;
    }

    strncpy(result, a, total_len - 1);
    result[total_len - 1] = '\0';
    strncat(result, b, total_len - strlen(result) - 1);

    return result;
}

/**
 * @brief Check if a value is truthy
 * @param value Value to check
 * @return true if truthy, false if falsy
 */
static bool operators_is_truthy(const RuntimeValue *value)
{
    if (!value) {
        return false;
    }

    switch (value->type) {
    case RV_NULL:
        return false;

    case RV_BOOLEAN:
        return value->data.boolean;

    case RV_NUMBER:
        return value->data.number != 0.0 && !isnan(value->data.number);

    case RV_STRING:
        return value->data.string.data && strlen(value->data.string.data) > 0;

    case RV_ARRAY:
        return value->data.array.count > 0;

    case RV_OBJECT:
        return value->data.object.count > 0;

    case RV_ERROR:
        return false;

        // Removed duplicate RV_NULL case (already handled above)

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
static int operators_compare_values(const RuntimeValue *a, const RuntimeValue *b)
{
    if (!a || !b) {
        return INT_MIN;
    }

    /* Same type comparisons */
    if (a->type == b->type) {
        switch (a->type) {
        case RV_NULL:
            return 0;

        case RV_BOOLEAN:
            if (a->data.boolean == b->data.boolean)
                return 0;
            return a->data.boolean ? 1 : -1;

        case RV_NUMBER: {
            double diff = a->data.number - b->data.number;
            if (diff < 0)
                return -1;
            if (diff > 0)
                return 1;
            return 0;
        }

        case RV_STRING:
            if (!a->data.string.data || !b->data.string.data) {
                return INT_MIN;
            }
            return strcmp(a->data.string.data, b->data.string.data);

            // Removed duplicate RV_NULL case (already handled above)

        default:
            return INT_MIN; /* Cannot compare arrays, objects, etc. */
        }
    }

    /* Different type comparisons - try to convert to numbers */
    double a_num, b_num;
    if (operators_to_number(a, &a_num) && operators_to_number(b, &b_num)) {
        double diff = a_num - b_num;
        if (diff < 0)
            return -1;
        if (diff > 0)
            return 1;
        return 0;
    }

    /* Cannot compare */
    return INT_MIN;
}

/* ============================================================================
 * THREE-VALUED LOGIC OPERATORS
 * For advanced applications: formal logic, quantum computing, AI reasoning
 * ============================================================================ */

/**
 * @brief Lukasiewicz three-valued AND (⊗)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of three-valued AND operation
 *
 * Truth table:
 *   T ⊗ T = T, T ⊗ U = U, T ⊗ F = F
 *   U ⊗ T = U, U ⊗ U = U, U ⊗ F = F
 *   F ⊗ T = F, F ⊗ U = F, F ⊗ F = F
 */
RuntimeValue *op_undecidable_and(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in undecidable AND");
    }

    /* Convert to three-valued logic */
    int a_val = operators_value_to_three_valued(a);
    int b_val = operators_value_to_three_valued(b);

    /* Lukasiewicz AND: min(a, b) */
    int result = (a_val < b_val) ? a_val : b_val;

    return operators_three_valued_to_value(result);
}

/**
 * @brief Lukasiewicz three-valued OR (⊕)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of three-valued OR operation
 *
 * Truth table:
 *   T ⊕ T = T, T ⊕ U = T, T ⊕ F = T
 *   U ⊕ T = T, U ⊕ U = U, U ⊕ F = U
 *   F ⊕ T = T, F ⊕ U = U, F ⊕ F = F
 */
RuntimeValue *op_undecidable_or(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in undecidable OR");
    }

    /* Convert to three-valued logic */
    int a_val = operators_value_to_three_valued(a);
    int b_val = operators_value_to_three_valued(b);

    /* Lukasiewicz OR: max(a, b) */
    int result = (a_val > b_val) ? a_val : b_val;

    return operators_three_valued_to_value(result);
}

/**
 * @brief Kleene three-valued strong AND (∧₃)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of Kleene AND operation
 *
 * Different from Lukasiewicz - handles uncertainty differently
 */
RuntimeValue *op_kleene_and(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in Kleene AND");
    }

    int a_val = operators_value_to_three_valued(a);
    int b_val = operators_value_to_three_valued(b);

    /* Kleene AND: if either is false, result is false */
    if (a_val == -1 || b_val == -1)
        return rv_new_boolean(false);
    if (a_val == 1 && b_val == 1)
        return rv_new_boolean(true);

    return rv_new_null();
}

/**
 * @brief Kleene three-valued strong OR (∨₃)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of Kleene OR operation
 */
RuntimeValue *op_kleene_or(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in Kleene OR");
    }

    int a_val = operators_value_to_three_valued(a);
    int b_val = operators_value_to_three_valued(b);

    /* Kleene OR: if either is true, result is true */
    if (a_val == 1 || b_val == 1)
        return rv_new_boolean(true);
    if (a_val == -1 && b_val == -1)
        return rv_new_boolean(false);

    return rv_new_null();
}

/**
 * @brief Three-valued implication (→₃)
 * @param a Left operand value (antecedent)
 * @param b Right operand value (consequent)
 * @return Result of three-valued implication
 *
 * Useful for AI reasoning and formal logic
 */
RuntimeValue *op_undecidable_implies(RuntimeValue *a, RuntimeValue *b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in undecidable implication");
    }

    int a_val = operators_value_to_three_valued(a);
    int b_val = operators_value_to_three_valued(b);

    /* Three-valued implication: max(1-a, b) */
    int result = ((1 - a_val) > b_val) ? (1 - a_val) : b_val;
    if (result > 1)
        result = 1;
    if (result < -1)
        result = -1;

    return operators_three_valued_to_value(result);
}

/**
 * @brief Probabilistic uncertainty operator
 * @param a Left operand value
 * @param b Right operand value
 * @param probability_a Probability of a being true
 * @param probability_b Probability of b being true
 * @return Result based on probabilistic logic
 *
 * For quantum computing and statistical applications
 */
RuntimeValue *
op_probabilistic_and(RuntimeValue *a, RuntimeValue *b, double probability_a, double probability_b)
{
    if (!a || !b) {
        return operators_create_error("Null operand in probabilistic AND");
    }

    /* If either value is undecidable, use probability */
    if (a->type == RV_NULL || b->type == RV_NULL) {
        double combined_probability = probability_a * probability_b;

        /* If combined probability is very low or high, decide */
        if (combined_probability < 0.1)
            return rv_new_boolean(false);
        if (combined_probability > 0.9)
            return rv_new_boolean(true);

        /* Otherwise remain undecidable */
        return rv_new_null();
    }

    /* Regular boolean AND for decided values */
    bool a_bool = (a->type == RV_BOOLEAN) ? a->data.boolean : true;
    bool b_bool = (b->type == RV_BOOLEAN) ? b->data.boolean : true;

    return rv_new_boolean(a_bool && b_bool);
}

/**
 * @brief Consensus operator for distributed systems
 * @param votes Array of vote values
 * @param vote_count Number of votes
 * @param threshold Required consensus threshold (0.0 to 1.0)
 * @return Consensus result or undecidable
 *
 * Returns undecidable if no clear consensus
 */
RuntimeValue *op_consensus(RuntimeValue *votes[], size_t vote_count, double threshold)
{
    if (!votes || vote_count == 0) {
        return operators_create_error("No votes provided for consensus");
    }

    int true_votes = 0;
    int false_votes = 0;
    int undecidable_votes = 0;

    for (size_t i = 0; i < vote_count; i++) {
        if (!votes[i])
            continue;

        switch (votes[i]->type) {
        case RV_BOOLEAN:
            if (votes[i]->data.boolean) {
                true_votes++;
            } else {
                false_votes++;
            }
            break;
        case RV_NULL:
            undecidable_votes++;
            break;
        default:
            /* Treat other types as undecidable for consensus */
            undecidable_votes++;
            break;
        }
    }

    double true_ratio = (double)true_votes / vote_count;
    double false_ratio = (double)false_votes / vote_count;
    double undecidable_ratio = (double)undecidable_votes / vote_count;

    /* If too many undecidable votes, consensus is undecidable */
    if (undecidable_ratio > 0.3) {
        return rv_new_null();
    }

    /* Check if threshold is met for either side */
    if (true_ratio >= threshold) {
        return rv_new_boolean(true);
    } else if (false_ratio >= threshold) {
        return rv_new_boolean(false);
    }

    /* No clear consensus */
    return rv_new_null();
}

/**
 * @brief Temporal logic operator - handles time-dependent undecidability
 * @param condition Condition to evaluate
 * @param time_horizon Time horizon for evaluation
 * @return Result based on temporal logic
 *
 * Useful for planning and dynamic systems
 */
RuntimeValue *op_eventually(RuntimeValue *condition, int time_horizon)
{
    if (!condition) {
        return operators_create_error("Null condition in temporal eventually");
    }

    /* If condition is currently decided, return it */
    if (condition->type == RV_BOOLEAN) {
        return rv_ref(condition);
    }

    /* For undecidable conditions, consider time horizon */
    if (condition->type == RV_NULL) {
        if (time_horizon <= 0) {
            /* No time left - remains undecidable */
            return rv_new_null();
        } else if (time_horizon > 100) {
            /* Long time horizon - more likely to become true eventually */
            return rv_new_boolean(true);
        } else {
            /* Medium time horizon - still undecidable */
            return rv_new_null();
        }
    }

    return rv_new_null();
}

/**
 * @brief Fuzzy membership operator
 * @param element Element to test
 * @param set Set to test membership in
 * @param membership_degree Degree of membership (0.0 to 1.0)
 * @return Membership result based on fuzzy logic
 *
 * For handling vague concepts and boundaries
 */
RuntimeValue *
op_fuzzy_membership(RuntimeValue *element, RuntimeValue *set, double membership_degree)
{
    if (!element || !set) {
        return operators_create_error("Null operand in fuzzy membership");
    }

    /* If membership degree is clearly defined */
    if (membership_degree >= 0.8) {
        return rv_new_boolean(true);
    } else if (membership_degree <= 0.2) {
        return rv_new_boolean(false);
    }

    /* Fuzzy boundary - undecidable */
    return rv_new_null();
}

/* ============================================================================
 * HELPER FUNCTIONS FOR THREE-VALUED LOGIC
 * ============================================================================ */

/**
 * @brief Convert value to three-valued logic representation
 * @param value Input value
 * @return 1 for true, 0 for undecidable, -1 for false
 */
static int operators_value_to_three_valued(const RuntimeValue *value)
{
    if (!value)
        return -1;

    switch (value->type) {
    case RV_BOOLEAN:
        return value->data.boolean ? 1 : -1;
    case RV_NULL:
        return 0;
    case RV_NUMBER:
        if (value->data.number == 0.0)
            return -1;
        return 1;
    default:
        return 0; /* Unknown types are undecidable */
    }
}

/**
 * @brief Convert three-valued logic result to Value
 * @param three_val 1 for true, 0 for undecidable, -1 for false
 * @return Corresponding Value object
 */
static RuntimeValue *operators_three_valued_to_value(int three_val)
{
    if (three_val > 0) {
        return rv_new_boolean(true);
    } else if (three_val < 0) {
        return rv_new_boolean(false);
    } else {
        return rv_new_null();
    }
}