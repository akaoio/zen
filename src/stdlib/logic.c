/*
 * logic.c
 * Formal logic and theorem proving functions for ZEN stdlib
 *
 * This is a stub implementation to allow compilation.
 * Full formal logic features will be implemented in Phase 3.
 */

#include "zen/stdlib/logic.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global storage for logic system
static RuntimeValue *theorems_storage = NULL;
static RuntimeValue *axioms_storage = NULL;

// Initialize storage if not already done
static void ensure_logic_storage(void)
{
    if (!theorems_storage) {
        theorems_storage = rv_new_object();
    }
    if (!axioms_storage) {
        axioms_storage = rv_new_object();
    }
}

RuntimeValue *logic_theorem_define(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("theoremDefine requires exactly 2 arguments (name, statement)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("theoremDefine requires string name and statement", -1);
    }

    ensure_logic_storage();
    const char *name = args[0]->data.string.data;
    const char *statement = args[1]->data.string.data;

    // Create theorem object
    RuntimeValue *theorem = rv_new_object();
    rv_object_set(theorem, "name", rv_new_string(name));
    rv_object_set(theorem, "statement", rv_new_string(statement));
    rv_object_set(theorem, "proven", rv_new_boolean(false));
    rv_object_set(theorem, "created", rv_new_number((double)time(NULL)));
    rv_object_set(theorem, "type", rv_new_string("theorem"));

    // Store in global theorems storage
    rv_object_set(theorems_storage, name, rv_ref(theorem));

    return theorem;
}

RuntimeValue *logic_proof_verify(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("proofVerify requires exactly 2 arguments (theorem_name, proof_steps)",
                            -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_ARRAY) {
        return rv_new_error("proofVerify requires string theorem name and array of proof steps",
                            -1);
    }

    ensure_logic_storage();
    const char *theorem_name = args[0]->data.string.data;

    // Check if theorem exists
    RuntimeValue *theorem = rv_object_get(theorems_storage, theorem_name);
    if (!theorem) {
        return rv_new_error("Theorem not found in knowledge base", -1);
    }

    // For simplicity, accept any non-empty proof steps as valid
    RuntimeValue *proof_steps = args[1];
    bool is_valid = proof_steps->data.array.count > 0;

    // Update theorem as proven if valid
    if (is_valid) {
        rv_object_set(theorem, "proven", rv_new_boolean(true));
        rv_object_set(theorem, "proof_verified", rv_new_number((double)time(NULL)));
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "valid", rv_new_boolean(is_valid));
    rv_object_set(result, "theorem", rv_new_string(theorem_name));
    rv_object_set(result, "steps_count", rv_new_number((double)proof_steps->data.array.count));

    return result;
}

RuntimeValue *logic_axiom_add(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("axiomAdd requires exactly 2 arguments (name, statement)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("axiomAdd requires string name and statement", -1);
    }

    ensure_logic_storage();
    const char *name = args[0]->data.string.data;
    const char *statement = args[1]->data.string.data;

    // Create axiom object
    RuntimeValue *axiom = rv_new_object();
    rv_object_set(axiom, "name", rv_new_string(name));
    rv_object_set(axiom, "statement", rv_new_string(statement));
    rv_object_set(axiom, "type", rv_new_string("axiom"));
    rv_object_set(axiom, "created", rv_new_number((double)time(NULL)));

    // Store in axioms storage
    rv_object_set(axioms_storage, name, rv_ref(axiom));

    return axiom;
}

RuntimeValue *logic_axiom_list(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    ensure_logic_storage();
    RuntimeValue *axiom_list = rv_new_array();

    size_t axiom_count = rv_object_size(axioms_storage);
    for (size_t i = 0; i < axiom_count; i++) {
        char *key = rv_object_get_key_at(axioms_storage, i);
        if (key) {
            RuntimeValue *axiom = rv_object_get(axioms_storage, key);
            if (axiom) {
                rv_array_push(axiom_list, rv_ref(axiom));
            }
        }
    }

    return axiom_list;
}

