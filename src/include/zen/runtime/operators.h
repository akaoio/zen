/*
 * operators.h
 * Operator implementations header
 * 
 * This file declares all ZEN operator functions
 */

#ifndef ZEN_RUNTIME_OPERATORS_H
#define ZEN_RUNTIME_OPERATORS_H

#include "../types/value.h"

/**
 * @brief Addition operator
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of addition operation or error value
 */
Value* op_add(Value* a, Value* b);

/**
 * @brief Subtraction operator
 * @param a Left operand value
 * @param b Right operand value  
 * @return Result of subtraction operation or error value
 */
Value* op_subtract(Value* a, Value* b);

/**
 * @brief Multiplication operator
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of multiplication operation or error value
 */
Value* op_multiply(Value* a, Value* b);

/**
 * @brief Division operator
 * @param a Left operand value (dividend)
 * @param b Right operand value (divisor)
 * @return Result of division operation or error value
 */
Value* op_divide(Value* a, Value* b);

/**
 * @brief Modulo operator
 * @param a Left operand value (dividend)
 * @param b Right operand value (divisor)
 * @return Result of modulo operation or error value
 */
Value* op_modulo(Value* a, Value* b);

/**
 * @brief Equality comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating equality or error value
 */
Value* op_equals(Value* a, Value* b);

/**
 * @brief Inequality comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating inequality or error value
 */
Value* op_not_equals(Value* a, Value* b);

/**
 * @brief Less than comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating if a < b or error value
 */
Value* op_less_than(Value* a, Value* b);

/**
 * @brief Greater than comparison
 * @param a Left operand value
 * @param b Right operand value
 * @return Boolean value indicating if a > b or error value
 */
Value* op_greater_than(Value* a, Value* b);

/**
 * @brief Logical AND
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical AND operation or error value
 */
Value* op_logical_and(Value* a, Value* b);

/**
 * @brief Logical OR
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of logical OR operation or error value
 */
Value* op_logical_or(Value* a, Value* b);

/**
 * @brief Logical NOT
 * @param a Operand value to negate
 * @return Boolean value indicating logical negation or error value
 */
Value* op_logical_not(Value* a);

/* ============================================================================
 * THREE-VALUED LOGIC OPERATORS
 * ============================================================================ */

/**
 * @brief Lukasiewicz three-valued AND (⊗)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of three-valued AND operation
 */
Value* op_undecidable_and(Value* a, Value* b);

/**
 * @brief Lukasiewicz three-valued OR (⊕)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of three-valued OR operation
 */
Value* op_undecidable_or(Value* a, Value* b);

/**
 * @brief Kleene three-valued strong AND (∧₃)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of Kleene AND operation
 */
Value* op_kleene_and(Value* a, Value* b);

/**
 * @brief Kleene three-valued strong OR (∨₃)
 * @param a Left operand value
 * @param b Right operand value
 * @return Result of Kleene OR operation
 */
Value* op_kleene_or(Value* a, Value* b);

/**
 * @brief Three-valued implication (→₃)
 * @param a Left operand value (antecedent)
 * @param b Right operand value (consequent)
 * @return Result of three-valued implication
 */
Value* op_undecidable_implies(Value* a, Value* b);

/**
 * @brief Probabilistic uncertainty operator
 * @param a Left operand value
 * @param b Right operand value
 * @param probability_a Probability of a being true
 * @param probability_b Probability of b being true
 * @return Result based on probabilistic logic
 */
Value* op_probabilistic_and(Value* a, Value* b, double probability_a, double probability_b);

/**
 * @brief Consensus operator for distributed systems
 * @param votes Array of vote values
 * @param vote_count Number of votes
 * @param threshold Required consensus threshold (0.0 to 1.0)
 * @return Consensus result or undecidable
 */
Value* op_consensus(Value* votes[], size_t vote_count, double threshold);

/**
 * @brief Temporal logic operator - eventually
 * @param condition Condition to evaluate
 * @param time_horizon Time horizon for evaluation
 * @return Result based on temporal logic
 */
Value* op_eventually(Value* condition, int time_horizon);

/**
 * @brief Fuzzy membership operator
 * @param element Element to test
 * @param set Set to test membership in
 * @param membership_degree Degree of membership (0.0 to 1.0)
 * @return Membership result based on fuzzy logic
 */
Value* op_fuzzy_membership(Value* element, Value* set, double membership_degree);

#endif /* ZEN_RUNTIME_OPERATORS_H */