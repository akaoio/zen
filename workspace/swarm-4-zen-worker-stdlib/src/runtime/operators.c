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
static int value_to_three_valued(const Value* value);
static Value* three_valued_to_value(int three_val);

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
        // Return infinity for division by zero (mathematical standard)
        if (a_num > 0.0) {
            return value_new_number(INFINITY);
        } else if (a_num < 0.0) {
            return value_new_number(-INFINITY);
        } else {
            // 0/0 case
            return value_new_number(NAN);
        }
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
 * @brief Logical AND with undecidable support
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical AND operation or error value
 */
Value* op_logical_and(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in logical AND");
    }
    
    /* Undecidable logic: undecidable AND anything = undecidable */
    if (a->type == VALUE_UNDECIDABLE || b->type == VALUE_UNDECIDABLE) {
        return value_new_undecidable();
    }
    
    /* Short-circuit evaluation: if a is falsy, return a */
    if (!is_truthy(a)) {
        return value_ref(a);
    }
    
    /* Otherwise return b */
    return value_ref(b);
}

/**
 * @brief Logical OR with undecidable support
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical OR operation or error value
 */
Value* op_logical_or(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in logical OR");
    }
    
    /* Undecidable logic: undecidable OR anything = undecidable */
    if (a->type == VALUE_UNDECIDABLE || b->type == VALUE_UNDECIDABLE) {
        return value_new_undecidable();
    }
    
    /* Short-circuit evaluation: if a is truthy, return a */
    if (is_truthy(a)) {
        return value_ref(a);
    }
    
    /* Otherwise return b */
    return value_ref(b);
}

/**
 * @brief Logical NOT with undecidable support
 * @param a Operand value to negate
 * @return Boolean value indicating logical negation or error value
 */
Value* op_logical_not(Value* a) {
    if (!a) {
        return create_error("Null operand in logical NOT");
    }
    
    /* Undecidable logic: NOT undecidable = undecidable */
    if (a->type == VALUE_UNDECIDABLE) {
        return value_new_undecidable();
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
            
        case VALUE_UNDECIDABLE:
            return false;  /* Undecidable values cannot be converted to numbers */
            
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
            
        case VALUE_UNDECIDABLE:
            return false;  /* Undecidable values are falsy */
            
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
                
            case VALUE_UNDECIDABLE:
                return 0;  /* All undecidable values are equal to each other */
                
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
Value* op_undecidable_and(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in undecidable AND");
    }
    
    /* Convert to three-valued logic */
    int a_val = value_to_three_valued(a);
    int b_val = value_to_three_valued(b);
    
    /* Lukasiewicz AND: min(a, b) */
    int result = (a_val < b_val) ? a_val : b_val;
    
    return three_valued_to_value(result);
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
Value* op_undecidable_or(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in undecidable OR");
    }
    
    /* Convert to three-valued logic */
    int a_val = value_to_three_valued(a);
    int b_val = value_to_three_valued(b);
    
    /* Lukasiewicz OR: max(a, b) */
    int result = (a_val > b_val) ? a_val : b_val;
    
    return three_valued_to_value(result);
}

/**
 * @brief Kleene three-valued strong AND (∧₃)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of Kleene AND operation
 * 
 * Different from Lukasiewicz - handles uncertainty differently
 */
Value* op_kleene_and(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in Kleene AND");
    }
    
    int a_val = value_to_three_valued(a);
    int b_val = value_to_three_valued(b);
    
    /* Kleene AND: if either is false, result is false */
    if (a_val == -1 || b_val == -1) return value_new_boolean(false);
    if (a_val == 1 && b_val == 1) return value_new_boolean(true);
    
    return value_new_undecidable();
}

/**
 * @brief Kleene three-valued strong OR (∨₃)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of Kleene OR operation
 */
Value* op_kleene_or(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in Kleene OR");
    }
    
    int a_val = value_to_three_valued(a);
    int b_val = value_to_three_valued(b);
    
    /* Kleene OR: if either is true, result is true */
    if (a_val == 1 || b_val == 1) return value_new_boolean(true);
    if (a_val == -1 && b_val == -1) return value_new_boolean(false);
    
    return value_new_undecidable();
}

/**
 * @brief Three-valued implication (→₃)
 * @param a Left operand value (antecedent)
 * @param b Right operand value (consequent)
 * @return Result of three-valued implication
 * 
 * Useful for AI reasoning and formal logic
 */