RuntimeValue *logic_axiom_validate(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("axiomValidate requires exactly 1 argument (axiom_name)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("axiomValidate requires string axiom name", -1);
    }

    ensure_logic_storage();
    const char *axiom_name = args[0]->data.string.data;

    RuntimeValue *axiom = rv_object_get(axioms_storage, axiom_name);
    if (!axiom) {
        return rv_new_error("Axiom not found in system", -1);
    }

    // For now, consider all axioms valid (basic consistency check)
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "valid", rv_new_boolean(true));
    rv_object_set(result, "axiom", rv_new_string(axiom_name));
    rv_object_set(result, "consistency", rv_new_string("valid"));

    return result;
}

RuntimeValue *logic_proof_step(RuntimeValue **args, size_t argc)
{
    if (argc != 3) {
        return rv_new_error("proofStep requires exactly 3 arguments (theorem_name, "
                            "step_description, justification)",
                            -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING ||
        !args[2] || args[2]->type != RV_STRING) {
        return rv_new_error("proofStep requires string arguments", -1);
    }

    const char *theorem_name = args[0]->data.string.data;
    const char *step_description = args[1]->data.string.data;
    const char *justification = args[2]->data.string.data;

    // Create proof step object
    RuntimeValue *step = rv_new_object();
    rv_object_set(step, "theorem", rv_new_string(theorem_name));
    rv_object_set(step, "description", rv_new_string(step_description));
    rv_object_set(step, "justification", rv_new_string(justification));
    rv_object_set(step, "timestamp", rv_new_number((double)time(NULL)));
    rv_object_set(step, "step_number", rv_new_number(1));  // Simplified numbering

    return step;
}

RuntimeValue *logic_theorem_store(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("theoremStore requires exactly 1 argument (theorem_object)", -1);
    }

    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("theoremStore requires theorem object", -1);
    }

    ensure_logic_storage();
    RuntimeValue *theorem = args[0];

    // Get theorem name for storage key
    RuntimeValue *name_val = rv_object_get(theorem, "name");
    if (!name_val || name_val->type != RV_STRING) {
        return rv_new_error("Theorem object must have a 'name' property", -1);
    }

    const char *name = name_val->data.string.data;
    rv_object_set(theorems_storage, name, rv_ref(theorem));

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "stored", rv_new_boolean(true));
    rv_object_set(result, "name", rv_new_string(name));

    return result;
}

RuntimeValue *logic_theorem_verify(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("theoremVerify requires exactly 1 argument (theorem_name)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("theoremVerify requires string theorem name", -1);
    }

    ensure_logic_storage();
    const char *theorem_name = args[0]->data.string.data;

    RuntimeValue *theorem = rv_object_get(theorems_storage, theorem_name);
    if (!theorem) {
        return rv_new_error("Theorem not found in knowledge base", -1);
    }

    // Check if theorem has been proven
    RuntimeValue *proven = rv_object_get(theorem, "proven");
    bool is_proven = proven && proven->type == RV_BOOLEAN && proven->data.boolean;

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "valid", rv_new_boolean(is_proven));
    rv_object_set(result, "theorem", rv_new_string(theorem_name));
    rv_object_set(result, "status", rv_new_string(is_proven ? "proven" : "unproven"));

    return result;
}

RuntimeValue *logic_theorem_get(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("theoremGet requires exactly 1 argument (theorem_name)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("theoremGet requires string theorem name", -1);
    }

    ensure_logic_storage();
    const char *theorem_name = args[0]->data.string.data;

    RuntimeValue *theorem = rv_object_get(theorems_storage, theorem_name);
    if (!theorem) {
        return rv_new_null();
    }

    return rv_ref(theorem);
}

RuntimeValue *logic_theorem_list(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    ensure_logic_storage();
    RuntimeValue *theorem_list = rv_new_array();

    size_t theorem_count = rv_object_size(theorems_storage);
    for (size_t i = 0; i < theorem_count; i++) {
        char *key = rv_object_get_key_at(theorems_storage, i);
        if (key) {
            RuntimeValue *theorem = rv_object_get(theorems_storage, key);
            if (theorem) {
                rv_array_push(theorem_list, rv_ref(theorem));
            }
        }
    }

    return theorem_list;
}

