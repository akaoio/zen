#ifndef ZEN_STDLIB_LOGIC_H
#define ZEN_STDLIB_LOGIC_H

#include "zen/core/ast.h"  // For AST-based structured logic (Gödel approved!)
#include "zen/core/runtime_value.h"

#include <stddef.h>

/**
 * @brief Formal Logic and Theorem Proving System for ZEN Language
 *
 * This module implements Phase 3 of the ZEN language specification:
 * formal logic capabilities with natural language theorem definition,
 * proof verification, and mathematical reasoning support.
 *
 * Features:
 * - Natural language theorem definition and management
 * - Formal proof verification with <1s performance target
 * - Axiom system with consistency validation
 * - Interactive proof construction
 * - Theorem storage and retrieval
 * - Mathematical reasoning capabilities
 */

// Forward declarations for internal types
typedef struct ZenTheorem ZenTheorem;
typedef struct ZenProof ZenProof;
typedef struct ZenAxiom ZenAxiom;
typedef struct ZenLogicSystem ZenLogicSystem;

/**
 * @brief Theorem structure - PURE AST ONLY (no strings!)
 */
typedef struct ZenTheorem {
    char *name;               // Theorem name/identifier only
    AST_T *statement_ast;     // PURE AST statement - no strings!
    ZenProof *proof;          // Associated proof (if proven)
    bool is_proven;           // Whether theorem has been proven
    AST_T **hypothesis_asts;  // Array of hypothesis ASTs
    size_t hypothesis_count;  // Number of hypotheses
    AST_T *conclusion_ast;    // Conclusion AST
    int ref_count;            // Reference counting for memory management
} ZenTheorem;

/**
 * @brief Proof structure - PURE AST ONLY (no strings!)
 */
typedef struct ZenProof {
    char *theorem_name;          // Name of theorem being proven only
    AST_T **step_asts;           // Array of proof step ASTs - pure structured data!
    size_t step_count;           // Number of proof steps
    AST_T **justification_asts;  // Justification ASTs - no strings!
    bool is_complete;            // Whether proof is complete
    bool is_valid;               // Whether proof has been verified
    double verification_time;    // Time taken for verification (seconds)
    int ref_count;               // Reference counting for memory management
} ZenProof;

/**
 * @brief Axiom structure - PURE AST ONLY (no strings!)
 */
typedef struct ZenAxiom {
    char *name;            // Axiom name/identifier only
    AST_T *statement_ast;  // PURE AST statement - no strings!
    bool is_consistent;    // Whether axiom is consistent with system
    int ref_count;         // Reference counting for memory management
} ZenAxiom;

// Core Logic System Functions

/**
 * @brief Define a new theorem with natural language statement
 * @param args Arguments array (theorem_name, statement, optional formal_statement)
 * @param argc Number of arguments (2 or 3)
 * @return Theorem value or error on failure
 *
 * Example usage in ZEN:
 * theorem_define "pythagorean_theorem"
 *     "For all right triangles with sides a, b, c where c is hypotenuse, a² + b² = c²"
 */
RuntimeValue *logic_theorem_define(RuntimeValue **args, size_t argc);

/**
 * @brief Verify a proof for a given theorem with <1s performance target
 * @param args Arguments array (theorem_name, proof_steps)
 * @param argc Number of arguments (2)
 * @return Boolean value indicating proof validity, or error on failure
 *
 * Performance target: Complete verification in under 1 second for complex proofs
 */
RuntimeValue *logic_proof_verify(RuntimeValue **args, size_t argc);

/**
 * @brief Add a new axiom to the formal logic system
 * @param args Arguments array (axiom_name, statement, optional formal_statement)
 * @param argc Number of arguments (2 or 3)
 * @return Boolean value indicating success, or error on failure
 *
 * Example usage in ZEN:
 * axiom_add "law_of_excluded_middle"
 *     "For any proposition P, either P is true or P is false"
 */
RuntimeValue *logic_axiom_add(RuntimeValue **args, size_t argc);

/**
 * @brief List all axioms in the logic system
 * @param args Arguments array (unused)
 * @param argc Number of arguments (0)
 * @return Array value containing all axiom names
 */
