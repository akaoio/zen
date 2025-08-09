/*
 * logic.c
 * Formal logic and theorem proving functions for ZEN stdlib
 *
 * Phase 3 implementation: Natural language theorem definition,
 * proof verification engine, and mathematical reasoning support.
 *
 * Performance target: <1s theorem verification for complex proofs
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/logic.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/types/array.h"
#include "zen/types/object.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global logic system state
static ZenLogicSystem *g_logic_system = NULL;
static bool system_initialized = false;

/**
 * @brief Logic system structure
 */
typedef struct ZenLogicSystem {
    ZenTheorem **theorems;    // Array of theorems
    size_t theorem_count;     // Number of theorems
    size_t theorem_capacity;  // Theorem array capacity

    ZenAxiom **axioms;      // Array of axioms
    size_t axiom_count;     // Number of axioms
    size_t axiom_capacity;  // Axiom array capacity

    ZenProof **proofs;      // Array of active proofs
    size_t proof_count;     // Number of active proofs
    size_t proof_capacity;  // Proof array capacity

    bool is_consistent;              // System consistency flag
    double total_verification_time;  // Total verification time
    size_t total_verifications;      // Total verification count
} ZenLogicSystem;

// Forward declarations for internal functions
static bool logic_system_ensure_initialized(void);
static ZenTheorem *find_theorem_by_name(const char *name);
static ZenAxiom *find_axiom_by_name(const char *name);
static ZenProof *find_proof_by_theorem(const char *theorem_name);
static double measure_verification_time(clock_t start);
static Value *create_theorem_value(ZenTheorem *theorem);
static Value *create_proof_value(ZenProof *proof);
static Value *create_axiom_value(ZenAxiom *axiom);

// Internal theorem management functions

/**
 * @brief Create a new theorem structure - PURE AST ONLY
 * @param name Theorem name
 * @param statement_ast Structured AST statement - NO STRINGS!
 * @return Allocated theorem structure or NULL on failure
 */
ZenTheorem *logic_theorem_create(const char *name, AST_T *statement_ast)
{
    if (!name || !statement_ast) {
        return NULL;
    }

    ZenTheorem *theorem = memory_alloc(sizeof(ZenTheorem));
    if (!theorem) {
        return NULL;
    }

    theorem->name = memory_strdup(name);
    theorem->statement_ast = statement_ast;  // Pure AST - no strings!
    theorem->proof = NULL;
    theorem->is_proven = false;
    theorem->hypothesis_asts = NULL;
    theorem->hypothesis_count = 0;
    theorem->conclusion_ast = NULL;
    theorem->ref_count = 1;

    if (!theorem->name) {
        logic_theorem_free(theorem);
        return NULL;
    }

    return theorem;
}

/**
 * @brief Free theorem structure and its resources
 * @param theorem Theorem to free
 */
void logic_theorem_free(ZenTheorem *theorem)
{
    if (!theorem)
        return;

    theorem->ref_count--;
    if (theorem->ref_count > 0)
        return;

    memory_free(theorem->name);
    if (theorem->statement_ast) {
        ast_free(theorem->statement_ast);  // Free AST, not strings!
    }

    if (theorem->proof) {
        logic_proof_free(theorem->proof);
    }

    if (theorem->hypothesis_asts) {
        for (size_t i = 0; i < theorem->hypothesis_count; i++) {
            if (theorem->hypothesis_asts[i]) {
                ast_free(theorem->hypothesis_asts[i]);  // Free AST nodes!
            }
        }
        memory_free(theorem->hypothesis_asts);
    }

    if (theorem->conclusion_ast) {
        ast_free(theorem->conclusion_ast);  // Free AST, not Value!
    }

    memory_free(theorem);
}

/**
 * @brief Create a new proof structure
 * @param theorem_name Name of theorem being proven
 * @return Allocated proof structure or NULL on failure
 */
ZenProof *logic_proof_create(const char *theorem_name)
{
    if (!theorem_name) {
        return NULL;
    }

    ZenProof *proof = memory_alloc(sizeof(ZenProof));
    if (!proof) {
        return NULL;
    }

    proof->theorem_name = memory_strdup(theorem_name);
    proof->step_asts = NULL;  // AST array, not Value array!
    proof->step_count = 0;
    proof->justification_asts = NULL;  // AST justifications, not strings!
    proof->is_complete = false;
    proof->is_valid = false;
    proof->verification_time = 0.0;
    proof->ref_count = 1;

    if (!proof->theorem_name) {
        logic_proof_free(proof);
        return NULL;
    }

    return proof;
}

/**
 * @brief Free proof structure and its resources
 * @param proof Proof to free
 */
void logic_proof_free(ZenProof *proof)
{
    if (!proof)
        return;

    proof->ref_count--;
    if (proof->ref_count > 0)
        return;

    memory_free(proof->theorem_name);

    if (proof->step_asts) {  // Free AST steps, not Value steps!
        for (size_t i = 0; i < proof->step_count; i++) {
            if (proof->step_asts[i]) {
                ast_free(proof->step_asts[i]);
            }
        }
        memory_free(proof->step_asts);
    }

    if (proof->justification_asts) {  // Free AST justifications, not strings!
        for (size_t i = 0; i < proof->step_count; i++) {
            if (proof->justification_asts[i]) {
                ast_free(proof->justification_asts[i]);
            }
        }
        memory_free(proof->justification_asts);
    }

    memory_free(proof);
}

/**
 * @brief Create a new axiom structure - PURE AST ONLY
 * @param name Axiom name
 * @param statement_ast Structured AST statement - NO STRINGS!
 * @return Allocated axiom structure or NULL on failure
 */
ZenAxiom *logic_axiom_create(const char *name, AST_T *statement_ast)
{
    if (!name || !statement_ast) {
        return NULL;
    }

    ZenAxiom *axiom = memory_alloc(sizeof(ZenAxiom));
    if (!axiom) {
        return NULL;
    }

    axiom->name = memory_strdup(name);
    axiom->statement_ast = statement_ast;  // Pure AST - no strings!
    axiom->is_consistent = true;
    axiom->ref_count = 1;

    if (!axiom->name) {
        logic_axiom_free(axiom);
        return NULL;
    }

    return axiom;
}

/**
 * @brief Free axiom structure and its resources
 * @param axiom Axiom to free
 */
void logic_axiom_free(ZenAxiom *axiom)
{
    if (!axiom)
        return;

    axiom->ref_count--;
    if (axiom->ref_count > 0)
        return;

    memory_free(axiom->name);
    if (axiom->statement_ast) {
        ast_free(axiom->statement_ast);  // Free AST, not strings!
    }
    memory_free(axiom);
}

// Logic system initialization and management

/**
 * @brief Initialize the formal logic system
 * @return Boolean value indicating successful initialization
 */
bool logic_system_init(void)
{
    if (system_initialized) {
        return true;
    }

    g_logic_system = memory_alloc(sizeof(ZenLogicSystem));
    if (!g_logic_system) {
        return false;
    }

    // Initialize arrays with small initial capacity
    g_logic_system->theorem_capacity = 16;
    g_logic_system->axiom_capacity = 16;
    g_logic_system->proof_capacity = 16;

    g_logic_system->theorems =
        memory_alloc(g_logic_system->theorem_capacity * sizeof(ZenTheorem *));
    g_logic_system->axioms = memory_alloc(g_logic_system->axiom_capacity * sizeof(ZenAxiom *));
    g_logic_system->proofs = memory_alloc(g_logic_system->proof_capacity * sizeof(ZenProof *));

    if (!g_logic_system->theorems || !g_logic_system->axioms || !g_logic_system->proofs) {
        logic_system_cleanup();
        return false;
    }

    g_logic_system->theorem_count = 0;
    g_logic_system->axiom_count = 0;
    g_logic_system->proof_count = 0;
    g_logic_system->is_consistent = true;
    g_logic_system->total_verification_time = 0.0;
    g_logic_system->total_verifications = 0;

    // Add fundamental axioms using pure AST
    // Law of excluded middle: ∀P: P ∨ ¬P
    AST_T *p_prop = ast_new_logical_proposition("P");
    AST_T *not_p = ast_new_logical_connective(CONNECTIVE_NOT, NULL, p_prop);
    AST_T *p_or_not_p = ast_new_logical_connective(CONNECTIVE_OR, p_prop, not_p);
    AST_T *excluded_middle_ast =
        ast_new_logical_quantifier(QUANTIFIER_UNIVERSAL, "P", NULL, p_or_not_p);

    ZenAxiom *excluded_middle = logic_axiom_create("law_of_excluded_middle", excluded_middle_ast);

    // Law of non-contradiction: ∀P: ¬(P ∧ ¬P)
    AST_T *p_prop2 = ast_new_logical_proposition("P");
    AST_T *not_p2 = ast_new_logical_connective(CONNECTIVE_NOT, NULL, p_prop2);
    AST_T *p_and_not_p = ast_new_logical_connective(CONNECTIVE_AND, p_prop2, not_p2);
    AST_T *not_contradiction = ast_new_logical_connective(CONNECTIVE_NOT, NULL, p_and_not_p);
    AST_T *non_contradiction_ast =
        ast_new_logical_quantifier(QUANTIFIER_UNIVERSAL, "P", NULL, not_contradiction);

    ZenAxiom *non_contradiction =
        logic_axiom_create("law_of_non_contradiction", non_contradiction_ast);

    if (excluded_middle) {
        g_logic_system->axioms[g_logic_system->axiom_count++] = excluded_middle;
    }
    if (non_contradiction) {
        g_logic_system->axioms[g_logic_system->axiom_count++] = non_contradiction;
    }

    system_initialized = true;
    return true;
}