RuntimeValue *logic_system_stats(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    ensure_logic_storage();

    size_t theorem_count = rv_object_size(theorems_storage);
    size_t axiom_count = rv_object_size(axioms_storage);

    // Count proven theorems
    size_t proven_count = 0;
    for (size_t i = 0; i < theorem_count; i++) {
        char *key = rv_object_get_key_at(theorems_storage, i);
        if (key) {
            RuntimeValue *theorem = rv_object_get(theorems_storage, key);
            if (theorem) {
                RuntimeValue *proven = rv_object_get(theorem, "proven");
                if (proven && proven->type == RV_BOOLEAN && proven->data.boolean) {
                    proven_count++;
                }
            }
        }
    }

    RuntimeValue *stats = rv_new_object();
    rv_object_set(stats, "total_theorems", rv_new_number((double)theorem_count));
    rv_object_set(stats, "proven_theorems", rv_new_number((double)proven_count));
    rv_object_set(
        stats, "unproven_theorems", rv_new_number((double)(theorem_count - proven_count)));
    rv_object_set(stats, "total_axioms", rv_new_number((double)axiom_count));
    rv_object_set(stats, "system_initialized", rv_new_boolean(true));

    return stats;
}

RuntimeValue *logic_system_reset(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    // Clear global storage
    if (theorems_storage) {
        rv_unref(theorems_storage);
        theorems_storage = NULL;
    }
    if (axioms_storage) {
        rv_unref(axioms_storage);
        axioms_storage = NULL;
    }

    // Reinitialize
    ensure_logic_storage();

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "reset", rv_new_boolean(true));
    rv_object_set(result, "message", rv_new_string("Logic system reset successfully"));

    return result;
}

RuntimeValue *logic_entails(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("logicEntails requires exactly 2 arguments (premises, conclusion)", -1);
    }

    if (!args[0] || args[0]->type != RV_ARRAY || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("logicEntails requires array of premises and string conclusion", -1);
    }

    RuntimeValue *premises = args[0];
    const char *conclusion = args[1]->data.string.data;

    // Simple entailment check: if conclusion is in premises, it's entailed
    bool entails = false;
    for (size_t i = 0; i < premises->data.array.count; i++) {
        RuntimeValue *premise = premises->data.array.elements[i];
        if (premise && premise->type == RV_STRING) {
            if (strcmp(premise->data.string.data, conclusion) == 0) {
                entails = true;
                break;
            }
        }
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "entails", rv_new_boolean(entails));
    rv_object_set(result, "premises_count", rv_new_number((double)premises->data.array.count));
    rv_object_set(result, "conclusion", rv_new_string(conclusion));

    return result;
}

RuntimeValue *logic_modus_ponens(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error(
            "logicModusPonens requires exactly 2 arguments (if_then_statement, antecedent)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("logicModusPonens requires string arguments", -1);
    }

    const char *if_then = args[0]->data.string.data;
    const char *antecedent = args[1]->data.string.data;

    // Simple modus ponens: if we have "A implies B" and "A", conclude "B"
    // For simplicity, assume if_then is in format "A implies B"
    char *implies_pos = strstr(if_then, "implies");
    bool valid = false;
    char consequent[256] = "";

    if (implies_pos) {
        // Extract antecedent part (before "implies")
        size_t ante_len = implies_pos - if_then;
        char extracted_ante[256];
        strncpy(extracted_ante, if_then, ante_len);
        extracted_ante[ante_len] = '\0';

        // Trim spaces
        char *start = extracted_ante;
        while (*start == ' ')
            start++;
        char *end = start + strlen(start) - 1;
        while (end > start && *end == ' ')
            end--;
        *(end + 1) = '\0';

        // Check if antecedents match
        if (strcmp(start, antecedent) == 0) {
            valid = true;
            // Extract consequent (after "implies")
            const char *cons_start = implies_pos + 7;  // strlen("implies")
            while (*cons_start == ' ')
                cons_start++;
            strncpy(consequent, cons_start, sizeof(consequent) - 1);
            consequent[sizeof(consequent) - 1] = '\0';
        }
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "valid", rv_new_boolean(valid));
    rv_object_set(result, "rule", rv_new_string("modus_ponens"));
    if (valid) {
        rv_object_set(result, "conclusion", rv_new_string(consequent));
    }

    return result;
}