RuntimeValue *logic_axiom_list(RuntimeValue **args, size_t argc);

/**
 * @brief Validate axiom consistency with existing system
 * @param args Arguments array (axiom_name or axiom_statement)
 * @param argc Number of arguments (1)
 * @return Boolean value indicating consistency, or error on failure
 */
RuntimeValue *logic_axiom_validate(RuntimeValue **args, size_t argc);

/**
 * @brief Interactive proof construction - add a proof step
 * @param args Arguments array (theorem_name, step_statement, justification)
 * @param argc Number of arguments (3)
 * @return Boolean value indicating step validity, or error on failure
 *
 * Example usage in ZEN:
 * proof_step "pythagorean_theorem"
 *     "Given right triangle with legs a and b, hypotenuse c"
 *     "assumption"
 */
RuntimeValue *logic_proof_step(RuntimeValue **args, size_t argc);

/**
 * @brief Store theorem in knowledge base for future reference
 * @param args Arguments array (theorem_name, optional metadata)
 * @param argc Number of arguments (1 or 2)
 * @return Boolean value indicating success, or error on failure
 */
RuntimeValue *logic_theorem_store(RuntimeValue **args, size_t argc);

/**
 * @brief Verify theorem validity and consistency
 * @param args Arguments array (theorem_name)
 * @param argc Number of arguments (1)
 * @return Boolean value indicating validity, or error on failure
 */
RuntimeValue *logic_theorem_verify(RuntimeValue **args, size_t argc);

// Advanced Logic Functions

/**
 * @brief Retrieve theorem by name from knowledge base
 * @param args Arguments array (theorem_name)
 * @param argc Number of arguments (1)
 * @return Theorem value or null if not found
 */
RuntimeValue *logic_theorem_get(RuntimeValue **args, size_t argc);

/**
 * @brief List all theorems in the knowledge base
 * @param args Arguments array (unused)
 * @param argc Number of arguments (0)
 * @return Array value containing all theorem names
 */
RuntimeValue *logic_theorem_list(RuntimeValue **args, size_t argc);

/**
 * @brief Check if a statement follows logically from given premises
 * @param args Arguments array (premises_array, conclusion)
 * @param argc Number of arguments (2)
 * @return Boolean value indicating logical entailment
 */
RuntimeValue *logic_entails(RuntimeValue **args, size_t argc);

/**
 * @brief Apply modus ponens inference rule
 * @param args Arguments array (conditional_statement, antecedent)
 * @param argc Number of arguments (2)
 * @return Consequent value or error if rule doesn't apply
 *
 * Example: Given "if P then Q" and "P", derive "Q"
 */
RuntimeValue *logic_modus_ponens(RuntimeValue **args, size_t argc);

/**
 * @brief Apply modus tollens inference rule
 * @param args Arguments array (conditional_statement, negated_consequent)
 * @param argc Number of arguments (2)
 * @return Negated antecedent value or error if rule doesn't apply
 *
 * Example: Given "if P then Q" and "not Q", derive "not P"
 */
RuntimeValue *logic_modus_tollens(RuntimeValue **args, size_t argc);

/**
 * @brief Check if an AST represents a Gödel sentence and return undecidable value
 * @param ast The AST to analyze for self-referential undecidable patterns
 * @return Value* containing undecidable value if Gödel sentence detected, NULL otherwise
 */
RuntimeValue *logic_check_godel_sentence(const AST_T *ast);

/**
 * @brief Check logical equivalence of two statements
 * @param args Arguments array (statement1, statement2)
 * @param argc Number of arguments (2)
 * @return Boolean value indicating logical equivalence
 */
RuntimeValue *logic_equivalent(RuntimeValue **args, size_t argc);

/**
 * @brief Generate truth table for logical expression
 * @param args Arguments array (logical_expression)
 * @param argc Number of arguments (1)
 * @return Object value containing truth table data
 */
RuntimeValue *logic_truth_table(RuntimeValue **args, size_t argc);

// Natural Language Processing for Logic

/**
 * @brief Parse natural language logical statement
 * @param args Arguments array (natural_language_statement)
 * @param argc Number of arguments (1)
 * @return Structured logical expression or error on parse failure
 */