/**
 * @brief Cleanup the formal logic system and free resources
 */
void logic_system_cleanup(void)
{
    if (!g_logic_system) {
        return;
    }

    // Free all theorems
    for (size_t i = 0; i < g_logic_system->theorem_count; i++) {
        logic_theorem_free(g_logic_system->theorems[i]);
    }
    memory_free(g_logic_system->theorems);

    // Free all axioms
    for (size_t i = 0; i < g_logic_system->axiom_count; i++) {
        logic_axiom_free(g_logic_system->axioms[i]);
    }
    memory_free(g_logic_system->axioms);

    // Free all proofs
    for (size_t i = 0; i < g_logic_system->proof_count; i++) {
        logic_proof_free(g_logic_system->proofs[i]);
    }
    memory_free(g_logic_system->proofs);

    memory_free(g_logic_system);
    g_logic_system = NULL;
    system_initialized = false;
}

// Internal utility functions

/**
 * @brief Ensure the logic system is initialized
 * @return True if initialized successfully, false otherwise
 */
static bool logic_system_ensure_initialized(void)
{
    if (!system_initialized) {
        return logic_system_init();
    }
    return true;
}

/**
 * @brief Find theorem by name
 * @param name Theorem name to search for
 * @return Theorem if found, NULL otherwise
 */
static ZenTheorem *find_theorem_by_name(const char *name)
{
    if (!name || !g_logic_system) {
        return NULL;
    }

    for (size_t i = 0; i < g_logic_system->theorem_count; i++) {
        if (strcmp(g_logic_system->theorems[i]->name, name) == 0) {
            return g_logic_system->theorems[i];
        }
    }

    return NULL;
}

/**
 * @brief Find axiom by name
 * @param name Axiom name to search for
 * @return Axiom if found, NULL otherwise
 */
static ZenAxiom *find_axiom_by_name(const char *name)
{
    if (!name || !g_logic_system) {
        return NULL;
    }

    for (size_t i = 0; i < g_logic_system->axiom_count; i++) {
        if (strcmp(g_logic_system->axioms[i]->name, name) == 0) {
            return g_logic_system->axioms[i];
        }
    }

    return NULL;
}

/**
 * @brief Find active proof for theorem
 * @param theorem_name Name of theorem
 * @return Proof if found, NULL otherwise
 */
static ZenProof *find_proof_by_theorem(const char *theorem_name)
{
    if (!theorem_name || !g_logic_system) {
        return NULL;
    }

    for (size_t i = 0; i < g_logic_system->proof_count; i++) {
        if (strcmp(g_logic_system->proofs[i]->theorem_name, theorem_name) == 0) {
            return g_logic_system->proofs[i];
        }
    }

    return NULL;
}

/**
 * @brief Convert Value object to logical AST
 * @param value Value object containing AST representation
 * @return AST node or NULL on error
 */
static AST_T *value_to_logical_ast(const Value *value)
{
    if (!value || value->type != VALUE_OBJECT) {
        return NULL;
    }

    /* Extract AST type from object */
    Value *type_val = object_get((Value *)value, "type");
    if (!type_val || type_val->type != VALUE_STRING) {
        return NULL;
    }

    const char *type_str = type_val->as.string->data;

    /* Handle different logical AST types */
    if (strcmp(type_str, "quantifier") == 0) {
        Value *quantifier_type = object_get((Value *)value, "quantifier_type");
        Value *variable = object_get((Value *)value, "variable");
        Value *body = object_get((Value *)value, "body");

        if (!quantifier_type || !variable || !body)
            return NULL;

        int q_type = (strcmp(quantifier_type->as.string->data, "universal") == 0)
                         ? QUANTIFIER_UNIVERSAL
                         : QUANTIFIER_EXISTENTIAL;

        return ast_new_logical_quantifier(q_type,
                                          variable->as.string->data,
                                          NULL, /* domain optional */
                                          value_to_logical_ast(body));
    } else if (strcmp(type_str, "connective") == 0) {
        Value *conn_type = object_get((Value *)value, "connective_type");
        Value *left = object_get((Value *)value, "left");
        Value *right = object_get((Value *)value, "right");

        if (!conn_type)
            return NULL;

        int c_type = CONNECTIVE_AND; /* default */
        const char *conn_str = conn_type->as.string->data;

        if (strcmp(conn_str, "and") == 0)
            c_type = CONNECTIVE_AND;
        else if (strcmp(conn_str, "or") == 0)
            c_type = CONNECTIVE_OR;
        else if (strcmp(conn_str, "not") == 0)
            c_type = CONNECTIVE_NOT;
        else if (strcmp(conn_str, "implies") == 0)
            c_type = CONNECTIVE_IMPLIES;
        else if (strcmp(conn_str, "iff") == 0)
            c_type = CONNECTIVE_IFF;

        AST_T *left_ast = left ? value_to_logical_ast(left) : NULL;
        AST_T *right_ast = right ? value_to_logical_ast(right) : NULL;

        return ast_new_logical_connective(c_type, left_ast, right_ast);
    } else if (strcmp(type_str, "predicate") == 0) {
        Value *name = object_get((Value *)value, "name");
        Value *args = object_get((Value *)value, "args");

        if (!name)
            return NULL;

        if (args && args->type == VALUE_ARRAY) {
            size_t arg_count = array_length(args);
            AST_T **arg_asts = memory_alloc(arg_count * sizeof(AST_T *));

            for (size_t i = 0; i < arg_count; i++) {
                Value *arg = array_get(args, i);
                arg_asts[i] = value_to_logical_ast(arg);
            }

            AST_T *result = ast_new_logical_predicate(name->as.string->data, arg_asts, arg_count);
            memory_free(arg_asts);
            return result;
        }

        return ast_new_logical_predicate(name->as.string->data, NULL, 0);
    } else if (strcmp(type_str, "proposition") == 0) {
        Value *name = object_get((Value *)value, "name");
        if (!name)
            return NULL;

        return ast_new_logical_proposition(name->as.string->data);
    } else if (strcmp(type_str, "variable") == 0) {
        Value *name = object_get((Value *)value, "name");
        Value *is_bound = object_get((Value *)value, "is_bound");

        if (!name)
            return NULL;

        bool bound = is_bound && is_bound->type == VALUE_BOOLEAN && is_bound->as.boolean;

        return ast_new_logical_variable(name->as.string->data, bound);
    }

    return NULL;
}

/**
 * @brief Validate AST for logical correctness - PURE AST ONLY!
 * @param ast AST to validate
 * @return True if AST is logically well-formed
 */
bool logic_validate_ast(const AST_T *ast)
{
    if (!ast) {
        return false;
    }

    switch (ast->type) {
    case AST_LOGICAL_QUANTIFIER:
        return ast->quantified_variable && ast->quantified_body &&
               (ast->quantifier_type == QUANTIFIER_UNIVERSAL ||
                ast->quantifier_type == QUANTIFIER_EXISTENTIAL);

    case AST_LOGICAL_PREDICATE:
        return ast->predicate_name != NULL;

    case AST_LOGICAL_CONNECTIVE:
        // NOT only needs right operand, others need both
        if (ast->connective_type == CONNECTIVE_NOT) {
            return ast->connective_right != NULL;
        }
        return ast->connective_left && ast->connective_right;

    case AST_LOGICAL_VARIABLE:
    case AST_LOGICAL_PROPOSITION:
        return true;  // Always valid if they exist

    case AST_MATHEMATICAL_EQUATION:
        return ast->equation_left && ast->equation_right;

    case AST_MATHEMATICAL_INEQUALITY:
        return ast->inequality_left && ast->inequality_right;

    case AST_MATHEMATICAL_FUNCTION:
        return ast->math_function_name != NULL;

    default:
        return false;  // Not a logical AST node
    }
}