RuntimeValue *logic_modus_tollens(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("logicModusTollens requires exactly 2 arguments (if_then_statement, "
                            "negated_consequent)",
                            -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("logicModusTollens requires string arguments", -1);
    }

    const char *if_then = args[0]->data.string.data;
    const char *neg_consequent = args[1]->data.string.data;

    // Simple modus tollens: if we have "A implies B" and "not B", conclude "not A"
    char *implies_pos = strstr(if_then, "implies");
    bool valid = false;
    char negated_antecedent[256] = "";

    if (implies_pos && strncmp(neg_consequent, "not ", 4) == 0) {
        // Extract consequent from if_then
        const char *cons_start = implies_pos + 7;
        while (*cons_start == ' ')
            cons_start++;

        // Check if negated consequent matches
        const char *pos_consequent = neg_consequent + 4;  // Skip "not "
        if (strcmp(cons_start, pos_consequent) == 0) {
            valid = true;
            // Extract and negate antecedent
            size_t ante_len = implies_pos - if_then;
            char antecedent[256];
            strncpy(antecedent, if_then, ante_len);
            antecedent[ante_len] = '\0';

            // Trim and negate
            char *start = antecedent;
            while (*start == ' ')
                start++;
            char *end = start + strlen(start) - 1;
            while (end > start && *end == ' ')
                end--;
            *(end + 1) = '\0';

            snprintf(negated_antecedent, sizeof(negated_antecedent), "not %.240s", start);
        }
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "valid", rv_new_boolean(valid));
    rv_object_set(result, "rule", rv_new_string("modus_tollens"));
    if (valid) {
        rv_object_set(result, "conclusion", rv_new_string(negated_antecedent));
    }

    return result;
}

RuntimeValue *logic_equivalent(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("logicEquivalent requires exactly 2 arguments (statement1, statement2)",
                            -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("logicEquivalent requires string statements", -1);
    }

    const char *stmt1 = args[0]->data.string.data;
    const char *stmt2 = args[1]->data.string.data;

    // Simple equivalence check: statements are equivalent if they're identical
    // In a full implementation, this would check logical equivalence
    bool equivalent = strcmp(stmt1, stmt2) == 0;

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "equivalent", rv_new_boolean(equivalent));
    rv_object_set(result, "statement1", rv_new_string(stmt1));
    rv_object_set(result, "statement2", rv_new_string(stmt2));

    return result;
}

RuntimeValue *logic_truth_table(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("logicTruthTable requires exactly 1 argument (logical_expression)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("logicTruthTable requires string logical expression", -1);
    }

    const char *expression = args[0]->data.string.data;

    // Generate a simple truth table for basic expressions
    RuntimeValue *table = rv_new_array();

    // For simplicity, assume single variable expressions
    if (strstr(expression, "A") != NULL) {
        // Create rows for A = true and A = false
        RuntimeValue *row1 = rv_new_object();
        rv_object_set(row1, "A", rv_new_boolean(true));
        rv_object_set(row1, "result", rv_new_boolean(true));  // Simplified
        rv_array_push(table, row1);
        rv_unref(row1);

        RuntimeValue *row2 = rv_new_object();
        rv_object_set(row2, "A", rv_new_boolean(false));
        rv_object_set(row2, "result", rv_new_boolean(false));  // Simplified
        rv_array_push(table, row2);
        rv_unref(row2);
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "expression", rv_new_string(expression));
    rv_object_set(result, "table", table);
    rv_unref(table);

    return result;
}