RuntimeValue *logic_parse_statement(RuntimeValue **args, size_t argc);

/**
 * @brief Convert formal logic to natural language
 * @param args Arguments array (formal_expression)
 * @param argc Number of arguments (1)
 * @return String value with natural language translation
 */
RuntimeValue *logic_to_natural(RuntimeValue **args, size_t argc);

/**
 * @brief Validate natural language mathematical reasoning
 * @param args Arguments array (reasoning_text)
 * @param argc Number of arguments (1)
 * @return Boolean value indicating reasoning validity
 */
RuntimeValue *logic_validate_reasoning(RuntimeValue **args, size_t argc);

// Mathematical Reasoning Support

/**
 * @brief Check if mathematical statement is well-formed
 * @param args Arguments array (mathematical_statement)
 * @param argc Number of arguments (1)
 * @return Boolean value indicating well-formedness
 */
RuntimeValue *logic_math_well_formed(RuntimeValue **args, size_t argc);

/**
 * @brief Apply algebraic manipulation rules
 * @param args Arguments array (expression, rule_name)
 * @param argc Number of arguments (2)
 * @return Transformed expression or error if rule doesn't apply
 */
RuntimeValue *logic_math_apply_rule(RuntimeValue **args, size_t argc);

/**
 * @brief Simplify mathematical expression
 * @param args Arguments array (mathematical_expression)
 * @param argc Number of arguments (1)
 * @return Simplified expression value
 */
RuntimeValue *logic_math_simplify(RuntimeValue **args, size_t argc);

/**
 * @brief Check mathematical equality with symbolic reasoning
 * @param args Arguments array (expression1, expression2)
 * @param argc Number of arguments (2)
 * @return Boolean value indicating mathematical equality
 */
RuntimeValue *logic_math_equal(RuntimeValue **args, size_t argc);

// Proof System Utilities

/**
 * @brief Get proof statistics for a theorem
 * @param args Arguments array (theorem_name)
 * @param argc Number of arguments (1)
 * @return Object value with proof statistics (steps, time, etc.)
 */
RuntimeValue *logic_proof_stats(RuntimeValue **args, size_t argc);

/**
 * @brief Export proof to different formats (LaTeX, Markdown, etc.)
 * @param args Arguments array (theorem_name, format)
 * @param argc Number of arguments (2)
 * @return String value with formatted proof export
 */
RuntimeValue *logic_proof_export(RuntimeValue **args, size_t argc);

/**
 * @brief Import theorem and proof from external format
 * @param args Arguments array (source_text, format)
 * @param argc Number of arguments (2)
 * @return Boolean value indicating successful import
 */
RuntimeValue *logic_proof_import(RuntimeValue **args, size_t argc);

// System Management

/**
 * @brief Initialize the formal logic system
 * @return Boolean value indicating successful initialization
 */
bool logic_system_init(void);

/**
 * @brief Cleanup the formal logic system and free resources
 */
void logic_system_cleanup(void);

/**
 * @brief Get logic system statistics
 * @param args Arguments array (unused)
 * @param argc Number of arguments (0)
 * @return Object value with system statistics
 */
RuntimeValue *logic_system_stats(RuntimeValue **args, size_t argc);

/**
 * @brief Reset the logic system (clear all theorems and proofs)
 * @param args Arguments array (optional confirmation)
 * @param argc Number of arguments (0 or 1)
 * @return Boolean value indicating successful reset
 */
RuntimeValue *logic_system_reset(RuntimeValue **args, size_t argc);

// Internal utility functions (not exposed to ZEN scripts)

/**
 * @brief Create a new theorem structure - PURE AST ONLY
 * @param name Theorem name
 * @param statement_ast Structured AST statement - NO STRINGS!
 * @return Allocated theorem structure or NULL on failure
 */
ZenTheorem *logic_theorem_create(const char *name, AST_T *statement_ast);

/**
 * @brief Free theorem structure and its resources
 * @param theorem Theorem to free
 */
void logic_theorem_free(ZenTheorem *theorem);

/**
 * @brief Create a new proof structure
 * @param theorem_name Name of theorem being proven
 * @return Allocated proof structure or NULL on failure
 */