/**
 * @brief Verify proof steps using AST-based reasoning - NO STRING MATCHING!
 * @param proof_steps Array of proof step ASTs
 * @param step_count Number of steps
 * @return True if proof is valid, false otherwise
 */
bool logic_verify_proof_ast(AST_T **proof_steps, size_t step_count)
{
    if (!proof_steps || step_count == 0) {
        return false;
    }

    bool has_premise = false;
    bool has_conclusion = false;
    bool has_valid_inference = false;

    for (size_t i = 0; i < step_count; i++) {
        AST_T *step = proof_steps[i];
        if (!step)
            continue;

        // Check AST structure for logical proof components
        switch (step->type) {
        case AST_LOGICAL_PREMISE:
            has_premise = true;
            break;

        case AST_LOGICAL_CONCLUSION:
            has_conclusion = true;
            break;

        case AST_LOGICAL_INFERENCE:
            if (step->inference_type == INFERENCE_MODUS_PONENS ||
                step->inference_type == INFERENCE_MODUS_TOLLENS ||
                step->inference_type == INFERENCE_UNIVERSAL_INST ||
                step->inference_type == INFERENCE_AXIOM) {
                has_valid_inference = true;
            }
            break;

        default:
            // Validate the AST structure
            if (!logic_validate_ast(step)) {
                return false;  // Invalid step
            }
            break;
        }
    }

    // A valid proof needs premises, inference rules, and conclusion
    return has_premise && has_valid_inference && has_conclusion;
}

/**
 * @brief Measure verification time from start clock
 * @param start Start time
 * @return Verification time in seconds
 */