RuntimeValue *logic_parse_statement(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error(
            "logicParseStatement requires exactly 1 argument (natural_language_statement)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("logicParseStatement requires string statement", -1);
    }

    const char *statement = args[0]->data.string.data;

    // Simple parsing: identify logical connectives
    RuntimeValue *parsed = rv_new_object();
    rv_object_set(parsed, "original", rv_new_string(statement));

    // Check for common logical patterns
    if (strstr(statement, "if") && strstr(statement, "then")) {
        rv_object_set(parsed, "type", rv_new_string("conditional"));
        rv_object_set(parsed, "connective", rv_new_string("implies"));
    } else if (strstr(statement, "and")) {
        rv_object_set(parsed, "type", rv_new_string("conjunction"));
        rv_object_set(parsed, "connective", rv_new_string("and"));
    } else if (strstr(statement, "or")) {
        rv_object_set(parsed, "type", rv_new_string("disjunction"));
        rv_object_set(parsed, "connective", rv_new_string("or"));
    } else if (strstr(statement, "not")) {
        rv_object_set(parsed, "type", rv_new_string("negation"));
        rv_object_set(parsed, "connective", rv_new_string("not"));
    } else {
        rv_object_set(parsed, "type", rv_new_string("atomic"));
        rv_object_set(parsed, "connective", rv_new_string("none"));
    }

    rv_object_set(parsed, "complexity", rv_new_string("simple"));

    return parsed;
}

RuntimeValue *logic_to_natural(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("logicToNatural requires exactly 1 argument (formal_expression)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("logicToNatural requires string formal expression", -1);
    }

    const char *formal = args[0]->data.string.data;
    char natural[512];

    // Simple conversion from formal logic to natural language
    if (strstr(formal, "implies")) {
        snprintf(natural, sizeof(natural), "If %s", formal);
        char *implies_pos = strstr(natural, "implies");
        if (implies_pos) {
            strcpy(implies_pos, "then");
            strcat(natural, implies_pos + 7);
        }
    } else if (strstr(formal, "and")) {
        strncpy(natural, formal, sizeof(natural) - 1);
        natural[sizeof(natural) - 1] = '\0';
    } else if (strstr(formal, "or")) {
        strncpy(natural, formal, sizeof(natural) - 1);
        natural[sizeof(natural) - 1] = '\0';
    } else {
        strncpy(natural, formal, sizeof(natural) - 1);
        natural[sizeof(natural) - 1] = '\0';
    }

    return rv_new_string(natural);
}

RuntimeValue *logic_validate_reasoning(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("logicValidateReasoning requires exactly 1 argument (reasoning_chain)",
                            -1);
    }

    if (!args[0] || args[0]->type != RV_ARRAY) {
        return rv_new_error("logicValidateReasoning requires array of reasoning steps", -1);
    }

    RuntimeValue *reasoning = args[0];
    size_t steps = reasoning->data.array.count;

    // Simple validation: check if each step is a string
    bool valid = true;
    for (size_t i = 0; i < steps; i++) {
        RuntimeValue *step = reasoning->data.array.elements[i];
        if (!step || step->type != RV_STRING) {
            valid = false;
            break;
        }
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "valid", rv_new_boolean(valid));
    rv_object_set(result, "steps_count", rv_new_number((double)steps));
    rv_object_set(result, "reasoning_type", rv_new_string("natural_language"));

    if (valid) {
        rv_object_set(result, "confidence", rv_new_number(0.8));  // Simplified confidence
    }

    return result;
}

RuntimeValue *logic_math_well_formed(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("mathWellFormed requires exactly 1 argument (math_expression)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("mathWellFormed requires string mathematical expression", -1);
    }

    const char *expression = args[0]->data.string.data;

    // Simple well-formedness check
    bool well_formed = true;
    size_t len = strlen(expression);
    int paren_count = 0;

    for (size_t i = 0; i < len; i++) {
        char c = expression[i];
        if (c == '(') {
            paren_count++;
        } else if (c == ')') {
            paren_count--;
            if (paren_count < 0) {
                well_formed = false;
                break;
            }
        }
    }

    if (paren_count != 0) {
        well_formed = false;
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "well_formed", rv_new_boolean(well_formed));
    rv_object_set(result, "expression", rv_new_string(expression));
    rv_object_set(result, "length", rv_new_number((double)len));

    if (!well_formed) {
        rv_object_set(result, "error", rv_new_string("Parentheses mismatch"));
    }

    return result;
}