ZenProof *logic_proof_create(const char *theorem_name);

/**
 * @brief Free proof structure and its resources
 * @param proof Proof to free
 */
void logic_proof_free(ZenProof *proof);

/**
 * @brief Create a new axiom structure - PURE AST ONLY
 * @param name Axiom name
 * @param statement_ast Structured AST statement - NO STRINGS!
 * @return Allocated axiom structure or NULL on failure
 */
ZenAxiom *logic_axiom_create(const char *name, AST_T *statement_ast);

/**
 * @brief Free axiom structure and its resources
 * @param axiom Axiom to free
 */
void logic_axiom_free(ZenAxiom *axiom);

/* ============================================================================
 * PURE AST LOGICAL OPERATIONS - NO STRINGS ALLOWED!
 * ============================================================================ */

/**
 * @brief Validate AST for logical correctness - PURE AST ONLY!
 * @param ast AST to validate
 * @return true if AST is logically well-formed
 */
bool logic_validate_ast(const AST_T *ast);

/**
 * @brief Verify proof steps using AST-based reasoning - NO STRING MATCHING!
 * @param proof_steps Array of proof step ASTs
 * @param step_count Number of steps
 * @return true if proof is logically valid
 */
bool logic_verify_proof_ast(AST_T **proof_steps, size_t step_count);

/**
 * @brief Apply logical inference rule to AST premises
 * @param inference_type Type of inference (modus ponens, etc.)
 * @param premises Array of premise ASTs
 * @param premise_count Number of premises
 * @return Derived conclusion AST or NULL if rule doesn't apply
 */
AST_T *logic_apply_inference_ast(int inference_type, AST_T **premises, size_t premise_count);

/**
 * @brief Check logical equivalence of two ASTs
 * @param ast1 First logical AST
 * @param ast2 Second logical AST
 * @return true if ASTs are logically equivalent
 */
bool logic_ast_equivalent(const AST_T *ast1, const AST_T *ast2);

/**
 * @brief Substitute variables in logical AST
 * @param ast Target AST
 * @param var_name Variable to substitute
 * @param replacement AST to substitute with
 * @return New AST with substitutions made
 */
AST_T *logic_ast_substitute(const AST_T *ast, const char *var_name, const AST_T *replacement);

/**
 * @brief Apply modus ponens to AST premises: P→Q, P ⊢ Q
 * @param conditional AST representing P→Q
 * @param antecedent AST representing P
 * @return AST representing Q, or NULL if rule doesn't apply
 */
AST_T *logic_modus_ponens_ast(const AST_T *conditional, const AST_T *antecedent);

/**
 * @brief Apply modus tollens to AST premises: P→Q, ¬Q ⊢ ¬P
 * @param conditional AST representing P→Q
 * @param negated_consequent AST representing ¬Q
 * @return AST representing ¬P, or NULL if rule doesn't apply
 */
AST_T *logic_modus_tollens_ast(const AST_T *conditional, const AST_T *negated_consequent);

/**
 * @brief Apply universal instantiation: ∀x P(x) ⊢ P(a)
 * @param universal_statement AST representing ∀x P(x)
 * @param instance AST representing the instance 'a'
 * @return AST representing P(a), or NULL if rule doesn't apply
 */
AST_T *logic_universal_instantiation_ast(const AST_T *universal_statement, const AST_T *instance);

// Error codes for logic system
#define ZEN_LOGIC_ERROR_INVALID_THEOREM  -1001
#define ZEN_LOGIC_ERROR_INVALID_PROOF    -1002
#define ZEN_LOGIC_ERROR_INVALID_AXIOM    -1003
#define ZEN_LOGIC_ERROR_PROOF_INCOMPLETE -1004
#define ZEN_LOGIC_ERROR_INCONSISTENT     -1005
#define ZEN_LOGIC_ERROR_TIMEOUT          -1006
#define ZEN_LOGIC_ERROR_PARSE_FAILED     -1007
#define ZEN_LOGIC_ERROR_NOT_FOUND        -1008
#define ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT  -1009
#define ZEN_LOGIC_ERROR_MEMORY_ALLOC     -1010

#endif  // ZEN_STDLIB_LOGIC_H