Value* op_undecidable_implies(Value* a, Value* b) {
    if (!a || !b) {
        return create_error("Null operand in undecidable implication");
    }
    
    int a_val = value_to_three_valued(a);
    int b_val = value_to_three_valued(b);
    
    /* Three-valued implication: max(1-a, b) */
    int result = ((1-a_val) > b_val) ? (1-a_val) : b_val;
    if (result > 1) result = 1;
    if (result < -1) result = -1;
    
    return three_valued_to_value(result);
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
Value* op_probabilistic_and(Value* a, Value* b, double probability_a, double probability_b) {
    if (!a || !b) {
        return create_error("Null operand in probabilistic AND");
    }
    
    /* If either value is undecidable, use probability */
    if (a->type == VALUE_UNDECIDABLE || b->type == VALUE_UNDECIDABLE) {
        double combined_probability = probability_a * probability_b;
        
        /* If combined probability is very low or high, decide */
        if (combined_probability < 0.1) return value_new_boolean(false);
        if (combined_probability > 0.9) return value_new_boolean(true);
        
        /* Otherwise remain undecidable */
        return value_new_undecidable();
    }
    
    /* Regular boolean AND for decided values */
    bool a_bool = (a->type == VALUE_BOOLEAN) ? a->as.boolean : true;
    bool b_bool = (b->type == VALUE_BOOLEAN) ? b->as.boolean : true;
    
    return value_new_boolean(a_bool && b_bool);
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
Value* op_consensus(Value* votes[], size_t vote_count, double threshold) {
    if (!votes || vote_count == 0) {
        return create_error("No votes provided for consensus");
    }
    
    int true_votes = 0;
    int false_votes = 0;
    int undecidable_votes = 0;
    
    for (size_t i = 0; i < vote_count; i++) {
        if (!votes[i]) continue;
        
        switch (votes[i]->type) {
            case VALUE_BOOLEAN:
                if (votes[i]->as.boolean) {
                    true_votes++;
                } else {
                    false_votes++;
                }
                break;
            case VALUE_UNDECIDABLE:
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
        return value_new_undecidable();
    }
    
    /* Check if threshold is met for either side */
    if (true_ratio >= threshold) {
        return value_new_boolean(true);
    } else if (false_ratio >= threshold) {
        return value_new_boolean(false);
    }
    
    /* No clear consensus */
    return value_new_undecidable();
}

/**
 * @brief Temporal logic operator - handles time-dependent undecidability
 * @param condition Condition to evaluate
 * @param time_horizon Time horizon for evaluation
 * @return Result based on temporal logic
 * 
 * Useful for planning and dynamic systems
 */
Value* op_eventually(Value* condition, int time_horizon) {
    if (!condition) {
        return create_error("Null condition in temporal eventually");
    }
    
    /* If condition is currently decided, return it */
    if (condition->type == VALUE_BOOLEAN) {
        return value_ref(condition);
    }
    
    /* For undecidable conditions, consider time horizon */
    if (condition->type == VALUE_UNDECIDABLE) {
        if (time_horizon <= 0) {
            /* No time left - remains undecidable */
            return value_new_undecidable();
        } else if (time_horizon > 100) {
            /* Long time horizon - more likely to become true eventually */
            return value_new_boolean(true);
        } else {
            /* Medium time horizon - still undecidable */
            return value_new_undecidable();
        }
    }
    
    return value_new_undecidable();
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
Value* op_fuzzy_membership(Value* element, Value* set, double membership_degree) {
    if (!element || !set) {
        return create_error("Null operand in fuzzy membership");
    }
    
    /* If membership degree is clearly defined */
    if (membership_degree >= 0.8) {
        return value_new_boolean(true);
    } else if (membership_degree <= 0.2) {
        return value_new_boolean(false);
    }
    
    /* Fuzzy boundary - undecidable */
    return value_new_undecidable();
}

/* ============================================================================
 * HELPER FUNCTIONS FOR THREE-VALUED LOGIC
 * ============================================================================ */

/**
 * @brief Convert value to three-valued logic representation
 * @param value Input value
 * @return 1 for true, 0 for undecidable, -1 for false
 */
static int value_to_three_valued(const Value* value) {
    if (!value) return -1;
    
    switch (value->type) {
        case VALUE_BOOLEAN:
            return value->as.boolean ? 1 : -1;
        case VALUE_UNDECIDABLE:
            return 0;
        case VALUE_NULL:
            return -1;
        case VALUE_NUMBER:
            if (value->as.number == 0.0) return -1;
            return 1;
        default:
            return 0;  /* Unknown types are undecidable */
    }
}

/**
 * @brief Convert three-valued logic result to Value
 * @param three_val 1 for true, 0 for undecidable, -1 for false
 * @return Corresponding Value object
 */
static Value* three_valued_to_value(int three_val) {
    if (three_val > 0) {
        return value_new_boolean(true);
    } else if (three_val < 0) {
        return value_new_boolean(false);
    } else {
        return value_new_undecidable();
    }
}