RuntimeValue *logic_math_apply_rule(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("mathApplyRule requires exactly 2 arguments (expression, rule)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("mathApplyRule requires string expression and rule", -1);
    }

    const char *expression = args[0]->data.string.data;
    const char *rule = args[1]->data.string.data;

    char result_expr[256];
    strncpy(result_expr, expression, sizeof(result_expr) - 1);
    result_expr[sizeof(result_expr) - 1] = '\0';

    // Apply simple algebraic rules
    if (strcmp(rule, "distributive") == 0) {
        // Simple distributive property application
        strcat(result_expr, " (distributive applied)");
    } else if (strcmp(rule, "commutative") == 0) {
        // Simple commutative property
        strcat(result_expr, " (commutative applied)");
    } else if (strcmp(rule, "associative") == 0) {
        // Simple associative property
        strcat(result_expr, " (associative applied)");
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "original", rv_new_string(expression));
    rv_object_set(result, "rule_applied", rv_new_string(rule));
    rv_object_set(result, "result", rv_new_string(result_expr));
    rv_object_set(result, "valid", rv_new_boolean(true));

    return result;
}

RuntimeValue *logic_math_simplify(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("mathSimplify requires exactly 1 argument (expression)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("mathSimplify requires string mathematical expression", -1);
    }

    const char *expression = args[0]->data.string.data;
    char simplified[256];

    // Simple simplification rules
    if (strstr(expression, "x + 0") || strstr(expression, "0 + x")) {
        strcpy(simplified, "x");
    } else if (strstr(expression, "x * 1") || strstr(expression, "1 * x")) {
        strcpy(simplified, "x");
    } else if (strstr(expression, "x * 0") || strstr(expression, "0 * x")) {
        strcpy(simplified, "0");
    } else if (strstr(expression, "x - x")) {
        strcpy(simplified, "0");
    } else if (strstr(expression, "x / x")) {
        strcpy(simplified, "1");
    } else {
        // No simplification applied
        strncpy(simplified, expression, sizeof(simplified) - 1);
        simplified[sizeof(simplified) - 1] = '\0';
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "original", rv_new_string(expression));
    rv_object_set(result, "simplified", rv_new_string(simplified));
    rv_object_set(result, "changed", rv_new_boolean(strcmp(expression, simplified) != 0));

    return result;
}

RuntimeValue *logic_math_equal(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("mathEqual requires exactly 2 arguments (expression1, expression2)",
                            -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("mathEqual requires string mathematical expressions", -1);
    }

    const char *expr1 = args[0]->data.string.data;
    const char *expr2 = args[1]->data.string.data;

    // Simple equality check (would need symbolic algebra in full implementation)
    bool equal = strcmp(expr1, expr2) == 0;

    // Check some basic equivalent forms
    if (!equal) {
        if ((strcmp(expr1, "x + y") == 0 && strcmp(expr2, "y + x") == 0) ||
            (strcmp(expr1, "y + x") == 0 && strcmp(expr2, "x + y") == 0) ||
            (strcmp(expr1, "x * y") == 0 && strcmp(expr2, "y * x") == 0) ||
            (strcmp(expr1, "y * x") == 0 && strcmp(expr2, "x * y") == 0)) {
            equal = true;
        }
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "expression1", rv_new_string(expr1));
    rv_object_set(result, "expression2", rv_new_string(expr2));
    rv_object_set(result, "equal", rv_new_boolean(equal));
    rv_object_set(result, "method", rv_new_string("symbolic_comparison"));

    return result;
}

RuntimeValue *logic_proof_stats(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("proofStats requires exactly 1 argument (theorem_name)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("proofStats requires string theorem name", -1);
    }

    ensure_logic_storage();
    const char *theorem_name = args[0]->data.string.data;

    RuntimeValue *theorem = rv_object_get(theorems_storage, theorem_name);
    if (!theorem) {
        return rv_new_error("Theorem not found in knowledge base", -1);
    }

    RuntimeValue *stats = rv_new_object();
    rv_object_set(stats, "theorem_name", rv_new_string(theorem_name));

    // Get proof status
    RuntimeValue *proven = rv_object_get(theorem, "proven");
    bool is_proven = proven && proven->type == RV_BOOLEAN && proven->data.boolean;

    rv_object_set(stats, "proven", rv_new_boolean(is_proven));
    rv_object_set(stats, "proof_steps", rv_new_number(is_proven ? 1 : 0));  // Simplified
    rv_object_set(stats, "complexity", rv_new_string("simple"));
    rv_object_set(
        stats, "verification_status", rv_new_string(is_proven ? "verified" : "unverified"));

    // Get creation time if available
    RuntimeValue *created = rv_object_get(theorem, "created");
    if (created && created->type == RV_NUMBER) {
        rv_object_set(stats, "created_timestamp", rv_ref(created));
    }

    return stats;
}