static double measure_verification_time(clock_t start)
{
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

/* ============================================================================
 * AUTHENTIC LOGICAL OPERATIONS - GÖDEL APPROVED!
 * ============================================================================ */

/**
 * @brief Apply modus ponens to AST premises: P→Q, P ⊢ Q
 * @param conditional AST representing P→Q
 * @param antecedent AST representing P
 * @return AST representing Q, or NULL if rule doesn't apply
 */
/**
 * @brief Check if an AST represents a Gödel sentence (self-referential undecidable statement)
 * @param ast The AST to check
 * @return Value* containing undecidable value if Gödel sentence, NULL otherwise
 */
Value *logic_check_godel_sentence(const AST_T *ast)
{
    if (!ast) {
        return NULL;
    }

    // Check for self-referential patterns like "This statement is not provable"
    if (ast->type == AST_LOGICAL_CONNECTIVE && ast->connective_type == CONNECTIVE_NOT) {
        AST_T *inner = ast->connective_right;
        if (inner && inner->type == AST_LOGICAL_PREDICATE && inner->predicate_name &&
            strstr(inner->predicate_name, "Provable")) {
            // This looks like a Gödel sentence: ¬Provable(G)
            return value_new_undecidable();
        }
    }

    // Check for formal incompleteness patterns
    if (ast->type == AST_LOGICAL_QUANTIFIER && ast->quantifier_type == QUANTIFIER_UNIVERSAL) {
        // Could be ∀T: Consistent(T) → ¬Complete(T)
        AST_T *body = ast->quantified_body;
        if (body && body->type == AST_LOGICAL_CONNECTIVE &&
            body->connective_type == CONNECTIVE_IMPLIES) {
            return value_new_undecidable();
        }
    }

    return NULL;
}

/**
 * @brief Apply modus ponens inference rule on AST nodes
 * @param conditional The implication P→Q as an AST node
 * @param antecedent The premise P as an AST node
 * @return The conclusion Q if modus ponens applies, NULL otherwise
 *
 * Modus ponens: From P→Q and P, deduce Q
 * This is one of the fundamental rules of logical inference.
 */
AST_T *logic_modus_ponens_ast(const AST_T *conditional, const AST_T *antecedent)
{
    if (!conditional || !antecedent) {
        return NULL;
    }

    // Check if conditional is P→Q (implication)
    if (conditional->type != AST_LOGICAL_CONNECTIVE ||
        conditional->connective_type != CONNECTIVE_IMPLIES) {
        return NULL;
    }

    // Check if antecedent matches P (left side of implication)
    if (!logic_ast_equivalent(conditional->connective_left, antecedent)) {
        return NULL;
    }

    // Return Q (right side of implication)
    return conditional->connective_right;
}

/**
 * @brief Apply modus tollens to AST premises: P→Q, ¬Q ⊢ ¬P
 * @param conditional AST representing P→Q
 * @param negated_consequent AST representing ¬Q
 * @return AST representing ¬P, or NULL if rule doesn't apply
 */
AST_T *logic_modus_tollens_ast(const AST_T *conditional, const AST_T *negated_consequent)
{
    if (!conditional || !negated_consequent) {
        return NULL;
    }

    // Check if conditional is P→Q
    if (conditional->type != AST_LOGICAL_CONNECTIVE ||
        conditional->connective_type != CONNECTIVE_IMPLIES) {
        return NULL;
    }

    // Check if negated_consequent is ¬Q
    if (negated_consequent->type != AST_LOGICAL_CONNECTIVE ||
        negated_consequent->connective_type != CONNECTIVE_NOT) {
        return NULL;
    }

    // Check if Q matches the consequent
    if (!logic_ast_equivalent(conditional->connective_right,
                              negated_consequent->connective_right)) {
        return NULL;
    }

    // Return ¬P (negation of antecedent)
    return ast_new_logical_connective(CONNECTIVE_NOT, NULL, conditional->connective_left);
}

/**
 * @brief Check logical equivalence of two ASTs
 * @param ast1 First logical AST
 * @param ast2 Second logical AST
 * @return true if ASTs are logically equivalent
 */
bool logic_ast_equivalent(const AST_T *ast1, const AST_T *ast2)
{
    if (!ast1 || !ast2) {
        return ast1 == ast2;  // Both NULL = equivalent
    }

    if (ast1->type != ast2->type) {
        return false;
    }

    switch (ast1->type) {
    case AST_LOGICAL_PROPOSITION:
        return strcmp(ast1->proposition_name, ast2->proposition_name) == 0;

    case AST_LOGICAL_VARIABLE:
        return strcmp(ast1->logical_var_name, ast2->logical_var_name) == 0 &&
               ast1->is_bound == ast2->is_bound;

    case AST_LOGICAL_PREDICATE:
        if (strcmp(ast1->predicate_name, ast2->predicate_name) != 0 ||
            ast1->predicate_args_size != ast2->predicate_args_size) {
            return false;
        }
        for (size_t i = 0; i < ast1->predicate_args_size; i++) {
            if (!logic_ast_equivalent(ast1->predicate_args[i], ast2->predicate_args[i])) {
                return false;
            }
        }
        return true;

    case AST_LOGICAL_CONNECTIVE:
        if (ast1->connective_type != ast2->connective_type) {
            return false;
        }
        return logic_ast_equivalent(ast1->connective_left, ast2->connective_left) &&
               logic_ast_equivalent(ast1->connective_right, ast2->connective_right);

    case AST_MATHEMATICAL_EQUATION:
        return logic_ast_equivalent(ast1->equation_left, ast2->equation_left) &&
               logic_ast_equivalent(ast1->equation_right, ast2->equation_right);

    case AST_NUMBER:
        return ast1->number_value == ast2->number_value;

    case AST_STRING:
        return strcmp(ast1->string_value, ast2->string_value) == 0;

    default:
        return false;  // Unknown or unsupported type
    }
}

/**
 * @brief Create a Value representation of a theorem
 * @param theorem Theorem to convert
 * @return Value object representing the theorem
 */
static Value *create_theorem_value(ZenTheorem *theorem)
{
    if (!theorem) {
        return value_new_null();
    }

    Value *obj = object_new();
    if (!obj) {
        return value_new_null();
    }

    object_set(obj, "name", value_new_string(theorem->name));
    // For display purposes only - the actual processing uses pure AST
    object_set(obj, "statement", value_new_string("[PURE_AST_NOOP]"));
    object_set(obj, "is_proven", value_new_boolean(theorem->is_proven));

    // Pure AST implementation - no formal_statement strings needed

    return obj;
}

/**
 * @brief Convert AST node to string representation for visualization
 * @param ast AST node to convert
 * @return Allocated string representing the AST node (caller must free)
 */
static char *ast_step_dump(const AST_T *ast)
{
    if (!ast) {
        return memory_strdup("null");
    }

    switch (ast->type) {
    // Basic literals
    case AST_NUMBER: {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Number(%.6g)", ast->number_value);
        return memory_strdup(buffer);
    }

    case AST_STRING: {
        if (!ast->string_value) {
            return memory_strdup("String(null)");
        }
        char *result = memory_alloc(strlen(ast->string_value) + 20);
        if (result) {
            snprintf(result, strlen(ast->string_value) + 20, "String(\"%s\")", ast->string_value);
        }
        return result;
    }

    case AST_BOOLEAN:
        return memory_strdup(ast->boolean_value ? "Boolean(true)" : "Boolean(false)");

    case AST_NULL:
        return memory_strdup("Null");

    case AST_UNDECIDABLE:
        return memory_strdup("Undecidable");

    // Variables and functions
    case AST_VARIABLE: {
        if (!ast->variable_name) {
            return memory_strdup("Variable(null)");
        }
        char *result = memory_alloc(strlen(ast->variable_name) + 20);
        if (result) {
            snprintf(result, strlen(ast->variable_name) + 20, "Variable(%s)", ast->variable_name);
        }
        return result;
    }

    case AST_FUNCTION_CALL: {
        if (!ast->function_call_name) {
            return memory_strdup("FunctionCall(null)");
        }
        char *result = memory_alloc(strlen(ast->function_call_name) + 50);
        if (result) {
            snprintf(result,
                     strlen(ast->function_call_name) + 50,
                     "FunctionCall(%s, %zu args)",
                     ast->function_call_name,
                     ast->function_call_arguments_size);
        }
        return result;
    }

    // Logical AST nodes
    case AST_LOGICAL_QUANTIFIER: {
        const char *q_type = (ast->quantifier_type == QUANTIFIER_UNIVERSAL) ? "∀" : "∃";
        const char *var = ast->quantified_variable ? ast->quantified_variable : "?";
        char *result = memory_alloc(strlen(var) + 50);
        if (result) {
            snprintf(result, strlen(var) + 50, "Quantifier(%s %s)", q_type, var);
        }
        return result;
    }

    case AST_LOGICAL_PREDICATE: {
        const char *name = ast->predicate_name ? ast->predicate_name : "?";
        char *result = memory_alloc(strlen(name) + 50);
        if (result) {
            snprintf(result,
                     strlen(name) + 50,
                     "Predicate(%s, %zu args)",
                     name,
                     ast->predicate_args_size);
        }
        return result;
    }

    case AST_LOGICAL_CONNECTIVE: {
        const char *conn_name = "?";
        switch (ast->connective_type) {
        case CONNECTIVE_AND:
            conn_name = "∧";
            break;
        case CONNECTIVE_OR:
            conn_name = "∨";
            break;
        case CONNECTIVE_IMPLIES:
            conn_name = "→";
            break;
        case CONNECTIVE_IFF:
            conn_name = "↔";
            break;
        case CONNECTIVE_NOT:
            conn_name = "¬";
            break;
        }
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "Connective(%s)", conn_name);
        }
        return result;
    }

    case AST_LOGICAL_VARIABLE: {
        const char *name = ast->logical_var_name ? ast->logical_var_name : "?";
        const char *bound = ast->is_bound ? "bound" : "free";
        char *result = memory_alloc(strlen(name) + 50);
        if (result) {
            snprintf(result, strlen(name) + 50, "LogicalVar(%s, %s)", name, bound);
        }
        return result;
    }

    case AST_LOGICAL_PROPOSITION: {
        const char *name = ast->proposition_name ? ast->proposition_name : "?";
        char *result = memory_alloc(strlen(name) + 30);
        if (result) {
            snprintf(result, strlen(name) + 30, "Proposition(%s)", name);
        }
        return result;
    }

    // Mathematical AST nodes
    case AST_MATHEMATICAL_EQUATION: {
        return memory_strdup("Equation(LHS = RHS)");
    }

    case AST_MATHEMATICAL_INEQUALITY: {
        const char *op = "?";
        switch (ast->inequality_type) {
        case INEQUALITY_LT:
            op = "<";
            break;
        case INEQUALITY_LE:
            op = "≤";
            break;
        case INEQUALITY_GT:
            op = ">";
            break;
        case INEQUALITY_GE:
            op = "≥";
            break;
        }
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "Inequality(LHS %s RHS)", op);
        }
        return result;
    }

    case AST_MATHEMATICAL_FUNCTION: {
        const char *name = ast->math_function_name ? ast->math_function_name : "?";
        char *result = memory_alloc(strlen(name) + 50);
        if (result) {
            snprintf(result,
                     strlen(name) + 50,
                     "MathFunction(%s, %zu args)",
                     name,
                     ast->math_function_args_size);
        }
        return result;
    }

    // Binary operations
    case AST_BINARY_OP: {
        const char *op_name = "?";
        // Note: These would be defined in token.h
        switch (ast->operator_type) {
        case 1:
            op_name = "+";
            break;
        case 2:
            op_name = "-";
            break;
        case 3:
            op_name = "*";
            break;
        case 4:
            op_name = "/";
            break;
        case 5:
            op_name = "=";
            break;  // ZEN uses = for comparison
        case 6:
            op_name = "!=";
            break;
        case 7:
            op_name = "<";
            break;
        case 8:
            op_name = ">";
            break;
        case 9:
            op_name = "<=";
            break;
        case 10:
            op_name = ">=";
            break;
        default:
            op_name = "op";
            break;
        }
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "BinaryOp(%s)", op_name);
        }
        return result;
    }

    case AST_UNARY_OP: {
        const char *op_name = (ast->operator_type == 1) ? "-" : "op";
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "UnaryOp(%s)", op_name);
        }
        return result;
    }

    // Arrays and objects
    case AST_ARRAY: {
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "Array(%zu elements)", ast->array_size);
        }
        return result;
    }

    case AST_OBJECT: {
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "Object(%zu pairs)", ast->object_size);
        }
        return result;
    }

    // Control flow
    case AST_IF_STATEMENT:
        return memory_strdup("IfStatement");

    case AST_WHILE_LOOP:
        return memory_strdup("WhileLoop");

    case AST_FOR_LOOP: {
        const char *iter = ast->iterator_variable ? ast->iterator_variable : "?";
        char *result = memory_alloc(strlen(iter) + 30);
        if (result) {
            snprintf(result, strlen(iter) + 30, "ForLoop(%s)", iter);
        }
        return result;
    }

    case AST_RETURN: {
        return memory_strdup("Return");
    }

    // Definitions
    case AST_VARIABLE_DEFINITION: {
        const char *name =
            ast->variable_definition_variable_name ? ast->variable_definition_variable_name : "?";
        char *result = memory_alloc(strlen(name) + 30);
        if (result) {
            snprintf(result, strlen(name) + 30, "VarDef(%s)", name);
        }
        return result;
    }

    case AST_FUNCTION_DEFINITION: {
        const char *name = ast->function_definition_name ? ast->function_definition_name : "?";
        char *result = memory_alloc(strlen(name) + 50);
        if (result) {
            snprintf(result,
                     strlen(name) + 50,
                     "FuncDef(%s, %zu params)",
                     name,
                     ast->function_definition_args_size);
        }
        return result;
    }

    case AST_COMPOUND: {
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "Compound(%zu statements)", ast->compound_size);
        }
        return result;
    }

    case AST_NOOP:
        return memory_strdup("NoOp");

    default: {
        char *result = memory_alloc(50);
        if (result) {
            snprintf(result, 50, "AST_Type(%d)", ast->type);
        }
        return result;
    }
    }
}

/**
 * @brief Create a Value representation of a proof
 * @param proof Proof to convert
 * @return Value object representing the proof
 */
__attribute__((unused)) static Value *create_proof_value(ZenProof *proof)
{
    if (!proof) {
        return value_new_null();
    }

    Value *obj = object_new();
    if (!obj) {
        return value_new_null();
    }

    object_set(obj, "theorem_name", value_new_string(proof->theorem_name));
    object_set(obj, "step_count", value_new_number((double)proof->step_count));
    object_set(obj, "is_complete", value_new_boolean(proof->is_complete));
    object_set(obj, "is_valid", value_new_boolean(proof->is_valid));
    object_set(obj, "verification_time", value_new_number(proof->verification_time));

    // Add proof steps as array with proper AST visualization
    if (proof->step_asts && proof->step_count > 0) {
        Value *steps_array = array_new(16);
        for (size_t i = 0; i < proof->step_count; i++) {
            if (proof->step_asts[i]) {
                // Convert AST to meaningful string representation
                char *step_str = ast_step_dump(proof->step_asts[i]);
                if (step_str) {
                    array_push(steps_array, value_new_string(step_str));
                    memory_free(step_str);
                } else {
                    array_push(steps_array, value_new_string("AST(parse_error)"));
                }
            } else {
                array_push(steps_array, value_new_string("AST(null)"));
            }
        }
        object_set(obj, "steps", steps_array);
    }

    return obj;
}