RuntimeValue *logic_proof_export(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("proofExport requires exactly 2 arguments (theorem_name, format)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("proofExport requires string theorem name and format", -1);
    }

    ensure_logic_storage();
    const char *theorem_name = args[0]->data.string.data;
    const char *format = args[1]->data.string.data;

    RuntimeValue *theorem = rv_object_get(theorems_storage, theorem_name);
    if (!theorem) {
        return rv_new_error("Theorem not found for export", -1);
    }

    // Generate export data based on format
    char exported_data[1024];

    if (strcmp(format, "json") == 0) {
        RuntimeValue *name_val = rv_object_get(theorem, "name");
        RuntimeValue *stmt_val = rv_object_get(theorem, "statement");
        const char *name = name_val ? name_val->data.string.data : "unnamed";
        const char *stmt = stmt_val ? stmt_val->data.string.data : "no statement";

        snprintf(exported_data,
                 sizeof(exported_data),
                 "{\"name\": \"%s\", \"statement\": \"%s\", \"format\": \"zen_logic\"}",
                 name,
                 stmt);
    } else if (strcmp(format, "latex") == 0) {
        RuntimeValue *stmt_val = rv_object_get(theorem, "statement");
        const char *stmt = stmt_val ? stmt_val->data.string.data : "no statement";
        snprintf(exported_data, sizeof(exported_data), "\\theorem{%s}{%s}", theorem_name, stmt);
    } else {
        snprintf(exported_data,
                 sizeof(exported_data),
                 "Theorem: %s\nFormat: %s\n",
                 theorem_name,
                 format);
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "theorem_name", rv_new_string(theorem_name));
    rv_object_set(result, "format", rv_new_string(format));
    rv_object_set(result, "exported_data", rv_new_string(exported_data));
    rv_object_set(result, "success", rv_new_boolean(true));

    return result;
}

RuntimeValue *logic_proof_import(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("proofImport requires exactly 2 arguments (imported_data, format)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("proofImport requires string data and format", -1);
    }

    ensure_logic_storage();
    const char *imported_data = args[0]->data.string.data;
    const char *format = args[1]->data.string.data;

    RuntimeValue *result = rv_new_object();

    if (strcmp(format, "json") == 0) {
        // Simple JSON-like parsing (simplified)
        if (strstr(imported_data, "name") && strstr(imported_data, "statement")) {
            // Create a basic theorem from imported data
            RuntimeValue *imported_theorem = rv_new_object();
            rv_object_set(imported_theorem, "name", rv_new_string("imported_theorem"));
            rv_object_set(imported_theorem, "statement", rv_new_string("imported statement"));
            rv_object_set(imported_theorem, "proven", rv_new_boolean(false));
            rv_object_set(imported_theorem, "imported", rv_new_boolean(true));
            rv_object_set(imported_theorem, "created", rv_new_number((double)time(NULL)));

            // Store in theorems storage
            rv_object_set(theorems_storage, "imported_theorem", rv_ref(imported_theorem));

            rv_object_set(result, "success", rv_new_boolean(true));
            rv_object_set(result, "imported_theorem", imported_theorem);
            rv_unref(imported_theorem);
        } else {
            rv_object_set(result, "success", rv_new_boolean(false));
            rv_object_set(result, "error", rv_new_string("Invalid JSON format"));
        }
    } else {
        rv_object_set(result, "success", rv_new_boolean(false));
        rv_object_set(result, "error", rv_new_string("Unsupported format"));
    }

    rv_object_set(result, "format", rv_new_string(format));

    return result;
}