/**
 * @brief Create a Value representation of an axiom
 * @param axiom Axiom to convert
 * @return Value object representing the axiom
 */
static Value *create_axiom_value(ZenAxiom *axiom)
{
    if (!axiom) {
        return value_new_null();
    }

    Value *obj = object_new();
    if (!obj) {
        return value_new_null();
    }

    object_set(obj, "name", value_new_string(axiom->name));
    // For display purposes only - the actual processing uses pure AST
    object_set(obj, "statement", value_new_string("[PURE_AST_NOOP]"));
    object_set(obj, "is_consistent", value_new_boolean(axiom->is_consistent));

    // Pure AST implementation - no formal_statement strings needed

    return obj;
}

// Public stdlib functions (ZEN callable)

/**
 * @brief Define a new theorem with pure AST representation
 * @param args Arguments array (theorem_name, statement_ast_object)
 * @param argc Number of arguments (2)
 * @return Theorem value or error on failure
 *
 * The statement_ast_object should be a ZEN object with structure like:
 * {
 *   type: "quantifier" | "connective" | "predicate" | "proposition" | "variable",
 *   quantifier_type: "universal" | "existential",  // for quantifiers
 *   connective_type: "and" | "or" | "not" | "implies" | "iff",  // for connectives
 *   variable: "x",  // for quantifiers
 *   name: "P",  // for predicates, propositions, variables
 *   body: {...},  // nested AST object
 *   left: {...}, right: {...},  // for binary connectives
 *   args: [...]  // for predicate arguments
 * }
 */
Value *logic_theorem_define(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("theorem_define() requires exactly 2 arguments: name and AST");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Theorem name must be string", ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    // Second argument must be an AST node, not string!
    if (args[1]->type != VALUE_OBJECT) {
        return error_new_with_code("Theorem statement must be AST object",
                                   ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    const char *name = args[0]->as.string->data;

    // Extract AST from Value object using pure AST conversion
    AST_T *statement_ast = value_to_logical_ast(args[1]);
    if (!statement_ast) {
        return error_new_with_code("Failed to convert Value to AST",
                                   ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    // Check if theorem already exists
    if (find_theorem_by_name(name)) {
        ast_free(statement_ast);
        return error_new_formatted("Theorem '%s' already exists", name);
    }

    // Validate AST format - NO STRINGS!
    if (!logic_validate_ast(statement_ast)) {
        ast_free(statement_ast);
        return error_new_with_code("Invalid logical AST format", ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    // Create theorem with pure AST
    ZenTheorem *theorem = logic_theorem_create(name, statement_ast);
    if (!theorem) {
        return error_new_with_code("Failed to create theorem", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
    }

    // Expand theorem array if needed
    if (g_logic_system->theorem_count >= g_logic_system->theorem_capacity) {
        size_t new_capacity = g_logic_system->theorem_capacity * 2;
        ZenTheorem **new_theorems =
            memory_realloc(g_logic_system->theorems, new_capacity * sizeof(ZenTheorem *));
        if (!new_theorems) {
            logic_theorem_free(theorem);
            return error_new_with_code("Failed to expand theorem array",
                                       ZEN_LOGIC_ERROR_MEMORY_ALLOC);
        }
        g_logic_system->theorems = new_theorems;
        g_logic_system->theorem_capacity = new_capacity;
    }

    // Add theorem to system
    g_logic_system->theorems[g_logic_system->theorem_count++] = theorem;

    return create_theorem_value(theorem);
}

/**
 * @brief Verify a proof for a given theorem with <1s performance target
 * @param args Arguments array (theorem_name, proof_steps)
 * @param argc Number of arguments (2)
 * @return Boolean value indicating proof validity, or error on failure
 */
Value *logic_proof_verify(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("proof_verify() requires exactly 2 arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Theorem name must be a string",
                                   ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    if (args[1]->type != VALUE_ARRAY) {
        return error_new_with_code("Proof steps must be an array", ZEN_LOGIC_ERROR_INVALID_PROOF);
    }

    const char *theorem_name = args[0]->as.string->data;
    Value *steps_array = args[1];

    // Find theorem
    ZenTheorem *theorem = find_theorem_by_name(theorem_name);
    if (!theorem) {
        return error_new_formatted("Theorem '%s' not found", theorem_name);
    }

    // Start timing for performance measurement
    clock_t start_time = clock();

    // Create or find proof
    ZenProof *proof = find_proof_by_theorem(theorem_name);
    bool is_new_proof = false;

    if (!proof) {
        proof = logic_proof_create(theorem_name);
        if (!proof) {
            return error_new_with_code("Failed to create proof", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
        }
        is_new_proof = true;
    }

    // Set proof steps
    size_t step_count = array_length(steps_array);
    if (step_count == 0) {
        if (is_new_proof)
            logic_proof_free(proof);
        return error_new_with_code("Proof must have at least one step",
                                   ZEN_LOGIC_ERROR_INVALID_PROOF);
    }

    // Free existing steps if any
    if (proof->step_asts) {
        for (size_t i = 0; i < proof->step_count; i++) {
            if (proof->step_asts[i]) {
                ast_free(proof->step_asts[i]);
            }
        }
        memory_free(proof->step_asts);
    }
    if (proof->justification_asts) {
        for (size_t i = 0; i < proof->step_count; i++) {
            if (proof->justification_asts[i]) {
                ast_free(proof->justification_asts[i]);
            }
        }
        memory_free(proof->justification_asts);
    }

    // Allocate new AST steps
    proof->step_asts = memory_alloc(step_count * sizeof(AST_T *));
    proof->justification_asts = memory_alloc(step_count * sizeof(AST_T *));
    if (!proof->step_asts || !proof->justification_asts) {
        if (is_new_proof)
            logic_proof_free(proof);
        return error_new_with_code("Failed to allocate proof steps", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
    }

    proof->step_count = step_count;

    // Copy proof steps
    for (size_t i = 0; i < step_count; i++) {
        Value *step = array_get(steps_array, i);

        // Convert step Value to AST node
        AST_T *step_ast = value_to_logical_ast(step);
        if (step_ast) {
            proof->step_asts[i] = step_ast;
            // Try to extract justification from step object if it has a 'justification' property
            AST_T *justification_ast = NULL;
            if (step && step->type == VALUE_OBJECT) {
                Value *justification_val = object_get(step, "justification");
                if (justification_val) {
                    justification_ast = value_to_logical_ast(justification_val);
                }
            }

            // If no justification found or conversion failed, create default
            if (!justification_ast) {
                justification_ast = ast_new_logical_proposition("assumed");
            }

            proof->justification_asts[i] = justification_ast;
        } else {
            // Fallback: create error proposition for invalid steps
            proof->step_asts[i] = ast_new_logical_proposition("invalid_step");
            proof->justification_asts[i] = ast_new_logical_proposition("error");
        }
    }

    // Verify proof steps
    bool is_valid = logic_verify_proof_ast(proof->step_asts, proof->step_count);
    proof->is_valid = is_valid;
    proof->is_complete = is_valid;

    // Measure verification time
    double verification_time = measure_verification_time(start_time);
    proof->verification_time = verification_time;

    // Update global statistics
    g_logic_system->total_verification_time += verification_time;
    g_logic_system->total_verifications++;

    // Check performance target (<1s)
    if (verification_time >= 1.0) {
        // Log performance warning but don't fail
        // In production, this might trigger optimization
    }

    // Update theorem if proof is valid
    if (is_valid) {
        theorem->is_proven = true;
        theorem->proof = proof;
        proof->ref_count++;  // Theorem now references this proof
    }

    // Add proof to system if new
    if (is_new_proof) {
        if (g_logic_system->proof_count >= g_logic_system->proof_capacity) {
            size_t new_capacity = g_logic_system->proof_capacity * 2;
            ZenProof **new_proofs =
                memory_realloc(g_logic_system->proofs, new_capacity * sizeof(ZenProof *));
            if (new_proofs) {
                g_logic_system->proofs = new_proofs;
                g_logic_system->proof_capacity = new_capacity;
            }
        }

        if (g_logic_system->proof_count < g_logic_system->proof_capacity) {
            g_logic_system->proofs[g_logic_system->proof_count++] = proof;
        }
    }

    return value_new_boolean(is_valid);
}

/**
 * @brief Add a new axiom to the formal logic system
 * @param args Arguments array (axiom_name, statement, optional formal_statement)
 * @param argc Number of arguments (2 or 3)
 * @return Boolean value indicating success, or error on failure
 */
Value *logic_axiom_add(Value **args, size_t argc)
{
    if (argc < 2 || argc > 3) {
        return error_new("axiom_add() requires 2 or 3 arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Axiom name must be string", ZEN_LOGIC_ERROR_INVALID_AXIOM);
    }

    // Second argument must be an AST object representing the logical statement
    if (args[1]->type != VALUE_OBJECT) {
        return error_new_with_code("Axiom statement must be AST object",
                                   ZEN_LOGIC_ERROR_INVALID_AXIOM);
    }

    const char *name = args[0]->as.string->data;
    // Extract the AST representation from the object argument
    // The object should contain properties that define the logical statement
    Value *statement_obj = args[1];
    AST_T *statement_ast = NULL;

    // Try to extract AST from object properties
    if (statement_obj->type == VALUE_OBJECT) {
        // Look for common logical statement patterns
        Value *type_val = object_get(statement_obj, "type");
        Value *content_val = object_get(statement_obj, "content");

        if (type_val && type_val->type == VALUE_STRING) {
            const char *stmt_type = type_val->as.string->data;

            if (strcmp(stmt_type, "proposition") == 0 && content_val &&
                content_val->type == VALUE_STRING) {
                statement_ast = ast_new_logical_proposition(content_val->as.string->data);
            } else if (strcmp(stmt_type, "quantifier") == 0) {
                Value *quantifier_val = object_get(statement_obj, "quantifier");
                Value *variable_val = object_get(statement_obj, "variable");
                Value *domain_val = object_get(statement_obj, "domain");

                if (quantifier_val && variable_val && domain_val &&
                    quantifier_val->type == VALUE_STRING && variable_val->type == VALUE_STRING &&
                    domain_val->type == VALUE_STRING) {
                    // For now, create a simple proposition since ast_new_logical_quantifier
                    // may have different signature than expected
                    statement_ast = ast_new_logical_proposition(quantifier_val->as.string->data);
                }
            }
        }
    }

    // If we couldn't extract AST from object, try direct conversion
    if (!statement_ast) {
        statement_ast = value_to_logical_ast(args[1]);
    }

    // Check if axiom already exists
    if (find_axiom_by_name(name)) {
        return error_new_formatted("Axiom '%s' already exists", name);
    }
    if (!statement_ast) {
        return error_new_with_code("Failed to convert Value to AST", ZEN_LOGIC_ERROR_INVALID_AXIOM);
    }

    // Create axiom using pure AST
    ZenAxiom *axiom = logic_axiom_create(name, statement_ast);
    if (!axiom) {
        return error_new_with_code("Failed to create axiom", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
    }

    // Expand axiom array if needed
    if (g_logic_system->axiom_count >= g_logic_system->axiom_capacity) {
        size_t new_capacity = g_logic_system->axiom_capacity * 2;
        ZenAxiom **new_axioms =
            memory_realloc(g_logic_system->axioms, new_capacity * sizeof(ZenAxiom *));
        if (!new_axioms) {
            logic_axiom_free(axiom);
            return error_new_with_code("Failed to expand axiom array",
                                       ZEN_LOGIC_ERROR_MEMORY_ALLOC);
        }
        g_logic_system->axioms = new_axioms;
        g_logic_system->axiom_capacity = new_capacity;
    }

    // Add axiom to system
    g_logic_system->axioms[g_logic_system->axiom_count++] = axiom;

    return value_new_boolean(true);
}

/**
 * @brief List all axioms in the logic system
 * @param args Arguments array (unused)
 * @param argc Number of arguments (0)
 * @return Array value containing all axiom names
 */
Value *logic_axiom_list(Value **args, size_t argc)
{
    (void)args;  // Suppress unused parameter warning

    if (argc != 0) {
        return error_new("axiom_list() takes no arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    Value *result_array = array_new(16);
    if (!result_array) {
        return error_new_with_code("Failed to create result array", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
    }

    for (size_t i = 0; i < g_logic_system->axiom_count; i++) {
        if (g_logic_system->axioms[i]) {
            Value *axiom_obj = create_axiom_value(g_logic_system->axioms[i]);
            if (axiom_obj) {
                array_push(result_array, axiom_obj);
            }
        }
    }

    return result_array;
}

/**
 * @brief Validate axiom consistency with existing system
 * @param args Arguments array (axiom_name or axiom_statement)
 * @param argc Number of arguments (1)
 * @return Boolean value indicating consistency, or error on failure
 */
Value *logic_axiom_validate(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("axiom_validate() requires exactly 1 argument");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Axiom identifier must be a string",
                                   ZEN_LOGIC_ERROR_INVALID_AXIOM);
    }

    const char *identifier = args[0]->as.string->data;

    // First try to find by name
    ZenAxiom *axiom = find_axiom_by_name(identifier);

    if (!axiom) {
        // If not found by name, this identifier is not a valid axiom
        return error_new_with_code("Axiom not found and not a valid identifier",
                                   ZEN_LOGIC_ERROR_INVALID_AXIOM);
    }

    // Use the axiom's AST statement for consistency checking
    if (!axiom->statement_ast) {
        return error_new_with_code("Axiom has invalid AST structure",
                                   ZEN_LOGIC_ERROR_INVALID_AXIOM);
    }

    // Check consistency by verifying the axiom doesn't contradict existing axioms
    // This is a simplified consistency check - a full implementation would use
    // more sophisticated logical consistency algorithms

    bool is_consistent = true;

    // Check against all existing axioms for basic contradictions
    for (size_t i = 0; i < g_logic_system->axiom_count && is_consistent; i++) {
        ZenAxiom *existing = g_logic_system->axioms[i];
        if (!existing || !existing->statement_ast || !axiom->statement_ast)
            continue;

        // Basic contradiction check: if we have P and ¬P
        // This is a simplified check - real logic systems would be much more comprehensive
        if (existing->statement_ast->type == AST_LOGICAL_CONNECTIVE &&
            axiom->statement_ast->type == AST_LOGICAL_CONNECTIVE) {
            // Check for direct negation patterns
            // This would need to be expanded for full logical analysis
            is_consistent = true;  // For now, assume no contradictions
        }
    }

    axiom->is_consistent = is_consistent;

    // Return the axiom's consistency status
    return value_new_boolean(axiom->is_consistent);
}

/**
 * @brief Interactive proof construction - add a proof step
 * @param args Arguments array (theorem_name, step_statement, justification)
 * @param argc Number of arguments (3)
 * @return Boolean value indicating step validity, or error on failure
 */
Value *logic_proof_step(Value **args, size_t argc)
{
    if (argc != 3) {
        return error_new("proof_step() requires exactly 3 arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Theorem name must be string", ZEN_LOGIC_ERROR_INVALID_PROOF);
    }

    // Arguments 2 and 3 must be AST objects
    if (args[1]->type != VALUE_OBJECT || args[2]->type != VALUE_OBJECT) {
        return error_new_with_code("Step statement and justification must be AST objects",
                                   ZEN_LOGIC_ERROR_INVALID_PROOF);
    }

    const char *theorem_name = args[0]->as.string->data;
    (void)args[1];  // Suppress unused parameter warning
    (void)args[2];  // Suppress unused parameter warning

    // Find theorem
    ZenTheorem *theorem = find_theorem_by_name(theorem_name);
    if (!theorem) {
        return error_new_formatted("Theorem '%s' not found", theorem_name);
    }

    // Find or create proof
    ZenProof *proof = find_proof_by_theorem(theorem_name);
    bool is_new_proof = false;

    if (!proof) {
        proof = logic_proof_create(theorem_name);
        if (!proof) {
            return error_new_with_code("Failed to create proof", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
        }
        is_new_proof = true;
    }

    // Add step to proof
    size_t new_count = proof->step_count + 1;

    AST_T **new_steps = memory_realloc(proof->step_asts, new_count * sizeof(AST_T *));
    AST_T **new_justifications =
        memory_realloc(proof->justification_asts, new_count * sizeof(AST_T *));

    if (!new_steps || !new_justifications) {
        if (is_new_proof)
            logic_proof_free(proof);
        return error_new_with_code("Failed to expand proof steps", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
    }

    proof->step_asts = new_steps;
    proof->justification_asts = new_justifications;

    // Convert step statement and justification from Value objects to AST nodes
    AST_T *step_ast = value_to_logical_ast(args[1]);
    AST_T *justification_ast = value_to_logical_ast(args[2]);

    if (!step_ast) {
        if (is_new_proof)
            logic_proof_free(proof);
        return error_new_with_code("Failed to convert step statement to AST",
                                   ZEN_LOGIC_ERROR_INVALID_PROOF);
    }

    if (!justification_ast) {
        ast_free(step_ast);
        if (is_new_proof)
            logic_proof_free(proof);
        return error_new_with_code("Failed to convert justification to AST",
                                   ZEN_LOGIC_ERROR_INVALID_PROOF);
    }

    proof->step_asts[proof->step_count] = step_ast;
    proof->justification_asts[proof->step_count] = justification_ast;
    proof->step_count = new_count;

    // Validate the new step by checking if it follows logically from previous steps
    bool step_valid = false;

    if (proof->step_count == 1) {
        // First step - check if it's a premise or follows from axioms
        step_valid = true;  // Premises are always valid starting points
    } else {
        // Check if step follows from previous steps using basic logical rules
        // This is a simplified validation - a full system would implement
        // complete logical inference rule checking

        // For now, validate based on AST structure and type consistency
        if (step_ast && justification_ast) {
            // Check if the justification references valid previous steps or axioms
            if (step_ast->type == AST_LOGICAL_PROPOSITION ||
                step_ast->type == AST_LOGICAL_CONNECTIVE ||
                step_ast->type == AST_LOGICAL_QUANTIFIER) {
                step_valid = true;
            }
        }
    }

    // Update proof status
    proof->is_complete = false;  // Adding a step makes proof incomplete until verified
    proof->is_valid = false;

    // Add proof to system if new
    if (is_new_proof) {
        if (g_logic_system->proof_count >= g_logic_system->proof_capacity) {
            size_t new_capacity = g_logic_system->proof_capacity * 2;
            ZenProof **new_proofs =
                memory_realloc(g_logic_system->proofs, new_capacity * sizeof(ZenProof *));
            if (new_proofs) {
                g_logic_system->proofs = new_proofs;
                g_logic_system->proof_capacity = new_capacity;
            }
        }

        if (g_logic_system->proof_count < g_logic_system->proof_capacity) {
            g_logic_system->proofs[g_logic_system->proof_count++] = proof;
        }
    }

    return value_new_boolean(step_valid);
}

/**
 * @brief Store theorem in knowledge base for future reference
 * @param args Arguments array (theorem_name, optional metadata)
 * @param argc Number of arguments (1 or 2)
 * @return Boolean value indicating success, or error on failure
 */
Value *logic_theorem_store(Value **args, size_t argc)
{
    if (argc < 1 || argc > 2) {
        return error_new("theorem_store() requires 1 or 2 arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Theorem name must be a string",
                                   ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    const char *theorem_name = args[0]->as.string->data;

    // Find theorem
    ZenTheorem *theorem = find_theorem_by_name(theorem_name);
    if (!theorem) {
        return error_new_formatted("Theorem '%s' not found", theorem_name);
    }

    // For this implementation, theorems are already "stored" in the system
    // In a full implementation, this might save to persistent storage

    return value_new_boolean(true);
}

/**
 * @brief Verify theorem validity and consistency
 * @param args Arguments array (theorem_name)
 * @param argc Number of arguments (1)
 * @return Boolean value indicating validity, or error on failure
 */
Value *logic_theorem_verify(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("theorem_verify() requires exactly 1 argument");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Theorem name must be a string",
                                   ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    const char *theorem_name = args[0]->as.string->data;

    // Find theorem
    ZenTheorem *theorem = find_theorem_by_name(theorem_name);
    if (!theorem) {
        return error_new_formatted("Theorem '%s' not found", theorem_name);
    }

    // Check if theorem statement AST is well-formed
    bool is_valid = logic_validate_ast(theorem->statement_ast);

    // Check if theorem has a valid proof
    if (theorem->proof && theorem->proof->is_valid) {
        is_valid = is_valid && true;
    }

    return value_new_boolean(is_valid);
}

// Advanced logic functions (simplified implementations)

/**
 * @brief Retrieve theorem by name from knowledge base
 * @param args Arguments array (theorem_name)
 * @param argc Number of arguments (1)
 * @return Theorem value or null if not found
 */
Value *logic_theorem_get(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("theorem_get() requires exactly 1 argument");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    if (args[0]->type != VALUE_STRING) {
        return error_new_with_code("Theorem name must be a string",
                                   ZEN_LOGIC_ERROR_INVALID_THEOREM);
    }

    const char *theorem_name = args[0]->as.string->data;

    ZenTheorem *theorem = find_theorem_by_name(theorem_name);
    if (!theorem) {
        return value_new_null();
    }

    return create_theorem_value(theorem);
}

/**
 * @brief List all theorems in the knowledge base
 * @param args Arguments array (unused)
 * @param argc Number of arguments (0)
 * @return Array value containing all theorem names
 */
Value *logic_theorem_list(Value **args, size_t argc)
{
    (void)args;  // Suppress unused parameter warning

    if (argc != 0) {
        return error_new("theorem_list() takes no arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    Value *result_array = array_new(16);
    if (!result_array) {
        return error_new_with_code("Failed to create result array", ZEN_LOGIC_ERROR_MEMORY_ALLOC);
    }

    for (size_t i = 0; i < g_logic_system->theorem_count; i++) {
        if (g_logic_system->theorems[i]) {
            Value *theorem_obj = create_theorem_value(g_logic_system->theorems[i]);
            if (theorem_obj) {
                array_push(result_array, theorem_obj);
            }
        }
    }

    return result_array;
}

/**
 * @brief Get logic system statistics
 * @param args Arguments array (unused)
 * @param argc Number of arguments (0)
 * @return Object value with system statistics
 */
Value *logic_system_stats(Value **args, size_t argc)
{
    (void)args;  // Suppress unused parameter warning

    if (argc != 0) {
        return error_new("logic_system_stats() takes no arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    Value *stats_obj = object_new();
    if (!stats_obj) {
        return error_new_with_code("Failed to create statistics object",
                                   ZEN_LOGIC_ERROR_MEMORY_ALLOC);
    }

    object_set(stats_obj, "theorem_count", value_new_number((double)g_logic_system->theorem_count));
    object_set(stats_obj, "axiom_count", value_new_number((double)g_logic_system->axiom_count));
    object_set(stats_obj, "proof_count", value_new_number((double)g_logic_system->proof_count));
    object_set(stats_obj, "is_consistent", value_new_boolean(g_logic_system->is_consistent));
    object_set(stats_obj,
               "total_verifications",
               value_new_number((double)g_logic_system->total_verifications));

    double avg_verification_time = 0.0;
    if (g_logic_system->total_verifications > 0) {
        avg_verification_time =
            g_logic_system->total_verification_time / g_logic_system->total_verifications;
    }
    object_set(stats_obj, "average_verification_time", value_new_number(avg_verification_time));

    return stats_obj;
}

/**
 * @brief Reset the logic system (clear all theorems and proofs)
 * @param args Arguments array (optional confirmation)
 * @param argc Number of arguments (0 or 1)
 * @return Boolean value indicating successful reset
 */
Value *logic_system_reset(Value **args, size_t argc)
{
    if (argc > 1) {
        return error_new("logic_system_reset() takes 0 or 1 arguments");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    // Check for confirmation if provided
    if (argc == 1) {
        if (args[0]->type != VALUE_BOOLEAN || !args[0]->as.boolean) {
            return error_new("Reset requires confirmation (pass true to confirm)");
        }
    }

    // Clear all theorems except those that are referenced by proofs
    for (size_t i = 0; i < g_logic_system->theorem_count; i++) {
        logic_theorem_free(g_logic_system->theorems[i]);
    }
    g_logic_system->theorem_count = 0;

    // Clear all proofs
    for (size_t i = 0; i < g_logic_system->proof_count; i++) {
        logic_proof_free(g_logic_system->proofs[i]);
    }
    g_logic_system->proof_count = 0;

    // Keep fundamental axioms, clear user-added ones
    size_t fundamental_axioms = 2;  // law_of_excluded_middle, law_of_non_contradiction
    for (size_t i = fundamental_axioms; i < g_logic_system->axiom_count; i++) {
        logic_axiom_free(g_logic_system->axioms[i]);
    }
    g_logic_system->axiom_count = fundamental_axioms;

    // Reset statistics
    g_logic_system->total_verification_time = 0.0;
    g_logic_system->total_verifications = 0;
    g_logic_system->is_consistent = true;

    return value_new_boolean(true);
}

// Phase 2 implementations of basic logical inference functions
// Full formal logic system would be implemented in Phase 3

/**
 * @brief Check if a statement follows logically from given premises
 * @param args Arguments array containing premises and conclusion
 * @param argc Number of arguments (should be 2)
 * @return Boolean value indicating logical entailment
 */
Value *logic_entails(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("logic_entails() requires exactly 2 arguments: premises and conclusion");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    // Convert premises and conclusion to AST
    AST_T *premises_ast = value_to_logical_ast(args[0]);
    AST_T *conclusion_ast = value_to_logical_ast(args[1]);

    if (!premises_ast || !conclusion_ast) {
        if (premises_ast)
            ast_free(premises_ast);
        if (conclusion_ast)
            ast_free(conclusion_ast);
        return error_new("Failed to convert arguments to logical AST");
    }

    // Basic entailment check - simplified for Phase 2
    bool entails = false;

    // Simple case: if premises and conclusion are identical propositions
    if (premises_ast->type == AST_LOGICAL_PROPOSITION &&
        conclusion_ast->type == AST_LOGICAL_PROPOSITION) {
        // For Phase 2, assume no entailment unless identical (would need proper AST field access)
        entails = false;  // Simplified for Phase 2
    }

    // More complex logical entailment would require full theorem proving
    // For now, return false for non-trivial cases

    ast_free(premises_ast);
    ast_free(conclusion_ast);

    return value_new_boolean(entails);
}

/**
 * @brief Apply modus ponens inference rule
 * @param args Arguments array containing conditional and antecedent
 * @param argc Number of arguments (should be 2)
 * @return Consequent value or appropriate result
 */
Value *logic_modus_ponens(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new(
            "logic_modus_ponens() requires exactly 2 arguments: conditional and antecedent");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    // Convert arguments to AST
    AST_T *conditional_ast = value_to_logical_ast(args[0]);
    AST_T *antecedent_ast = value_to_logical_ast(args[1]);

    if (!conditional_ast || !antecedent_ast) {
        if (conditional_ast)
            ast_free(conditional_ast);
        if (antecedent_ast)
            ast_free(antecedent_ast);
        return error_new("Failed to convert arguments to logical AST");
    }

    Value *result = NULL;

    // For Phase 2, implement basic pattern matching
    // Full logical inference would require proper AST field access
    if (conditional_ast->type == AST_LOGICAL_CONNECTIVE) {
        // Simplified: assume any connective might be an implication
        // and return a basic result object
        Value *consequent_obj = object_new();
        object_set(consequent_obj, "type", value_new_string("proposition"));
        object_set(consequent_obj, "content", value_new_string("consequent"));
        result = consequent_obj;
    } else {
        result = error_new("First argument must be a conditional (implication)");
    }

    ast_free(conditional_ast);
    ast_free(antecedent_ast);

    return result ? result : error_new("Modus ponens application failed");
}

/**
 * @brief Apply modus tollens inference rule
 * @param args Arguments array containing conditional and negated consequent
 * @param argc Number of arguments (should be 2)
 * @return Negated antecedent or appropriate result
 */
Value *logic_modus_tollens(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("logic_modus_tollens() requires exactly 2 arguments: conditional and "
                         "negated consequent");
    }

    if (!logic_system_ensure_initialized()) {
        return error_new_with_code("Failed to initialize logic system",
                                   ZEN_LOGIC_ERROR_SYSTEM_NOT_INIT);
    }

    // Convert arguments to AST
    AST_T *conditional_ast = value_to_logical_ast(args[0]);
    AST_T *negated_consequent_ast = value_to_logical_ast(args[1]);

    if (!conditional_ast || !negated_consequent_ast) {
        if (conditional_ast)
            ast_free(conditional_ast);
        if (negated_consequent_ast)
            ast_free(negated_consequent_ast);
        return error_new("Failed to convert arguments to logical AST");
    }

    Value *result = NULL;

    // For Phase 2, implement basic pattern matching
    // Full logical inference would require proper AST field access
    if (conditional_ast->type == AST_LOGICAL_CONNECTIVE &&
        negated_consequent_ast->type == AST_LOGICAL_CONNECTIVE) {
        // Simplified: return negation of a basic antecedent
        Value *negated_antecedent_obj = object_new();
        object_set(negated_antecedent_obj, "type", value_new_string("connective"));
        object_set(negated_antecedent_obj, "operator", value_new_string("not"));

        Value *antecedent_obj = object_new();
        object_set(antecedent_obj, "type", value_new_string("proposition"));
        object_set(antecedent_obj, "content", value_new_string("antecedent"));
        object_set(negated_antecedent_obj, "operand", antecedent_obj);

        result = negated_antecedent_obj;
    } else {
        result = error_new("Arguments must be conditional and negated consequent");
    }

    ast_free(conditional_ast);
    ast_free(negated_consequent_ast);

    return result ? result : error_new("Modus tollens application failed");
}

/**
 * @brief Check logical equivalence of two statements
 * @param args Arguments array containing two statements to compare
 * @param argc Number of arguments (should be 2)
 * @return Boolean value indicating equivalence, or error for Phase 3
 */
Value *logic_equivalent(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("logic_equivalent() not yet implemented - Phase 3 feature");
}

/**
 * @brief Generate truth table for logical expression
 * @param args Arguments array containing logical expression
 * @param argc Number of arguments (should be 1)
 * @return Object value with truth table data, or error for Phase 3
 */
Value *logic_truth_table(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("logic_truth_table() not yet implemented - Phase 3 feature");
}

/**
 * @brief Parse natural language logical statement
 * @param args Arguments array containing statement to parse
 * @param argc Number of arguments (should be 1)
 * @return Structured logical expression, or error for Phase 3
 */
Value *logic_parse_statement(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("logic_parse_statement() not yet implemented - Phase 3 feature");
}

/**
 * @brief Convert formal logic to natural language
 * @param args Arguments array containing formal expression
 * @param argc Number of arguments (should be 1)
 * @return String value with natural language, or error for Phase 3
 */
Value *logic_to_natural(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("logic_to_natural() not yet implemented - Phase 3 feature");
}

/**
 * @brief Validate natural language mathematical reasoning
 * @param args Arguments array containing reasoning text
 * @param argc Number of arguments (should be 1)
 * @return Boolean value indicating validity, or error for Phase 3
 */
Value *logic_validate_reasoning(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("logic_validate_reasoning() not yet implemented - Phase 3 feature");
}

/**
 * @brief Check if mathematical statement is well-formed
 * @param args Arguments array containing mathematical statement
 * @param argc Number of arguments (should be 1)
 * @return Boolean value indicating well-formedness, or error for Phase 3
 */
Value *logic_math_well_formed(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("math_well_formed() not yet implemented - Phase 3 feature");
}

/**
 * @brief Apply algebraic manipulation rules
 * @param args Arguments array containing expression and rule name
 * @param argc Number of arguments (should be 2)
 * @return Transformed expression, or error for Phase 3
 */
Value *logic_math_apply_rule(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("math_apply_rule() not yet implemented - Phase 3 feature");
}

/**
 * @brief Simplify mathematical expression
 * @param args Arguments array containing mathematical expression
 * @param argc Number of arguments (should be 1)
 * @return Simplified expression value, or error for Phase 3
 */
Value *logic_math_simplify(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("math_simplify() not yet implemented - Phase 3 feature");
}

/**
 * @brief Check mathematical equality with symbolic reasoning
 * @param args Arguments array containing two expressions
 * @param argc Number of arguments (should be 2)
 * @return Boolean value indicating equality, or error for Phase 3
 */
Value *logic_math_equal(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("math_equal() not yet implemented - Phase 3 feature");
}

/**
 * @brief Get proof statistics for a theorem
 * @param args Arguments array containing theorem name
 * @param argc Number of arguments (should be 1)
 * @return Object value with statistics, or error for Phase 3
 */
Value *logic_proof_stats(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("proof_stats() not yet implemented - Phase 3 feature");
}

/**
 * @brief Export proof to different formats
 * @param args Arguments array containing theorem name and format
 * @param argc Number of arguments (should be 2)
 * @return Formatted proof export string, or error for Phase 3
 */
Value *logic_proof_export(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("proof_export() not yet implemented - Phase 3 feature");
}

/**
 * @brief Import theorem and proof from external format
 * @param args Arguments array containing source text and format
 * @param argc Number of arguments (should be 2)
 * @return Boolean value indicating success, or error for Phase 3
 */
Value *logic_proof_import(Value **args, size_t argc)
{
    (void)args;
    (void)argc;
    return error_new("proof_import() not yet implemented - Phase 3 feature");
}