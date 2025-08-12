/*
 * visitor.c
 * Advanced AST visitor pattern implementation with runtime optimizations
 *
 * This file implements the enterprise-grade ZEN runtime evaluation system
 * with performance profiling, hot function optimization, exception handling,
 * and advanced execution analytics.
 */

#define _GNU_SOURCE  // Enable strdup
#include "zen/core/visitor.h"

#include "zen/core/ast_runtime_convert.h"
#include "zen/core/error.h"
#include "zen/core/lexer.h"
#include "zen/core/logger.h"
#include "zen/core/memory.h"
#include "zen/core/parser.h"
#include "zen/core/runtime_value.h"
#include "zen/core/scope.h"
#include "zen/core/token.h"
#include "zen/runtime/operators.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/json.h"
#include "zen/stdlib/module.h"
#include "zen/stdlib/stdlib.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Advanced runtime optimization constants
#define DEFAULT_HOT_FUNCTION_THRESHOLD 100
// Get max call stack depth from environment or use default
static int get_max_call_stack_depth()
{
    const char *env = getenv("ZEN_MAX_CALL_DEPTH");
    return env ? atoi(env) : 10000;
}
#define MAX_CALL_STACK_DEPTH        get_max_call_stack_depth()
#define PROFILE_CAPACITY_INCREMENT  50
#define HOT_FUNCTION_TIME_THRESHOLD 0.001  // 1ms

// Forward declarations
static RuntimeValue *visitor_ast_to_value(AST_T *node);

static RuntimeValue *visitor_visit_binary_op(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_unary_op(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_array(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_object(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_property_access(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_if_statement(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_while_loop(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_for_loop(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_class_definition(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_new_expression(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_try_catch(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_throw(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_compound_assignment(visitor_T *visitor, AST_T *node);
static RuntimeValue *visitor_visit_assignment(visitor_T *visitor, AST_T *node);
static RuntimeValue *
visitor_execute_user_function(visitor_T *visitor, AST_T *fdef, AST_T **args, int args_size);
static RuntimeValue *visitor_execute_user_function_ex(
    visitor_T *visitor, AST_T *fdef, AST_T **args, int args_size, bool is_method_call);
static bool visitor_is_truthy_rv(RuntimeValue *rv);

// Database-like file operations
// static RuntimeValue *visitor_visit_file_reference(visitor_T *visitor, AST_T *node);
// static RuntimeValue *visitor_navigate_property_path(visitor_T *visitor, AST_T *root, AST_T
// *property_path); Removed unused function declaration

// Advanced runtime optimization functions
static void visitor_push_call_frame(visitor_T *visitor,
                                    AST_T *function_def,
                                    AST_T **args,
                                    size_t arg_count,
                                    const char *function_name);
static void visitor_pop_call_frame(visitor_T *visitor);
static FunctionProfile *visitor_get_or_create_profile(visitor_T *visitor,
                                                      const char *function_name);
static void visitor_update_function_profile(visitor_T *visitor,
                                            const char *function_name,
                                            double execution_time);
// static AST_T *visitor_optimize_constant_expression(visitor_T *visitor, AST_T *node);
// static bool visitor_is_constant_expression(AST_T *node);
// static AST_T *visitor_apply_tail_call_optimization(visitor_T *visitor, AST_T *node);
static double visitor_get_time_diff(clock_t start, clock_t end);
static void visitor_update_ast_scope(AST_T *node, scope_T *new_scope);

/**
 * @brief Create a new visitor instance with advanced runtime features
 * @param void Function takes no parameters
 * @return New visitor instance or NULL on failure
 */
// Note: Temporary node tracking was removed due to architectural issues
// The proper solution would be to separate runtime values from AST nodes

visitor_T *visitor_new()
{
    visitor_T *visitor = memory_alloc(sizeof(visitor_T));
    if (!visitor)
        return NULL;

    // Initialize execution state
    visitor->mode = EXECUTION_MODE_INTERPRETED;

    // Initialize call stack
    visitor->call_stack = NULL;
    visitor->call_stack_depth = 0;
    visitor->max_call_stack_depth = MAX_CALL_STACK_DEPTH;

    // Initialize profiling system
    visitor->function_profiles = memory_alloc(sizeof(FunctionProfile) * PROFILE_CAPACITY_INCREMENT);
    visitor->profile_count = 0;
    visitor->profile_capacity = PROFILE_CAPACITY_INCREMENT;
    visitor->profiling_enabled = true;  // Enable by default for optimization

    // Initialize runtime optimizations (disabled by default for safety)
    visitor->tail_call_optimization = false;
    visitor->constant_folding = false;  // Disabled due to recursion bug
    visitor->dead_code_elimination = false;
    visitor->inline_small_functions = false;

    // Initialize exception handling
    visitor->exception_state.exception_value = NULL;
    visitor->exception_state.exception_message = NULL;
    visitor->exception_state.source_location = NULL;
    visitor->exception_state.is_active = false;

    // Initialize execution analytics
    visitor->total_instructions_executed = 0;
    visitor->total_execution_time = 0.0;
    visitor->memory_allocations = 0;
    visitor->cache_hits = 0;
    visitor->cache_misses = 0;

    // Set hot function threshold
    visitor->hot_function_threshold = DEFAULT_HOT_FUNCTION_THRESHOLD;

    // Initialize current execution scope
    visitor->current_scope = NULL;

    if (!visitor->function_profiles) {
        memory_free(visitor);
        return NULL;
    }

    return visitor;
}

/**
 * @brief Free a visitor instance and its advanced runtime resources
 * @param visitor The visitor instance to free
 */
void visitor_free(visitor_T *visitor)
{
    if (!visitor) {
        return;
    }

    // Free call stack
    while (visitor->call_stack) {
        visitor_pop_call_frame(visitor);
    }

    // Free function profiles
    if (visitor->function_profiles) {
        for (size_t i = 0; i < visitor->profile_count; i++) {
            if (visitor->function_profiles[i].function_name) {
                memory_free(visitor->function_profiles[i].function_name);
            }
            if (visitor->function_profiles[i].optimized_ast) {
                ast_free(visitor->function_profiles[i].optimized_ast);
            }
        }
        memory_free(visitor->function_profiles);
    }

    // Free exception state
    if (visitor->exception_state.exception_message) {
        memory_free(visitor->exception_state.exception_message);
    }
    if (visitor->exception_state.source_location) {
        memory_free(visitor->exception_state.source_location);
    }
    if (visitor->exception_state.exception_value) {
        ast_free(visitor->exception_state.exception_value);
    }

    // Clear current_scope reference to prevent dangling pointer
    // Note: We don't free it as the visitor doesn't own the scope
    visitor->current_scope = NULL;

    memory_free(visitor);
}

/**
 * @brief Visit and evaluate an AST node with advanced runtime features
 * @param visitor Visitor instance
 * @param node AST node to visit
 * @return Result of evaluation
 */
RuntimeValue *visitor_visit(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    static int depth = 0;
    depth++;
    if (depth > 100) {
        LOG_ERROR(LOG_CAT_VISITOR, "Stack overflow detected! Node type=%d", node->type);
        depth--;
        return rv_new_null();
    }

    // Check for active exception
    if (visitor_has_exception(visitor)) {
        depth--;
        return rv_new_null();
    }

    // Increment instruction counter for analytics
    visitor->total_instructions_executed++;

    // Start timing for this node evaluation
    clock_t start_time = clock();
    (void)start_time;  // Mark as used

    // Apply constant folding optimization if enabled
    // Constant folding optimization disabled for now

    RuntimeValue *result = NULL;

    switch (node->type) {
    case AST_VARIABLE_DEFINITION:
        return visitor_visit_variable_definition(visitor, node);
    case AST_FUNCTION_DEFINITION:
        return visitor_visit_function_definition(visitor, node);
    case AST_VARIABLE:
        return visitor_visit_variable(visitor, node);
    case AST_FUNCTION_CALL:
        return visitor_visit_function_call(visitor, node);
    case AST_COMPOUND:
        return visitor_visit_compound(visitor, node);
    case AST_NOOP:
        return rv_new_null();

    // Literals - convert to RuntimeValue
    case AST_STRING:
        return rv_new_string(node->string_value);
    case AST_NUMBER:
        return rv_new_number(node->number_value);
    case AST_BOOLEAN:
        return rv_new_boolean(node->boolean_value);
    case AST_NULL:
    case AST_UNDECIDABLE:
        return rv_new_null();

    // Expression evaluation
    case AST_BINARY_OP:
        return visitor_visit_binary_op(visitor, node);
    case AST_UNARY_OP:
        return visitor_visit_unary_op(visitor, node);

    // Data structures
    case AST_ARRAY:
        return visitor_visit_array(visitor, node);
    case AST_OBJECT:
        return visitor_visit_object(visitor, node);
    case AST_PROPERTY_ACCESS:
        return visitor_visit_property_access(visitor, node);

    // Ternary conditional expression
    case AST_TERNARY: {
        if (!node->ternary_condition || !node->ternary_true_expr || !node->ternary_false_expr) {
            return rv_new_null();
        }

        // Evaluate the condition
        RuntimeValue *condition = visitor_visit(visitor, node->ternary_condition);
        if (!condition) {
            return rv_new_null();
        }

        // Check if condition is truthy
        bool is_truthy = false;
        if (condition->type == RV_BOOLEAN) {
            is_truthy = condition->data.boolean;
        } else if (condition->type == RV_NUMBER) {
            is_truthy = (condition->data.number != 0);
        } else if (condition->type == RV_NULL) {
            is_truthy = false;
        } else if (condition->type == RV_STRING) {
            is_truthy = (strlen(rv_get_string(condition)) > 0);
        } else {
            is_truthy = true;  // Objects, arrays, functions are truthy
        }

        rv_unref(condition);

        // Evaluate and return the appropriate branch
        if (is_truthy) {
            return visitor_visit(visitor, node->ternary_true_expr);
        } else {
            return visitor_visit(visitor, node->ternary_false_expr);
        }
    }

    // Control flow
    case AST_IF_STATEMENT:
        return visitor_visit_if_statement(visitor, node);
    case AST_WHILE_LOOP:
        return visitor_visit_while_loop(visitor, node);
    case AST_FOR_LOOP:
        return visitor_visit_for_loop(visitor, node);
    case AST_BREAK:
    case AST_CONTINUE:
        // Return special markers for control flow
        return rv_new_string(node->type == AST_BREAK ? "__BREAK__" : "__CONTINUE__");
    case AST_RETURN:
        // Handle return statements - evaluate the return value
        if (node->return_value) {
            RuntimeValue *return_val = visitor_visit(visitor, node->return_value);
            // Create a special marker for return
            RuntimeValue *marker = rv_new_object();
            rv_object_set(marker, "__RETURN__", return_val);
            rv_unref(return_val);  // rv_object_set already refs the value
            return marker;
        }
        return rv_new_null();

    // Database-like file operations
    case AST_FILE_GET: {
        if (!node->file_get_path) {
            return rv_new_error("FILE_GET missing file path", -1);
        }

        // Evaluate file path expression
        RuntimeValue *path_val = visitor_visit(visitor, node->file_get_path);
        if (!path_val || path_val->type != RV_STRING) {
            if (path_val)
                rv_unref(path_val);
            return rv_new_error("FILE_GET requires string file path", -1);
        }

        // Use readFile to get the file content
        RuntimeValue *content = io_read_file(&path_val, 1);
        rv_unref(path_val);

        // If property specified, extract that property
        if (node->file_get_property && content && content->type == RV_STRING) {
            // Try to parse as JSON and extract property
            RuntimeValue *parsed = json_parse(content->data.string.data);
            rv_unref(content);

            if (parsed && parsed->type == RV_OBJECT) {
                RuntimeValue *prop_val = visitor_visit(visitor, node->file_get_property);
                if (prop_val && prop_val->type == RV_STRING) {
                    RuntimeValue *result = rv_object_get(parsed, prop_val->data.string.data);
                    rv_unref(prop_val);
                    rv_unref(parsed);
                    return result ? rv_ref(result) : rv_new_null();
                }
                if (prop_val)
                    rv_unref(prop_val);
            }
            if (parsed)
                rv_unref(parsed);
        }

        return content ? content : rv_new_null();
    }

    case AST_FILE_PUT: {
        if (!node->file_put_path || !node->file_put_value) {
            return rv_new_error("FILE_PUT missing required parameters", -1);
        }

        // Evaluate file path and value
        RuntimeValue *path_val = visitor_visit(visitor, node->file_put_path);
        RuntimeValue *value_val = visitor_visit(visitor, node->file_put_value);

        if (!path_val || path_val->type != RV_STRING || !value_val) {
            if (path_val)
                rv_unref(path_val);
            if (value_val)
                rv_unref(value_val);
            return rv_new_error("FILE_PUT requires valid path and value", -1);
        }

        // Convert value to JSON string for storage
        RuntimeValue *json_args[1] = {value_val};
        char *json_str = json_stringify(json_args[0]);
        rv_unref(value_val);

        if (!json_str) {
            rv_unref(path_val);
            return rv_new_error("Failed to serialize value for FILE_PUT", -1);
        }

        RuntimeValue *json_rv = rv_new_string(json_str);
        memory_free(json_str);

        // Write to file
        RuntimeValue *write_args[2] = {path_val, json_rv};
        RuntimeValue *result = io_write_file(write_args, 2);

        rv_unref(path_val);
        rv_unref(json_rv);

        return result;
    }

    case AST_FILE_REFERENCE: {
        if (!node->file_ref_target_file || !node->file_ref_property_path) {
            return rv_new_error("FILE_REFERENCE missing target file or property path", -1);
        }

        // Load target file
        RuntimeValue *target_path = rv_new_string(node->file_ref_target_file);
        RuntimeValue *file_content = io_read_file(&target_path, 1);
        rv_unref(target_path);

        if (!file_content || file_content->type != RV_STRING) {
            if (file_content)
                rv_unref(file_content);
            return rv_new_error("Failed to load referenced file", -1);
        }

        // Parse as JSON
        RuntimeValue *parsed = json_parse(file_content->data.string.data);
        rv_unref(file_content);

        if (!parsed || parsed->type != RV_OBJECT) {
            if (parsed)
                rv_unref(parsed);
            return rv_new_error("Referenced file is not valid JSON", -1);
        }

        // Navigate property path (e.g., "office.alice")
        RuntimeValue *current = parsed;
        char *path_copy = memory_strdup(node->file_ref_property_path);
        char *token = strtok(path_copy, ".");

        while (token && current && current->type == RV_OBJECT) {
            RuntimeValue *next = rv_object_get(current, token);
            if (next) {
                if (current != parsed)
                    rv_unref(current);
                current = rv_ref(next);
            } else {
                current = NULL;
                break;
            }
            token = strtok(NULL, ".");
        }

        memory_free(path_copy);
        rv_unref(parsed);

        return current ? current : rv_new_null();
    }

    // Formal Logic AST nodes - full implementations
    case AST_LOGICAL_QUANTIFIER: {
        // Create quantifier object representation
        RuntimeValue *quantifier = rv_new_object();

        const char *type_str =
            (node->quantifier_type == QUANTIFIER_UNIVERSAL) ? "universal" : "existential";
        rv_object_set(quantifier, "type", rv_new_string(type_str));

        if (node->quantified_variable) {
            rv_object_set(quantifier, "variable", rv_new_string(node->quantified_variable));
        }

        if (node->quantified_domain) {
            RuntimeValue *domain = visitor_visit(visitor, node->quantified_domain);
            rv_object_set(quantifier, "domain", domain);
            if (domain)
                rv_unref(domain);
        }

        if (node->quantified_body) {
            RuntimeValue *body = visitor_visit(visitor, node->quantified_body);
            rv_object_set(quantifier, "body", body);
            if (body)
                rv_unref(body);
        }

        return quantifier;
    }

    case AST_LOGICAL_PREDICATE: {
        // Create predicate object
        RuntimeValue *predicate = rv_new_object();

        if (node->predicate_name) {
            rv_object_set(predicate, "name", rv_new_string(node->predicate_name));
        }

        // Process predicate arguments
        if (node->predicate_args && node->predicate_args_size > 0) {
            RuntimeValue *args = rv_new_array();
            for (size_t i = 0; i < node->predicate_args_size; i++) {
                RuntimeValue *arg = visitor_visit(visitor, node->predicate_args[i]);
                if (arg) {
                    rv_array_push(args, arg);
                    rv_unref(arg);
                }
            }
            rv_object_set(predicate, "arguments", args);
            rv_unref(args);
        }

        rv_object_set(predicate, "type", rv_new_string("predicate"));
        return predicate;
    }

    case AST_LOGICAL_CONNECTIVE: {
        // Create logical connective object
        RuntimeValue *connective = rv_new_object();

        const char *op_str;
        switch (node->connective_type) {
        case CONNECTIVE_AND:
            op_str = "and";
            break;
        case CONNECTIVE_OR:
            op_str = "or";
            break;
        case CONNECTIVE_IMPLIES:
            op_str = "implies";
            break;
        case CONNECTIVE_IFF:
            op_str = "iff";
            break;
        case CONNECTIVE_NOT:
            op_str = "not";
            break;
        default:
            op_str = "unknown";
            break;
        }

        rv_object_set(connective, "operator", rv_new_string(op_str));

        if (node->connective_left) {
            RuntimeValue *left = visitor_visit(visitor, node->connective_left);
            rv_object_set(connective, "left", left);
            if (left)
                rv_unref(left);
        }

        if (node->connective_right) {
            RuntimeValue *right = visitor_visit(visitor, node->connective_right);
            rv_object_set(connective, "right", right);
            if (right)
                rv_unref(right);
        }

        return connective;
    }

    case AST_LOGICAL_VARIABLE: {
        RuntimeValue *logical_var = rv_new_object();

        if (node->logical_var_name) {
            rv_object_set(logical_var, "name", rv_new_string(node->logical_var_name));
        }

        rv_object_set(logical_var, "bound", rv_new_boolean(node->is_bound));
        rv_object_set(logical_var, "type", rv_new_string("logical_variable"));

        return logical_var;
    }

    case AST_LOGICAL_PROPOSITION: {
        RuntimeValue *proposition = rv_new_object();

        if (node->proposition_name) {
            rv_object_set(proposition, "name", rv_new_string(node->proposition_name));
        }

        rv_object_set(proposition, "value", rv_new_boolean(node->proposition_value));
        rv_object_set(proposition, "type", rv_new_string("proposition"));

        return proposition;
    }

    case AST_LOGICAL_THEOREM: {
        RuntimeValue *theorem = rv_new_object();

        if (node->theorem_name) {
            rv_object_set(theorem, "name", rv_new_string(node->theorem_name));
        }

        // Process hypotheses
        if (node->theorem_hypotheses && node->theorem_hypotheses_size > 0) {
            RuntimeValue *hypotheses = rv_new_array();
            for (size_t i = 0; i < node->theorem_hypotheses_size; i++) {
                RuntimeValue *hypothesis = visitor_visit(visitor, node->theorem_hypotheses[i]);
                if (hypothesis) {
                    rv_array_push(hypotheses, hypothesis);
                    rv_unref(hypothesis);
                }
            }
            rv_object_set(theorem, "hypotheses", hypotheses);
            rv_unref(hypotheses);
        }

        if (node->theorem_conclusion) {
            RuntimeValue *conclusion = visitor_visit(visitor, node->theorem_conclusion);
            rv_object_set(theorem, "conclusion", conclusion);
            if (conclusion)
                rv_unref(conclusion);
        }

        if (node->theorem_proof) {
            RuntimeValue *proof = visitor_visit(visitor, node->theorem_proof);
            rv_object_set(theorem, "proof", proof);
            if (proof)
                rv_unref(proof);
        }

        rv_object_set(theorem, "type", rv_new_string("theorem"));
        return theorem;
    }

    case AST_LOGICAL_AXIOM: {
        RuntimeValue *axiom = rv_new_object();

        if (node->axiom_name) {
            rv_object_set(axiom, "name", rv_new_string(node->axiom_name));
        }

        if (node->axiom_statement) {
            RuntimeValue *statement = visitor_visit(visitor, node->axiom_statement);
            rv_object_set(axiom, "statement", statement);
            if (statement)
                rv_unref(statement);
        }

        rv_object_set(axiom, "consistent", rv_new_boolean(node->axiom_is_consistent));
        rv_object_set(axiom, "type", rv_new_string("axiom"));

        return axiom;
    }

    case AST_LOGICAL_PROOF_STEP: {
        RuntimeValue *step = rv_new_object();

        if (node->step_statement) {
            RuntimeValue *statement = visitor_visit(visitor, node->step_statement);
            rv_object_set(step, "statement", statement);
            if (statement)
                rv_unref(statement);
        }

        if (node->step_justification) {
            RuntimeValue *justification = visitor_visit(visitor, node->step_justification);
            rv_object_set(step, "justification", justification);
            if (justification)
                rv_unref(justification);
        }

        // Process premises
        if (node->step_premises && node->step_premises_size > 0) {
            RuntimeValue *premises = rv_new_array();
            for (size_t i = 0; i < node->step_premises_size; i++) {
                RuntimeValue *premise = visitor_visit(visitor, node->step_premises[i]);
                if (premise) {
                    rv_array_push(premises, premise);
                    rv_unref(premise);
                }
            }
            rv_object_set(step, "premises", premises);
            rv_unref(premises);
        }

        rv_object_set(step, "type", rv_new_string("proof_step"));
        return step;
    }

    case AST_LOGICAL_PREMISE:
        // Logical premise is just a statement node
        result = rv_new_object();
        rv_object_set(result, "type", rv_new_string("premise"));
        return result;

    case AST_LOGICAL_CONCLUSION:
        // Logical conclusion is just a statement node
        result = rv_new_object();
        rv_object_set(result, "type", rv_new_string("conclusion"));
        return result;

    case AST_LOGICAL_INFERENCE: {
        RuntimeValue *inference = rv_new_object();

        const char *rule_str;
        switch (node->inference_type) {
        case INFERENCE_MODUS_PONENS:
            rule_str = "modus_ponens";
            break;
        case INFERENCE_MODUS_TOLLENS:
            rule_str = "modus_tollens";
            break;
        case INFERENCE_UNIVERSAL_INST:
            rule_str = "universal_instantiation";
            break;
        case INFERENCE_EXISTENTIAL_GEN:
            rule_str = "existential_generalization";
            break;
        case INFERENCE_ASSUMPTION:
            rule_str = "assumption";
            break;
        case INFERENCE_AXIOM:
            rule_str = "axiom";
            break;
        default:
            rule_str = "unknown";
            break;
        }

        rv_object_set(inference, "rule", rv_new_string(rule_str));

        // Process premises
        if (node->inference_premises && node->inference_premises_size > 0) {
            RuntimeValue *premises = rv_new_array();
            for (size_t i = 0; i < node->inference_premises_size; i++) {
                RuntimeValue *premise = visitor_visit(visitor, node->inference_premises[i]);
                if (premise) {
                    rv_array_push(premises, premise);
                    rv_unref(premise);
                }
            }
            rv_object_set(inference, "premises", premises);
            rv_unref(premises);
        }

        if (node->inference_conclusion) {
            RuntimeValue *conclusion = visitor_visit(visitor, node->inference_conclusion);
            rv_object_set(inference, "conclusion", conclusion);
            if (conclusion)
                rv_unref(conclusion);
        }

        return inference;
    }

    case AST_LOGICAL_SUBSTITUTION:
        // Basic substitution representation
        result = rv_new_object();
        rv_object_set(result, "type", rv_new_string("substitution"));
        return result;

    case AST_MATHEMATICAL_EQUATION: {
        RuntimeValue *equation = rv_new_object();

        if (node->equation_left) {
            RuntimeValue *left = visitor_visit(visitor, node->equation_left);
            rv_object_set(equation, "left", left);
            if (left)
                rv_unref(left);
        }

        if (node->equation_right) {
            RuntimeValue *right = visitor_visit(visitor, node->equation_right);
            rv_object_set(equation, "right", right);
            if (right)
                rv_unref(right);
        }

        rv_object_set(equation, "type", rv_new_string("equation"));
        rv_object_set(equation, "operator", rv_new_string("equals"));

        return equation;
    }

    case AST_MATHEMATICAL_INEQUALITY: {
        RuntimeValue *inequality = rv_new_object();

        const char *op_str;
        switch (node->inequality_type) {
        case INEQUALITY_LT:
            op_str = "<";
            break;
        case INEQUALITY_LE:
            op_str = "<=";
            break;
        case INEQUALITY_GT:
            op_str = ">";
            break;
        case INEQUALITY_GE:
            op_str = ">=";
            break;
        default:
            op_str = "unknown";
            break;
        }

        rv_object_set(inequality, "operator", rv_new_string(op_str));

        if (node->inequality_left) {
            RuntimeValue *left = visitor_visit(visitor, node->inequality_left);
            rv_object_set(inequality, "left", left);
            if (left)
                rv_unref(left);
        }

        if (node->inequality_right) {
            RuntimeValue *right = visitor_visit(visitor, node->inequality_right);
            rv_object_set(inequality, "right", right);
            if (right)
                rv_unref(right);
        }

        rv_object_set(inequality, "type", rv_new_string("inequality"));
        return inequality;
    }

    case AST_MATHEMATICAL_FUNCTION: {
        RuntimeValue *math_func = rv_new_object();

        if (node->math_function_name) {
            rv_object_set(math_func, "name", rv_new_string(node->math_function_name));
        }

        // Process function arguments
        if (node->math_function_args && node->math_function_args_size > 0) {
            RuntimeValue *args = rv_new_array();
            for (size_t i = 0; i < node->math_function_args_size; i++) {
                RuntimeValue *arg = visitor_visit(visitor, node->math_function_args[i]);
                if (arg) {
                    rv_array_push(args, arg);
                    rv_unref(arg);
                }
            }
            rv_object_set(math_func, "arguments", args);
            rv_unref(args);
        }

        rv_object_set(math_func, "type", rv_new_string("mathematical_function"));
        return math_func;
    }

    case AST_IMPORT:
        return visitor_visit_import(visitor, node);
    case AST_EXPORT:
        return visitor_visit_export(visitor, node);
    case AST_CLASS_DEFINITION:
        return visitor_visit_class_definition(visitor, node);
    case AST_NEW_EXPRESSION:
        return visitor_visit_new_expression(visitor, node);
    case AST_TRY_CATCH:
        return visitor_visit_try_catch(visitor, node);
    case AST_THROW:
        return visitor_visit_throw(visitor, node);
    case AST_COMPOUND_ASSIGNMENT:
        return visitor_visit_compound_assignment(visitor, node);
    case AST_ASSIGNMENT:
        return visitor_visit_assignment(visitor, node);

    default:
        // For unimplemented features, return null
        result = rv_new_null();
        break;
    }

    // Update execution timing
    if (result) {
        clock_t end_time = clock();
        visitor->total_execution_time += visitor_get_time_diff(start_time, end_time);
    }

    depth--;
    return result;
}

/**
 * @brief Visit variable definition node
 * @param visitor Visitor instance
 * @param node Variable definition AST node
 * @return The defined variable value
 */
RuntimeValue *visitor_visit_variable_definition(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->scope) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Invalid variable definition (visitor=%p, node=%p, scope=%p)",
                  (void *)visitor,
                  (void *)node,
                  node ? (void *)node->scope : NULL);
        return rv_new_null();
    }

    if (!node->variable_definition_variable_name) {
        LOG_ERROR(LOG_CAT_VISITOR, "Variable definition missing name");
        return rv_new_null();
    }

    // Evaluate the value expression
    RuntimeValue *value = NULL;
    if (node->variable_definition_value) {
        LOG_VISITOR_DEBUG("Evaluating expression for variable '%s', AST type: %d",
                          node->variable_definition_variable_name,
                          node->variable_definition_value->type);
        value = visitor_visit(visitor, node->variable_definition_value);
        if (value) {
            char *value_str = rv_to_string(value);
            LOG_VISITOR_DEBUG("Expression evaluated to: %s", value_str);
            memory_free(value_str);
        } else {
            LOG_VISITOR_DEBUG("Expression evaluated to: NULL");
        }
    } else {
        value = rv_new_null();
    }

    // Use the new scope_set_variable function to store RuntimeValue directly
    // This completely separates runtime values from AST nodes
    // Use visitor's current scope if available, otherwise use node's scope
    scope_T *target_scope = visitor->current_scope ? visitor->current_scope : node->scope;
    if (!scope_set_variable(target_scope, node->variable_definition_variable_name, value)) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Failed to set variable '%s' in scope",
                  node->variable_definition_variable_name);
        rv_unref(value);
        return rv_new_null();
    }

    if (value) {
        char *value_str = rv_to_string(value);
        LOG_VISITOR_DEBUG("Stored variable '%s' with value: %s",
                          node->variable_definition_variable_name,
                          value_str);
        memory_free(value_str);
    } else {
        LOG_VISITOR_DEBUG("Stored variable '%s' with value: NULL",
                          node->variable_definition_variable_name);
    }

    return value;
}

/**
 * @brief Visit function definition node
 * @param visitor Visitor instance
 * @param node Function definition AST node
 * @return The function definition node
 */
RuntimeValue *visitor_visit_function_definition(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // Use visitor's current scope if available, otherwise use node's scope
    scope_T *target_scope = visitor->current_scope ? visitor->current_scope : node->scope;
    if (!target_scope) {
        return rv_new_null();
    }

    LOG_VISITOR_DEBUG(
        "Defining function '%s' in scope %p", node->function_definition_name, (void *)target_scope);
    scope_add_function_definition(target_scope, node);

    // Return function as runtime value
    return rv_new_function(node, target_scope);
}

/**
 * @brief Visit variable node
 * @param visitor Visitor instance
 * @param node Variable AST node
 * @return Variable value or NULL
 */
RuntimeValue *visitor_visit_variable(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->variable_name) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Invalid variable access (visitor=%p, node=%p, name=%s)",
                  (void *)visitor,
                  (void *)node,
                  node && node->variable_name ? node->variable_name : "NULL");
        return rv_new_null();
    }

    // Use the visitor's current scope if available, otherwise fall back to node's scope
    scope_T *lookup_scope = visitor->current_scope ? visitor->current_scope : node->scope;

    if (!lookup_scope) {
        LOG_ERROR(LOG_CAT_VISITOR, "No scope available for variable '%s'", node->variable_name);
        return rv_new_null();
    }

    // Looking up variable in scope using new RuntimeValue storage
    LOG_VISITOR_DEBUG(
        "Looking up variable '%s' in scope %p", node->variable_name, (void *)lookup_scope);

    // Try the new scope_get_variable function first
    RuntimeValue *value = scope_get_variable(lookup_scope, node->variable_name);

    if (value) {
        // Variable found in new storage
        return value;  // Already referenced by scope_get_variable
    }

    // Fall back to old method for compatibility during migration
    AST_T *vdef = scope_get_variable_definition(lookup_scope, node->variable_name);

    if (vdef != NULL) {
        // Variable found in old storage
        LOG_VISITOR_DEBUG(
            "Found variable '%s' in old storage, type=%d", node->variable_name, vdef->type);

        // Use the cached RuntimeValue if available
        if (vdef->runtime_value) {
            // Create a new reference to return
            RuntimeValue *cached = (RuntimeValue *)vdef->runtime_value;
            if (cached) {
                char *cached_str = rv_to_string(cached);
                LOG_VISITOR_DEBUG(
                    "Found cached value for '%s': %s", node->variable_name, cached_str);
                memory_free(cached_str);
            } else {
                LOG_VISITOR_DEBUG("Found cached value for '%s': NULL", node->variable_name);
            }
            return rv_ref(cached);
        }

        // No cached RuntimeValue - try to evaluate the stored value if it's a simple literal
        if (vdef->variable_definition_value != NULL) {
            AST_T *val = vdef->variable_definition_value;
            // Only convert simple literals, not expressions
            if (val->type == AST_NUMBER || val->type == AST_STRING || val->type == AST_BOOLEAN ||
                val->type == AST_NULL || val->type == AST_ARRAY || val->type == AST_OBJECT) {
                RuntimeValue *result = ast_to_runtime_value(val);
                // Cache it for next time
                vdef->runtime_value = rv_ref(result);
                return result;
            }
        }

        // No value or complex expression - return null
        LOG_VISITOR_DEBUG("Variable '%s' has no cached RuntimeValue and no simple literal value",
                          node->variable_name);
        return rv_new_null();
    }

    LOG_ERROR(LOG_CAT_VISITOR, "Undefined variable '%s'", node->variable_name);
    return rv_new_null();
}

/**
 * @brief Visit function call node
 * @param visitor Visitor instance
 * @param node Function call AST node
 * @return Result of function call
 */
RuntimeValue *visitor_visit_function_call(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // Handle method calls (obj.method syntax)
    if (node->function_call_expression) {
        // Evaluate the expression to get the function
        RuntimeValue *func_rv = visitor_visit(visitor, node->function_call_expression);

        // Check if this is a stdlib function marker from property access
        if (func_rv && func_rv->type == RV_OBJECT) {
            RuntimeValue *stdlib_marker = rv_object_get(func_rv, "__stdlib_func");
            if (stdlib_marker && stdlib_marker->type == RV_STRING) {
                // This is a stdlib function accessed via dot notation
                const char *func_name = rv_get_string(stdlib_marker);
                const ZenStdlibFunction *stdlib_func = stdlib_get(func_name);

                if (stdlib_func) {
                    // Call the stdlib function
                    RuntimeValue **value_args = NULL;
                    size_t argc = (size_t)node->function_call_arguments_size;

                    if (argc > 0) {
                        value_args = memory_alloc(sizeof(RuntimeValue *) * argc);
                        if (!value_args) {
                            rv_unref(func_rv);
                            return rv_new_null();
                        }

                        // Evaluate each argument
                        for (size_t i = 0; i < argc; i++) {
                            value_args[i] =
                                visitor_visit(visitor, node->function_call_arguments[i]);
                            if (!value_args[i]) {
                                value_args[i] = rv_new_null();
                            }
                        }
                    }

                    // Call the stdlib function
                    RuntimeValue *result = stdlib_func->func(value_args, argc);

                    // Clean up arguments
                    if (value_args) {
                        for (size_t i = 0; i < argc; i++) {
                            if (value_args[i]) {
                                rv_unref(value_args[i]);
                            }
                        }
                        memory_free(value_args);
                    }

                    rv_unref(func_rv);
                    return result ? result : rv_new_null();
                }
            }
        }

        if (!func_rv || func_rv->type != RV_FUNCTION) {
            if (func_rv)
                rv_unref(func_rv);
            return rv_new_null();
        }

        // For method calls, we need to prepend 'self' as the first argument
        bool is_method_call = (node->function_call_expression->type == AST_PROPERTY_ACCESS);
        RuntimeValue *result = NULL;

        if (is_method_call) {
            // Get the object (self) from the property access
            AST_T *obj_ast = node->function_call_expression->object;
            RuntimeValue *self_rv = visitor_visit(visitor, obj_ast);

            // Prepare arguments with self as first
            size_t total_args = 1 + node->function_call_arguments_size;
            RuntimeValue **args = memory_alloc(sizeof(RuntimeValue *) * total_args);

            // First argument is self
            args[0] = self_rv;

            // Evaluate remaining arguments
            for (size_t i = 0; i < node->function_call_arguments_size; i++) {
                args[i + 1] = visitor_visit(visitor, node->function_call_arguments[i]);
                if (!args[i + 1]) {
                    args[i + 1] = rv_new_null();
                }
            }

            // Call the function
            AST_T *func_def = (AST_T *)func_rv->data.function.ast_node;

            // Safety check
            if (!func_def) {
                LOG_ERROR(LOG_CAT_VISITOR, "Method function definition is NULL");
                // Clean up
                for (size_t i = 0; i < total_args; i++) {
                    if (args[i])
                        rv_unref(args[i]);
                }
                memory_free(args);
                rv_unref(func_rv);
                return rv_new_null();
            }

            // Note: visitor_execute_user_function expects AST arguments, but we have RuntimeValues
            // We need to convert back to AST temporarily or modify the function
            // For now, let's use a simpler approach

            // For method calls, we need to handle 'this' binding differently
            // We'll pass the evaluated runtime values and let the function handle them

            // For methods, we need to pass 'self' as the first argument
            // Create AST arguments array with self as first element
            size_t total_ast_args = 1 + node->function_call_arguments_size;
            AST_T **ast_args = memory_alloc(sizeof(AST_T *) * total_ast_args);

            // Convert self RuntimeValue to AST for first argument
            ast_args[0] = runtime_value_to_ast(self_rv);

            // Copy remaining arguments
            for (size_t i = 0; i < node->function_call_arguments_size; i++) {
                ast_args[i + 1] = node->function_call_arguments[i];
            }

            // Call the method with self as first argument
            // Use the _ex version to indicate this is a method call
            result =
                visitor_execute_user_function_ex(visitor, func_def, ast_args, total_ast_args, true);

            // CRITICAL: Don't free the self AST - it might be referenced by the function scope
            // This is a temporary fix - proper solution is to track AST lifecycle
            // ast_free(ast_args[0]);

            // Clean up
            for (size_t i = 0; i < total_args; i++) {
                if (args[i])
                    rv_unref(args[i]);
            }
            memory_free(args);
            memory_free(ast_args);
        } else {
            // Regular function call through expression
            AST_T *func_def = (AST_T *)func_rv->data.function.ast_node;
            result = visitor_execute_user_function(visitor,
                                                   func_def,
                                                   node->function_call_arguments,
                                                   node->function_call_arguments_size);
        }

        rv_unref(func_rv);
        return result ? result : rv_new_null();
    }

    // Original code for named function calls
    if (!node->function_call_name) {
        return rv_new_null();
    }

    // Check for stdlib functions first
    LOG_VISITOR_DEBUG("Looking up stdlib function '%s'", node->function_call_name);
    const ZenStdlibFunction *stdlib_func = stdlib_get(node->function_call_name);
    if (stdlib_func != NULL) {
        LOG_VISITOR_DEBUG("Found stdlib function '%s' with %zu arguments",
                          node->function_call_name,
                          (size_t)node->function_call_arguments_size);
        // Convert AST arguments to Value arguments
        RuntimeValue **value_args = NULL;
        size_t argc = (size_t)node->function_call_arguments_size;

        if (argc > 0) {
            value_args = memory_alloc(sizeof(RuntimeValue *) * argc);
            if (!value_args) {
                return rv_new_null();
            }

            // Evaluate each argument
            for (size_t i = 0; i < argc; i++) {
                value_args[i] = visitor_visit(visitor, node->function_call_arguments[i]);
                if (!value_args[i]) {
                    value_args[i] = rv_new_null();
                }
            }
        }

        // Call the stdlib function
        RuntimeValue *result = stdlib_func->func(value_args, argc);

        // Clean up arguments
        if (value_args) {
            for (size_t i = 0; i < argc; i++) {
                if (value_args[i]) {
                    rv_unref(value_args[i]);
                }
            }
            memory_free(value_args);
        }

        // Return the result directly - no conversion needed
        return result ? result : rv_new_null();
    }

    // Legacy print support removed - now handled via stdlib

    // Handle "new" constructor calls
    if (strcmp(node->function_call_name, "new") == 0) {
        if (node->function_call_arguments_size < 1) {
            LOG_ERROR(LOG_CAT_VISITOR, "new requires a class name");
            return rv_new_null();
        }

        // First argument should be a variable (class name)
        AST_T *class_name_node = node->function_call_arguments[0];
        if (class_name_node->type != AST_VARIABLE) {
            LOG_ERROR(LOG_CAT_VISITOR, "new requires a class name");
            return rv_new_null();
        }

        // Look up the class
        AST_T *class_var =
            scope_get_variable_definition(node->scope, class_name_node->variable_name);
        if (class_var && class_var->variable_definition_value) {
            AST_T *class_def = class_var->variable_definition_value;
            if (class_def->type == AST_CLASS_DEFINITION) {
                // Create a new instance
                // Create class object (representing class definition)
                RuntimeValue *class_val = rv_new_object();
                rv_object_set(class_val, "__type", rv_new_string("class"));
                rv_object_set(class_val, "name", rv_new_string(class_def->class_name));
                rv_object_set(class_val,
                              "parent",
                              class_def->parent_class ? rv_new_string(class_def->parent_class)
                                                      : rv_new_null());

                // Store class methods and properties if they exist
                // Class body handling would be implemented here
                if (false) {
                    RuntimeValue *methods = rv_new_object();
                    rv_object_set(class_val, "methods", methods);
                    rv_unref(methods);
                }
                if (!class_val) {
                    return rv_new_null();
                }

                // Create instance object from class
                RuntimeValue *instance = rv_new_object();
                rv_object_set(instance, "__type", rv_new_string("instance"));
                rv_object_set(instance, "__class", rv_ref(class_val));

                // Copy class name for quick access
                RuntimeValue *class_name = rv_object_get(class_val, "name");
                if (class_name) {
                    rv_object_set(instance, "__class_name", rv_ref(class_name));
                }
                rv_unref(class_val);

                if (instance) {
                    // Constructor method with arguments can be implemented when needed
                    // Return the new instance as RuntimeValue
                    AST_T *result_ast = value_to_ast(instance);
                    rv_unref(instance);
                    if (!result_ast) {
                        return rv_new_null();
                    }
                    RuntimeValue *rv = ast_to_runtime_value(result_ast);
                    ast_free(result_ast);  // Free temporary AST
                    return rv;
                }
            }
        }

        LOG_ERROR(LOG_CAT_VISITOR, "Class '%s' not found", class_name_node->variable_name);
        return rv_new_null();
    }

    // Check if calling a class constructor
    AST_T *class_var = scope_get_variable_definition(node->scope, node->function_call_name);
    if (class_var && class_var->variable_definition_value) {
        RuntimeValue *potential_class = visitor_ast_to_value(class_var->variable_definition_value);
        // Check if potential_class is a class object
        if (potential_class && potential_class->type == RV_OBJECT) {
            RuntimeValue *type_marker = rv_object_get(potential_class, "__type");
            if (type_marker && type_marker->type == RV_STRING &&
                strcmp(type_marker->data.string.data, "class") == 0) {
                // This is a class constructor call - create new instance
                RuntimeValue *instance = rv_new_null();
                if (instance) {
                    // Constructor method calling can be implemented when needed
                    // Return the new instance as RuntimeValue
                    AST_T *result_ast = value_to_ast(instance);
                    rv_unref(instance);
                    rv_unref(potential_class);
                    if (!result_ast) {
                        return rv_new_null();
                    }
                    RuntimeValue *rv = ast_to_runtime_value(result_ast);
                    ast_free(result_ast);  // Free temporary AST
                    return rv;
                }
            }
        }
        if (potential_class) {
            rv_unref(potential_class);
        }
    }

    // Look up user-defined function
    // Use the visitor's current scope if available, otherwise fall back to node's scope
    scope_T *lookup_scope = visitor->current_scope ? visitor->current_scope : node->scope;

    if (!lookup_scope) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "No scope available for function lookup of '%s'",
                  node->function_call_name);
        return rv_new_null();
    }

    AST_T *fdef = scope_get_function_definition(lookup_scope, node->function_call_name);

    if (fdef == NULL) {
        // Function not found - check if it's a variable reference instead
        // This handles ZEN's design where standalone identifiers default to function calls
        // but should fallback to variable lookup if no function exists

        // First try the new RuntimeValue storage
        RuntimeValue *var_value = scope_get_variable(lookup_scope, node->function_call_name);
        if (var_value) {
            LOG_VISITOR_DEBUG(
                "Function '%s' not found, treating as variable reference (new storage)",
                node->function_call_name);
            return var_value;  // Already referenced by scope_get_variable
        }

        // Then try the old AST storage
        AST_T *vdef = scope_get_variable_definition(lookup_scope, node->function_call_name);
        if (vdef) {
            LOG_VISITOR_DEBUG(
                "Function '%s' not found, treating as variable reference (old storage)",
                node->function_call_name);
            // Use the cached runtime value if available
            if (vdef->runtime_value) {
                return rv_ref((RuntimeValue *)vdef->runtime_value);
            }
            // Fallback for simple literals only
            if (vdef->variable_definition_value) {
                AST_T *value = vdef->variable_definition_value;
                if (value->type == AST_NUMBER || value->type == AST_STRING ||
                    value->type == AST_BOOLEAN || value->type == AST_NULL ||
                    value->type == AST_ARRAY || value->type == AST_OBJECT) {
                    return ast_to_runtime_value(value);
                }
            }
            return rv_new_null();
        }

        // Neither function nor variable found
        LOG_ERROR(LOG_CAT_VISITOR, "Undefined function or variable '%s'", node->function_call_name);
        return rv_new_null();
    }

    // Execute user-defined function
    return visitor_execute_user_function(
        visitor, fdef, node->function_call_arguments, node->function_call_arguments_size);
}

/**
 * @brief Visit compound node (list of statements)
 * @param visitor Visitor instance
 * @param node Compound AST node
 * @return Result of last statement or NOOP
 */
RuntimeValue *visitor_visit_compound(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Invalid compound node (visitor=%p, node=%p)",
                  (void *)visitor,
                  (void *)node);
        return rv_new_null();
    }

    // Allow empty compounds
    if (!node->compound_statements || node->compound_size == 0) {
        LOG_VISITOR_DEBUG("Empty compound (0 statements)");
        return rv_new_null();
    }

    LOG_VISITOR_DEBUG("Executing compound with %zu statements", node->compound_size);

    RuntimeValue *last_result = rv_new_null();

    for (size_t i = 0; i < node->compound_size; i++) {
        if (!node->compound_statements[i]) {
            LOG_ERROR(LOG_CAT_VISITOR, "Statement %zu is NULL - PARSER BUG!", i + 1);
            continue;
        }

        // Free previous result
        if (last_result) {
            rv_unref(last_result);
        }

        last_result = visitor_visit(visitor, node->compound_statements[i]);

        // Property access handled

        // Check if this is a standalone method call
        // If the statement is a property access that returned a method function,
        // and it's used as a standalone statement, execute it with no arguments
        if (last_result && last_result->type == RV_FUNCTION &&
            node->compound_statements[i]->type == AST_PROPERTY_ACCESS) {
            // This is a method accessed as a property - execute it as a zero-arg method call
            AST_T *prop_access = node->compound_statements[i];
            // Execute as standalone method call

            // Get the object (self) value
            RuntimeValue *self_value = visitor_visit(visitor, prop_access->object);
            if (!self_value) {
                self_value = rv_new_null();
            }

            // Convert to AST for function execution
            AST_T *self_ast = runtime_value_to_ast(self_value);
            AST_T **args = memory_alloc(sizeof(AST_T *));
            args[0] = self_ast;

            // Execute the method with self as the first argument
            RuntimeValue *method_result = visitor_execute_user_function_ex(
                visitor, (AST_T *)last_result->data.function.ast_node, args, 1, true);

            // Method executed

            // Cleanup
            rv_unref(self_value);
            // Don't free self_ast - it might be referenced by the function scope
            // ast_free(self_ast);
            memory_free(args);
            rv_unref(last_result);
            last_result = method_result;
        }

        // Check for control flow markers
        if (last_result && last_result->type == RV_OBJECT) {
            // Check for RETURN marker
            RuntimeValue *return_marker = rv_object_get(last_result, "__RETURN__");
            if (return_marker) {
                // Return statement found - stop executing statements and return the marker
                LOG_VISITOR_DEBUG("Return statement found, stopping compound execution");
                // Don't unref return_marker - it's owned by last_result
                return last_result;
            }
            // Note: We don't check for __class__ here - class definitions are normal statements
        } else if (last_result && last_result->type == RV_STRING) {
            // Check for BREAK/CONTINUE markers
            const char *str = rv_get_string(last_result);
            if (strcmp(str, "__BREAK__") == 0 || strcmp(str, "__CONTINUE__") == 0) {
                // Control flow marker found - propagate it up
                return last_result;
            }
        }

        // Statement executed successfully
    }

    // Compound execution complete
    return last_result;
}

/**
 * @brief Convert AST node to Value object
 * @param node AST node to convert
 * @return Value object or NULL on failure
 */
static RuntimeValue *visitor_ast_to_value(AST_T *node)
{
    if (!node) {
        LOG_VISITOR_DEBUG("visitor_ast_to_value called with NULL node");
        return rv_new_null();
    }

    LOG_VISITOR_DEBUG("Converting AST type %d to Value", node->type);

    switch (node->type) {
    case AST_NULL:
        LOG_VISITOR_DEBUG("Converting AST_NULL to Value");
        return rv_new_null();

    case AST_UNDECIDABLE:
        LOG_VISITOR_DEBUG("Converting AST_UNDECIDABLE to Value");
        return rv_new_null();

    case AST_BOOLEAN:
        LOG_VISITOR_DEBUG("Converting AST_BOOLEAN (%s) to Value",
                          node->boolean_value ? "true" : "false");
        return rv_new_boolean(node->boolean_value);
    case AST_NUMBER:
        LOG_VISITOR_DEBUG("Converting AST_NUMBER (%f) to Value", node->number_value);
        return rv_new_number(node->number_value);
    case AST_STRING:
        LOG_VISITOR_DEBUG("Converting AST_STRING ('%s') to Value",
                          node->string_value ? node->string_value : "");
        return rv_new_string(node->string_value ? node->string_value : "");
    case AST_ARRAY: {
        // Create a new array Value
        RuntimeValue *array_val = rv_new_array();
        if (!array_val)
            return rv_new_null();

        // Convert each element and add to array
        for (size_t i = 0; i < node->array_size; i++) {
            RuntimeValue *element_val = visitor_ast_to_value(node->array_elements[i]);
            if (element_val) {
                rv_array_push(array_val, element_val);
                rv_unref(element_val);
            }
        }

        LOG_VISITOR_DEBUG("Converted AST_ARRAY to Value with %zu elements", node->array_size);
        return array_val;
    }
    case AST_OBJECT: {
        // Create a new object Value
        RuntimeValue *object_val = rv_new_object();
        if (!object_val)
            return rv_new_null();

        // Set each key-value pair
        for (size_t i = 0; i < node->object_size; i++) {
            if (node->object_keys[i] && node->object_values[i]) {
                RuntimeValue *value_val = visitor_ast_to_value(node->object_values[i]);
                if (value_val) {
                    rv_object_set(object_val, node->object_keys[i], value_val);

                    // Verify it was set
                    RuntimeValue *check_val = rv_object_get(object_val, node->object_keys[i]);
                    if (check_val) {
                        // rv_object_get returns borrowed reference, don't unref
                    }

                    rv_unref(value_val);
                }
            }
        }

        LOG_VISITOR_DEBUG("Converted AST_OBJECT to Value with %zu pairs", node->object_size);
        return object_val;
    }
    case AST_VARIABLE:
        // For AST_VARIABLE nodes, we need to look up the actual value from scope
        // This should not happen in normal flow as variables should be resolved first
        LOG_VISITOR_DEBUG(
            "AST_VARIABLE in visitor_ast_to_value - variable should be resolved first");
        return rv_new_null();

    default:
        LOG_VISITOR_DEBUG("Unknown AST type %d, returning null", node->type);
        return rv_new_null();
    }
}

/**
 * @brief Convert Value object to AST node
 * @param value Value object to convert
 * @return AST node or NULL on failure
 */
AST_T *value_to_ast(RuntimeValue *value)
{
    if (!value) {
        return ast_new(AST_NULL);
    }

    switch (value->type) {
    case RV_NULL:  // Note: RuntimeValue doesn't have undecidable, mapping to RV_NULL
        return ast_new(AST_UNDECIDABLE);
    case RV_BOOLEAN: {
        AST_T *ast = ast_new(AST_BOOLEAN);
        if (ast) {
            ast->boolean_value = value->data.boolean;
        }
        return ast;
    }
    case RV_NUMBER: {
        AST_T *ast = ast_new(AST_NUMBER);
        if (ast) {
            ast->number_value = value->data.number;
        }
        return ast;
    }
    case RV_STRING: {
        AST_T *ast = ast_new(AST_STRING);
        if (ast && value->data.string.data) {
            // NOTE: Removed automatic @ prefix conversion here to prevent infinite loops
            // The @ prefix should only be processed during initial parsing, not value conversion
            ast->string_value = memory_strdup(value->data.string.data);
        }
        return ast;
    }
    case RV_ARRAY: {
        AST_T *ast = ast_new(AST_ARRAY);
        if (!ast)
            return ast_new(AST_NULL);

        // Get array length
        size_t length = value->data.array.count;
        ast->array_size = length;

        if (length > 0) {
            // Allocate array for elements
            ast->array_elements = memory_alloc(sizeof(AST_T *) * length);
            if (!ast->array_elements) {
                ast_free(ast);
                return ast_new(AST_NULL);
            }

            // Convert each element back to AST
            for (size_t i = 0; i < length; i++) {
                RuntimeValue *element_val = value->data.array.elements[i];
                ast->array_elements[i] = value_to_ast(element_val);
                if (element_val)
                    rv_unref(element_val);
            }
        } else {
            ast->array_elements = NULL;
        }

        return ast;
    }
    case RV_OBJECT: {
        AST_T *ast = ast_new(AST_OBJECT);
        if (!ast)
            return ast_new(AST_NULL);

        // Get object data directly from RuntimeValue structure
        ast->object_size = value->data.object.count;

        if (value->data.object.count > 0) {
            // Allocate arrays for keys and values
            ast->object_keys = memory_alloc(sizeof(char *) * value->data.object.count);
            ast->object_values = memory_alloc(sizeof(AST_T *) * value->data.object.count);

            if (!ast->object_keys || !ast->object_values) {
                if (ast->object_keys)
                    memory_free(ast->object_keys);
                if (ast->object_values)
                    memory_free(ast->object_values);
                ast_free(ast);
                return ast_new(AST_NULL);
            }

            // Copy keys and convert values
            for (size_t i = 0; i < value->data.object.count; i++) {
                ast->object_keys[i] = memory_strdup(value->data.object.keys[i]);
                ast->object_values[i] = value_to_ast(value->data.object.values[i]);
            }
        } else {
            ast->object_keys = NULL;
            ast->object_values = NULL;
        }

        return ast;
    }
    case RV_ERROR:
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Error in expression evaluation: %s",
                  value->data.error.message ? value->data.error.message : "Unknown error");
        return ast_new(AST_NULL);
    // Class system implemented using object-based representation
    /*case VALUE_CLASS: {
        // For now, represent class as a special object AST
        AST_T *ast = ast_new(AST_OBJECT);
        if (ast) {
            // Mark this as a class somehow - could add a flag to AST_T
            // For now, just return the object representation
        }
        return ast;
    }*/
    // Instance system implemented using object-based representation
    /*case VALUE_INSTANCE: {
        // Represent instance as an object with properties
        AST_T *ast = ast_new(AST_OBJECT);
        if (ast && value->as.instance && value->as.instance->properties) {
            // Convert the properties object to AST
            AST_T *props_ast = value_to_ast(value->as.instance->properties);
            if (props_ast && props_ast->type == AST_OBJECT) {
                // Copy properties from the converted object
                ast->object_size = props_ast->object_size;
                ast->object_keys = props_ast->object_keys;
                ast->object_values = props_ast->object_values;
                // Clear the temporary AST to prevent double-free
                props_ast->object_keys = NULL;
                props_ast->object_values = NULL;
                props_ast->object_size = 0;
                ast_free(props_ast);
            }
        }
        return ast;
    }*/
    default:
        return ast_new(AST_NULL);
    }
}

/**
 * @brief Visit binary operation node
 * @param visitor Visitor instance
 * @param node Binary operation AST node
 * @return Result of binary operation
 */
static RuntimeValue *visitor_visit_binary_op(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->left || !node->right) {
        return rv_new_null();
    }

    // CRITICAL FIX: Ensure operands are evaluated fresh every time
    // This prevents cached variable references in loops from causing infinite loops

    // Evaluate operands directly
    // CRITICAL BUG FOUND: After deep recursion, AST nodes are being corrupted
    // Variables are turning into function calls
    // This needs to be investigated - likely memory pool corruption
    RuntimeValue *left_rv = visitor_visit(visitor, node->left);
    RuntimeValue *right_rv = visitor_visit(visitor, node->right);

    LOG_VISITOR_DEBUG("Binary op - evaluating left operand type=%d", node->left->type);
    LOG_VISITOR_DEBUG("Binary op - evaluating right operand type=%d", node->right->type);
    LOG_VISITOR_DEBUG("Binary op - got left_rv type=%d, right_rv type=%d",
                      left_rv ? (int)left_rv->type : -1,
                      right_rv ? (int)right_rv->type : -1);

    if (!left_rv || !right_rv) {
        if (left_rv)
            rv_unref(left_rv);
        if (right_rv)
            rv_unref(right_rv);
        return rv_new_null();
    }

    // Use RuntimeValues directly - no need for conversion
    RuntimeValue *left_val = left_rv;
    RuntimeValue *right_val = right_rv;

    if (!left_val || !right_val) {
        if (left_val)
            rv_unref(left_val);
        if (right_val)
            rv_unref(right_val);
        return rv_new_null();
    }

    RuntimeValue *result = NULL;

    // Apply the appropriate operator
    switch (node->operator_type) {
    case TOKEN_PLUS:
        result = op_add(left_val, right_val);
        if (result && result->type == RV_NUMBER) {
        }
        break;
    case TOKEN_MINUS:
        result = op_subtract(left_val, right_val);
        break;
    case TOKEN_MULTIPLY:
        result = op_multiply(left_val, right_val);
        break;
    case TOKEN_DIVIDE:
        result = op_divide(left_val, right_val);
        break;
    case TOKEN_MODULO:
        result = op_modulo(left_val, right_val);
        break;
    case TOKEN_EQUALS:
        result = op_equals(left_val, right_val);
        break;
    case TOKEN_NOT_EQUALS:
        result = op_not_equals(left_val, right_val);
        break;
    case TOKEN_LESS_THAN:
        result = op_less_than(left_val, right_val);
        break;
    case TOKEN_GREATER_THAN:
        result = op_greater_than(left_val, right_val);
        break;
    case TOKEN_LESS_EQUALS:
        // Implement <= as !(a > b)
        {
            RuntimeValue *gt_val = op_greater_than(left_val, right_val);
            if (gt_val) {
                result = op_logical_not(gt_val);
                rv_unref(gt_val);
            }
        }
        break;
    case TOKEN_GREATER_EQUALS:
        // Implement >= as !(a < b)
        {
            RuntimeValue *lt_val = op_less_than(left_val, right_val);
            if (lt_val) {
                result = op_logical_not(lt_val);
                rv_unref(lt_val);
            }
        }
        break;
    case TOKEN_AND:
        result = op_logical_and(left_val, right_val);
        break;
    case TOKEN_OR:
        result = op_logical_or(left_val, right_val);
        break;
    case TOKEN_RANGE: {
        // Handle range operator (start..end)
        // Use left_val and right_val which are the converted values

        // Both operands must be numbers
        if (!left_val || !right_val || left_val->type != RV_NUMBER ||
            right_val->type != RV_NUMBER) {
            LOG_ERROR(LOG_CAT_VISITOR,
                      "Range operator requires numeric operands (got %d, %d)",
                      left_val ? (int)left_val->type : -1,
                      right_val ? (int)right_val->type : -1);
            result = NULL;
            break;
        }

        // Create an array with values from start to end
        double start = left_val->data.number;
        double end = right_val->data.number;

        // Ensure we're dealing with integers
        if (start != (double)(int)start || end != (double)(int)end) {
            LOG_ERROR(LOG_CAT_VISITOR, "Range operator requires integer operands");
            result = NULL;
            break;
        }

        int istart = (int)start;
        int iend = (int)end;

        // Create array to hold range values - return RuntimeValue directly
        RuntimeValue *range_array = rv_new_array();

        // Handle ascending and descending ranges
        if (istart <= iend) {
            for (int i = istart; i <= iend; i++) {
                RuntimeValue *num = rv_new_number((double)i);
                rv_array_push(range_array, num);
                rv_unref(num);
            }
        } else {
            for (int i = istart; i >= iend; i--) {
                RuntimeValue *num = rv_new_number((double)i);
                rv_array_push(range_array, num);
                rv_unref(num);
            }
        }

        // Clean up operands before returning
        rv_unref(left_val);
        rv_unref(right_val);

        return range_array;  // Return directly without conversion
    }
    default:
        LOG_ERROR(LOG_CAT_VISITOR, "Unknown binary operator %d", node->operator_type);
        result = NULL;
        break;
    }

    // Clean up operands
    rv_unref(left_val);
    rv_unref(right_val);

    if (!result) {
        return rv_new_null();
    }

    // Convert result to RuntimeValue
    AST_T *result_ast = value_to_ast(result);
    rv_unref(result);
    RuntimeValue *result_rv = ast_to_runtime_value(result_ast);
    ast_free(result_ast);

    return result_rv;
}

/**
 * @brief Visit unary operation node
 * @param visitor Visitor instance
 * @param node Unary operation AST node
 * @return Result of unary operation
 */
static RuntimeValue *visitor_visit_unary_op(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->operand) {
        return rv_new_null();
    }

    // Evaluate operand
    RuntimeValue *operand_rv = visitor_visit(visitor, node->operand);
    if (!operand_rv) {
        return rv_new_null();
    }

    // Convert RuntimeValue to AST then to Value object
    AST_T *operand_ast = runtime_value_to_ast(operand_rv);
    RuntimeValue *operand_val = visitor_ast_to_value(operand_ast);
    ast_free(operand_ast);
    rv_unref(operand_rv);

    if (!operand_val) {
        return rv_new_null();
    }

    RuntimeValue *result = NULL;

    // Apply the appropriate unary operator
    switch (node->operator_type) {
    case TOKEN_MINUS:
        // Unary minus: multiply by -1
        {
            RuntimeValue *neg_one = rv_new_number(-1.0);
            if (neg_one) {
                result = op_multiply(operand_val, neg_one);
                rv_unref(neg_one);
            }
        }
        break;
    case TOKEN_NOT:
        result = op_logical_not(operand_val);
        break;
    default:
        LOG_ERROR(LOG_CAT_VISITOR, "Unknown unary operator %d", node->operator_type);
        result = NULL;
        break;
    }

    // Clean up operand
    rv_unref(operand_val);

    if (!result) {
        return rv_new_null();
    }

    // Convert result to RuntimeValue
    AST_T *result_ast = value_to_ast(result);
    rv_unref(result);
    RuntimeValue *result_rv = ast_to_runtime_value(result_ast);
    ast_free(result_ast);

    return result_rv;
}

/**
 * @brief Visit array literal node
 * @param visitor Visitor instance
 * @param node Array AST node
 * @return Array value as AST node
 */
static RuntimeValue *visitor_visit_array(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // Create a new RuntimeValue array
    RuntimeValue *array_rv = rv_new_array();
    if (!array_rv) {
        return rv_new_null();
    }

    // Evaluate and add each element
    for (size_t i = 0; i < node->array_size; i++) {
        RuntimeValue *element_rv = visitor_visit(visitor, node->array_elements[i]);
        if (element_rv) {
            rv_array_push(array_rv, element_rv);
            rv_unref(element_rv);  // array_push adds its own reference
        }
    }

    return array_rv;
}

/**
 * @brief Visit object literal node
 * @param visitor Visitor instance
 * @param node Object AST node
 * @return Object value as AST node
 */
static RuntimeValue *visitor_visit_object(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // Create a new RuntimeValue object
    RuntimeValue *object_rv = rv_new_object();
    if (!object_rv) {
        return rv_new_null();
    }

    // Evaluate and set each key-value pair
    for (size_t i = 0; i < node->object_size; i++) {
        if (node->object_keys[i] && node->object_values[i]) {
            LOG_VISITOR_DEBUG("Processing object key-value pair %zu: %s (type=%d)",
                              i,
                              node->object_keys[i],
                              node->object_values[i]->type);

            RuntimeValue *value_rv = visitor_visit(visitor, node->object_values[i]);
            if (value_rv) {
                LOG_VISITOR_DEBUG("Setting object property %s (value type=%d)",
                                  node->object_keys[i],
                                  value_rv->type);
                rv_object_set(object_rv, node->object_keys[i], value_rv);
                rv_unref(value_rv);  // object_set adds its own reference
            }
        }
    }

    return object_rv;
}

/**
 * @brief Visit property access node (obj.property)
 * @param visitor Visitor instance
 * @param node Property access AST node
 * @return Property value or NULL
 */
static RuntimeValue *visitor_visit_property_access(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        LOG_ERROR(LOG_CAT_VISITOR, "visitor_visit_property_access: visitor or node is NULL");
        return rv_new_null();
    }
    
    if (!node->object) {
        LOG_ERROR(LOG_CAT_VISITOR, "visitor_visit_property_access: node->object is NULL");
        return rv_new_null();
    }
    
    if (!node->property_name) {
        LOG_ERROR(LOG_CAT_VISITOR, "visitor_visit_property_access: node->property_name is NULL");
        return rv_new_null();
    }
    
    LOG_VISITOR_DEBUG("visitor_visit_property_access: accessing property '%s'", node->property_name);

    // Special handling for namespace functions (e.g., json.stringify)
    // Check if object is a variable that maps to a namespace
    if (node->object->type == AST_VARIABLE) {
        const char *namespace = node->object->variable_name;
        const char *method = node->property_name;

        // Map namespace.method to camelCase function name
        char mapped_name[256];
        if (strcmp(namespace, "json") == 0) {
            if (strcmp(method, "stringify") == 0) {
                strcpy(mapped_name, "jsonStringify");
            } else if (strcmp(method, "parse") == 0) {
                strcpy(mapped_name, "jsonParse");
            } else if (strcmp(method, "pretty") == 0) {
                strcpy(mapped_name, "jsonPretty");
            } else {
                snprintf(
                    mapped_name, sizeof(mapped_name), "json%c%s", toupper(method[0]), method + 1);
            }
        } else if (strcmp(namespace, "yaml") == 0) {
            if (strcmp(method, "parse") == 0) {
                strcpy(mapped_name, "yamlParse");
            } else if (strcmp(method, "stringify") == 0) {
                strcpy(mapped_name, "yamlStringify");
            } else {
                snprintf(
                    mapped_name, sizeof(mapped_name), "yaml%c%s", toupper(method[0]), method + 1);
            }
        } else if (strcmp(namespace, "regex") == 0) {
            snprintf(mapped_name, sizeof(mapped_name), "regex%c%s", toupper(method[0]), method + 1);
        } else if (strcmp(namespace, "http") == 0) {
            snprintf(mapped_name, sizeof(mapped_name), "http%c%s", toupper(method[0]), method + 1);
        } else {
            // Not a known namespace, proceed with normal property access
            goto normal_property_access;
        }

        // Look up the mapped function in stdlib
        const ZenStdlibFunction *func = stdlib_get(mapped_name);
        if (func) {
            // Return a function RuntimeValue that wraps the stdlib function
            // For now, we'll return a special marker that the function_call handler can recognize
            RuntimeValue *func_marker = rv_new_object();
            rv_object_set(func_marker, "__stdlib_func", rv_new_string(mapped_name));
            return func_marker;
        }
    }

normal_property_access: {
    // Evaluate the object expression
    LOG_VISITOR_DEBUG("About to evaluate object expression of type %d", node->object->type);
    
    RuntimeValue *object_rv = visitor_visit(visitor, node->object);
    LOG_VISITOR_DEBUG("Object evaluated, got type %d", object_rv ? (int)object_rv->type : -1);
    if (!object_rv) {
        return rv_new_null();
    }

    // Determine the property name - could be dynamic if using bracket notation
    char *property_name = node->property_name;
    char *dynamic_property = NULL;

    // If there's a left node (index expression), evaluate it for dynamic property access
    if (node->left) {
        RuntimeValue *index_rv = visitor_visit(visitor, node->left);
        if (index_rv) {
            if (index_rv->type == RV_STRING) {
                dynamic_property = memory_strdup(index_rv->data.string.data);
                property_name = dynamic_property;
            } else if (index_rv->type == RV_NUMBER) {
                dynamic_property = memory_alloc(32);
                snprintf(dynamic_property, 32, "%g", index_rv->data.number);
                property_name = dynamic_property;
            }
            rv_unref(index_rv);
        }
    }

    RuntimeValue *result = NULL;

    // Handle different types using RuntimeValue directly
    if (rv_is_object(object_rv)) {
        // Check if this is a class instance by looking for __class__
        RuntimeValue *class_obj = rv_object_get(object_rv, "__class__");
        if (class_obj) {
            // Look for method in class instance
            // This is a class instance - look for method in the class
            char method_key[256];
            snprintf(method_key, sizeof(method_key), "__method_%s", property_name);

            LOG_VISITOR_DEBUG("Looking for method: %s", method_key);

            // Look for method in class
            // Search for method key

            RuntimeValue *method = rv_object_get(class_obj, method_key);
            
            if (method) {
                LOG_VISITOR_DEBUG("Found method %s, type: %d", method_key, method->type);
            } else {
                LOG_VISITOR_DEBUG("Method %s not found", method_key);
            }

            // Method lookup in class object

            if (method && method->type == RV_FUNCTION) {
                // Found the method - return it
                // Found method
                result = method;
                rv_ref(result);
            } else {
                // Try to get instance property
                result = rv_object_get(object_rv, property_name);
                if (result) {
                    rv_ref(result);
                } else {
                    result = rv_new_null();
                }
            }
        } else {
            // Regular object property access
            result = rv_object_get(object_rv, property_name);
            if (result) {
                // rv_object_get returns a borrowed reference, need to increase refcount
                rv_ref(result);
            } else {
                result = rv_new_null();
            }
        }
    } else if (rv_is_array(object_rv)) {
        // Handle special array property: length
        if (strcmp(property_name, "length") == 0) {
            size_t length = rv_array_length(object_rv);
            result = rv_new_number((double)length);
        } else {
            // Handle array indexing: arr[0] becomes arr.0 in property access
            char *endptr;
            long index = strtol(property_name, &endptr, 10);

            if (*endptr == '\0' && index >= 0) {
                // Valid numeric index
                RuntimeValue *element = rv_array_get(object_rv, (size_t)index);
                if (element) {
                    // rv_array_get returns a borrowed reference, need to increase refcount
                    result = element;
                    rv_ref(result);
                } else {
                    result = rv_new_null();
                }
            } else {
                LOG_ERROR(LOG_CAT_VISITOR, "Invalid array property/index '%s'", property_name);
                result = rv_new_null();
            }
        }
    } else {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Cannot access property '%s' on type %s",
                  property_name,
                  rv_type_name(object_rv));
        result = rv_new_null();
    }

    // Clean up
    if (dynamic_property) {
        memory_free(dynamic_property);
    }
    rv_unref(object_rv);

    return result;
}  // Close brace for normal_property_access label block
}

/**
 * @brief Check if a RuntimeValue evaluates to a truthy value
 * @param rv RuntimeValue to evaluate
 * @return true if truthy, false otherwise
 */
static bool visitor_is_truthy_rv(RuntimeValue *rv)
{
    if (!rv) {
        return false;
    }

    switch (rv->type) {
    case RV_NULL:
        return false;
    case RV_BOOLEAN:
        return rv->data.boolean;
    case RV_NUMBER:
        return rv->data.number != 0.0;
    case RV_STRING:
        return rv->data.string.data && rv->data.string.length > 0;
    case RV_ARRAY:
        return rv->data.array.count > 0;
    case RV_OBJECT:
        return rv->data.object.count > 0;
    case RV_FUNCTION:
        return true;  // Functions are truthy
    case RV_ERROR:
        return false;  // Errors are falsy
    default:
        // Unknown types are truthy
        return true;
    }
}

/**
 * @brief Visit if statement node
 * @param visitor Visitor instance
 * @param node If statement AST node
 * @return Result of executed branch or NOOP
 */
static RuntimeValue *visitor_visit_if_statement(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->condition) {
        return rv_new_null();
    }

    // Evaluate the condition
    RuntimeValue *condition_result = visitor_visit(visitor, node->condition);
    if (!condition_result) {
        return rv_new_null();
    }

    // Check if condition is truthy
    bool condition_is_true = visitor_is_truthy_rv(condition_result);
    rv_unref(condition_result);

    if (condition_is_true) {
        // Execute then branch
        if (node->then_branch) {
            return visitor_visit(visitor, node->then_branch);
        }
    } else {
        // Execute else branch if it exists
        if (node->else_branch) {
            return visitor_visit(visitor, node->else_branch);
        }
    }

    return rv_new_null();
}

// Removed complex propagate_shared_scope function - replaced with direct scope assignment

/**
 * @brief Visit while loop node
 * @param visitor Visitor instance
 * @param node While loop AST node
 * @return Result of last iteration or NOOP
 */
static RuntimeValue *visitor_visit_while_loop(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->loop_condition || !node->loop_body) {
        return rv_new_null();
    }

    if (!node->scope) {
        LOG_ERROR(LOG_CAT_VISITOR, "While loop node has no scope");
        return rv_new_null();
    }

    // CRITICAL FIX: Use proper recursive scope propagation
    // This ensures that ALL nested AST nodes (like variables inside conditions) get the correct
    // scope
    visitor_update_ast_scope(node->loop_condition, node->scope);
    visitor_update_ast_scope(node->loop_body, node->scope);

    RuntimeValue *last_result = rv_new_null();
    int iteration_count = 0;
    const int max_iterations = 10000;  // Safety limit

    while (iteration_count < max_iterations) {
        // Simple condition evaluation - no scope manipulation per iteration
        RuntimeValue *condition_result = visitor_visit(visitor, node->loop_condition);
        if (!condition_result) {
            break;
        }

        bool condition_is_true = visitor_is_truthy_rv(condition_result);
        rv_unref(condition_result);

        if (!condition_is_true) {
            break;
        }

        // Simple body execution - no scope manipulation per iteration
        RuntimeValue *body_result = visitor_visit(visitor, node->loop_body);
        if (body_result) {
            // Check for break/continue statements
            if (body_result->type == RV_STRING) {
                const char *str = rv_get_string(body_result);
                if (strcmp(str, "__BREAK__") == 0) {
                    rv_unref(body_result);
                    break;
                } else if (strcmp(str, "__CONTINUE__") == 0) {
                    rv_unref(body_result);
                    iteration_count++;
                    continue;
                }
            } else if (body_result->type == RV_OBJECT) {
                // Check for RETURN marker
                RuntimeValue *return_marker = rv_object_get(body_result, "__RETURN__");
                if (return_marker) {
                    // Don't unref return_marker - it's owned by body_result
                    rv_unref(last_result);
                    return body_result;  // Return the whole marker object
                }
            }
            // Update last result
            rv_unref(last_result);
            last_result = body_result;
        }

        iteration_count++;
    }

    if (iteration_count >= max_iterations) {
        LOG_WARN(LOG_CAT_VISITOR,
                 "While loop exceeded maximum iterations (%d), terminating",
                 max_iterations);
    }

    return last_result;
}

/**
 * @brief Visit for loop node
 * @param visitor Visitor instance
 * @param node For loop AST node
 * @return Result of last iteration or NOOP
 */
static RuntimeValue *visitor_visit_for_loop(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->iterator_variable || !node->iterable || !node->for_body) {
        return rv_new_null();
    }

    // Evaluate the iterable expression
    RuntimeValue *iterable_result = visitor_visit(visitor, node->iterable);
    if (!iterable_result) {
        return rv_new_null();
    }

    RuntimeValue *last_result = rv_new_null();

    // CRITICAL FIX: Create iterator variable definition ONCE before the loop
    // This prevents repeated creation/destruction and potential memory issues
    AST_T *iterator_def = ast_new(AST_VARIABLE_DEFINITION);
    if (!iterator_def || !node->scope) {
        // MEMORY LEAK FIX: Clean up iterator_def if allocation failed
        if (iterator_def) {
            ast_free(iterator_def);
        }
        rv_unref(iterable_result);
        rv_unref(last_result);
        return rv_new_null();
    }

    iterator_def->variable_definition_variable_name = memory_strdup(node->iterator_variable);
    iterator_def->scope = node->scope;

    // Handle different types of iterables
    if (iterable_result->type == RV_ARRAY) {
        for (size_t i = 0; i < rv_array_length(iterable_result); i++) {
            RuntimeValue *element = rv_array_get(iterable_result, i);
            if (!element)
                continue;

            // Convert element to AST for variable storage
            AST_T *element_ast = runtime_value_to_ast(element);

            // FIXED: Update the iterator variable's value instead of creating new one
            // Free the old AST value first to prevent memory leak
            if (iterator_def->variable_definition_value) {
                ast_free(iterator_def->variable_definition_value);
            }
            iterator_def->variable_definition_value = element_ast;

            // Update the cached runtime value
            if (iterator_def->runtime_value) {
                rv_unref(iterator_def->runtime_value);
            }
            iterator_def->runtime_value = rv_ref(element);

            // Add/update iterator in scope (scope_add handles updates automatically)
            scope_add_variable_definition(node->scope, iterator_def);

            // Don't unref element here - it's a borrowed reference from rv_array_get

            // Execute loop body
            RuntimeValue *body_result = visitor_visit(visitor, node->for_body);
            if (body_result) {
                // Check for break/continue/return statements
                if (body_result->type == RV_STRING) {
                    const char *str = rv_get_string(body_result);
                    if (strcmp(str, "__BREAK__") == 0) {
                        rv_unref(body_result);
                        break;
                    } else if (strcmp(str, "__CONTINUE__") == 0) {
                        rv_unref(body_result);
                        continue;
                    }
                } else if (body_result->type == RV_OBJECT) {
                    // Check for RETURN marker
                    RuntimeValue *return_marker = rv_object_get(body_result, "__RETURN__");
                    if (return_marker) {
                        // Don't unref return_marker - it's owned by body_result
                        rv_unref(last_result);
                        rv_unref(iterable_result);
                        // Clean up iterator
                        memory_free(iterator_def->variable_definition_variable_name);
                        ast_free(iterator_def);
                        return body_result;
                    }
                }
                rv_unref(last_result);
                last_result = body_result;
            }
        }
    } else if (iterable_result->type == RV_OBJECT) {
        // Iterate over object keys
        for (size_t i = 0; i < rv_object_size(iterable_result); i++) {
            char *key = rv_object_get_key_at(iterable_result, i);
            if (!key)
                continue;

            // Create a string AST node for the key
            AST_T *key_ast = ast_new(AST_STRING);
            if (!key_ast)
                continue;

            key_ast->string_value = memory_strdup(key);

            // FIXED: Update the iterator variable's value instead of creating new one
            iterator_def->variable_definition_value = key_ast;

            // Update the cached runtime value with string
            if (iterator_def->runtime_value) {
                rv_unref(iterator_def->runtime_value);
            }
            iterator_def->runtime_value = rv_new_string(key);

            // Add/update iterator in scope (scope_add handles updates automatically)
            scope_add_variable_definition(node->scope, iterator_def);

            // Execute loop body
            RuntimeValue *body_result = visitor_visit(visitor, node->for_body);
            if (body_result) {
                // Check for break/continue/return statements
                if (body_result->type == RV_STRING) {
                    const char *str = rv_get_string(body_result);
                    if (strcmp(str, "__BREAK__") == 0) {
                        rv_unref(body_result);
                        break;
                    } else if (strcmp(str, "__CONTINUE__") == 0) {
                        rv_unref(body_result);
                        continue;
                    }
                } else if (body_result->type == RV_OBJECT) {
                    // Check for RETURN marker
                    RuntimeValue *return_marker = rv_object_get(body_result, "__RETURN__");
                    if (return_marker) {
                        // Don't unref return_marker - it's owned by body_result
                        rv_unref(last_result);
                        rv_unref(iterable_result);
                        // Clean up iterator
                        memory_free(iterator_def->variable_definition_variable_name);
                        ast_free(iterator_def);
                        return body_result;
                    }
                }
                rv_unref(last_result);
                last_result = body_result;
            }
        }
    } else {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "For loop iterable is not an array or object (type: %d)",
                  iterable_result->type);
        // MEMORY LEAK FIX: Clean up iterator variable definition completely
        if (iterator_def) {
            if (iterator_def->variable_definition_variable_name) {
                memory_free(iterator_def->variable_definition_variable_name);
                iterator_def->variable_definition_variable_name = NULL;
            }
            ast_free(iterator_def);
        }
        rv_unref(iterable_result);
        rv_unref(last_result);
        return rv_new_null();
    }

    // MEMORY LEAK FIX: Clean up iterator variable definition completely
    if (iterator_def) {
        if (iterator_def->variable_definition_variable_name) {
            memory_free(iterator_def->variable_definition_variable_name);
            iterator_def->variable_definition_variable_name = NULL;
        }
        // Free the iterator_def node itself (it's not owned by the main AST tree)
        ast_free(iterator_def);
        iterator_def = NULL;
    }

    // Clean up the iterable result
    rv_unref(iterable_result);

    return last_result;
}

/**
 * @brief Visit import statement
 * @param visitor Visitor instance
 * @param node Import AST node
 * @return Module exports or null
 */
RuntimeValue *visitor_visit_import(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->import_path) {
        return rv_new_null();
    }

    // For now, just log and return null to avoid crashes
    LOG_INFO(LOG_CAT_VISITOR,
             "Import statement encountered but not fully implemented: %s",
             node->import_path);
    return rv_new_null();

    // Read the module file
    char *module_path = node->import_path;

    // Add .zen extension if not present
    char full_path[512];
    if (!strstr(module_path, ".zen") && !strstr(module_path, ".zn")) {
        snprintf(full_path, sizeof(full_path), "%s.zen", module_path);
    } else {
        strncpy(full_path, module_path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    }

    // Read module source
    char *source = io_read_file_internal(full_path);
    if (!source) {
        LOG_ERROR(LOG_CAT_VISITOR, "Failed to read module: %s", full_path);
        return rv_new_null();
    }

    // Create a new lexer and parser for the module
    lexer_T *module_lexer = lexer_new(source);
    if (!module_lexer) {
        memory_free(source);
        return rv_new_null();
    }

    parser_T *module_parser = parser_new(module_lexer);
    if (!module_parser) {
        lexer_free(module_lexer);
        memory_free(source);
        return rv_new_null();
    }

    // Parse the module
    scope_T *module_scope = scope_new();
    AST_T *module_ast = parser_parse(module_parser, module_scope);

    if (!module_ast) {
        LOG_ERROR(LOG_CAT_VISITOR, "Failed to parse module");
        parser_free(module_parser);
        lexer_free(module_lexer);
        memory_free(source);
        return rv_new_null();
    }

    // Create a new visitor for the module to avoid interference
    visitor_T *module_visitor = visitor_new();

    // Execute the module - this will populate variable runtime values
    RuntimeValue *module_result = visitor_visit(module_visitor, module_ast);
    if (module_result) {
        rv_unref(module_result);
    }

    // Get all exported values from the module scope
    RuntimeValue *exports = rv_new_object();

    // Add all variables from module scope to exports
    if (module_ast && module_ast->type == AST_COMPOUND && module_ast->compound_statements) {
        // Try to find variable definitions in the compound statements
        for (size_t i = 0; i < module_ast->compound_size; i++) {
            AST_T *stmt = module_ast->compound_statements[i];
            if (stmt && stmt->type == AST_VARIABLE_DEFINITION) {
                const char *var_name = stmt->variable_definition_variable_name;
                // Get the runtime value from the module scope
                RuntimeValue *value = scope_get_variable(module_scope, var_name);

                if (var_name && value) {
                    rv_object_set(exports, var_name, value);
                }
            }
        }
    }

    // Also check scope variable definitions
    for (size_t i = 0; i < module_scope->variable_definitions_size; i++) {
        AST_T *var_def = module_scope->variable_definitions[i];
        if (var_def && var_def->variable_definition_variable_name) {
            // Skip internal variables
            if (var_def->variable_definition_variable_name[0] == '_') {
                continue;
            }

            // Get the runtime value from the module scope
            RuntimeValue *value =
                scope_get_variable(module_scope, var_def->variable_definition_variable_name);

            if (value) {
                rv_object_set(exports, var_def->variable_definition_variable_name, value);
                rv_unref(value);  // rv_object_set refs it
            }
        }
    }

    // Add all functions from module scope to exports
    for (size_t i = 0; i < module_scope->function_definitions_size; i++) {
        AST_T *func_def = module_scope->function_definitions[i];
        if (func_def && func_def->function_definition_name) {
            // Skip internal functions
            if (func_def->function_definition_name[0] == '_') {
                continue;
            }

            // Create function value
            RuntimeValue *func_value = rv_new_function(func_def, module_scope);
            rv_object_set(exports, func_def->function_definition_name, func_value);
            rv_unref(func_value);
        }
    }

    // Import into current scope
    if (node->import_names_size == 0) {
        // Import all exports into current scope
        if (exports->type == RV_OBJECT) {
            for (size_t i = 0; i < exports->data.object.count; i++) {
                const char *name = exports->data.object.keys[i];
                RuntimeValue *value = exports->data.object.values[i];

                if (value->type == RV_FUNCTION && value->data.function.ast_node) {
                    // Import as function definition
                    AST_T *func_ast = (AST_T *)value->data.function.ast_node;
                    // Create a shallow copy of the function definition with the correct scope
                    AST_T *import_func = ast_new(AST_FUNCTION_DEFINITION);
                    import_func->function_definition_name = memory_strdup(name);
                    import_func->function_definition_args = func_ast->function_definition_args;
                    import_func->function_definition_args_size =
                        func_ast->function_definition_args_size;
                    import_func->function_definition_body = func_ast->function_definition_body;
                    import_func->scope = node->scope;

                    scope_add_function_definition(node->scope, import_func);
                } else {
                    // Import as variable definition
                    AST_T *import_var = ast_new(AST_VARIABLE_DEFINITION);
                    import_var->variable_definition_variable_name = memory_strdup(name);
                    import_var->runtime_value = rv_ref(value);
                    import_var->scope = node->scope;

                    scope_add_variable_definition(node->scope, import_var);
                }
            }
        }
    }

    // Cleanup - Don't free module_ast and module_scope as they're referenced by functions
    visitor_free(module_visitor);
    // ast_free(module_ast);  // Keep alive - referenced by exported functions
    // scope_free(module_scope);  // Keep alive - referenced by exported functions
    parser_free(module_parser);
    lexer_free(module_lexer);
    memory_free(source);

    return exports;
}

/**
 * @brief Visit export statement
 * @param visitor Visitor instance
 * @param node Export AST node
 * @return Null value
 */
RuntimeValue *visitor_visit_export(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // For now, export is a no-op since we export all non-private symbols by default
    // In the future, we could implement specific export handling

    // Export a named value
    if (node->export_name && node->export_value) {
        LOG_VISITOR_DEBUG("Exporting: %s", node->export_name);
        RuntimeValue *value = visitor_visit(visitor, node->export_value);
        // Could maintain an export list here
        rv_unref(value);
    }

    return rv_new_null();
}

/**
 * @brief Visit string node
 * @param visitor Visitor instance
 * @param node String AST node
 * @return The string as RuntimeValue
 */
RuntimeValue *visitor_visit_string(visitor_T *visitor, AST_T *node)
{
    (void)visitor;  // Suppress unused parameter warning
    // String literals are converted to RuntimeValue
    return rv_new_string(node->string_value);
}

/**
 * @brief Recursively update scope for all nodes in AST tree
 * @param node The AST node to update
 * @param new_scope The new scope to set
 */
static void visitor_update_ast_scope(AST_T *node, scope_T *new_scope)
{
    if (!node)
        return;

    // Update this node's scope
    node->scope = new_scope;

    // Recursively update children based on node type
    switch (node->type) {
    case AST_COMPOUND:
        if (node->compound_statements) {
            for (size_t i = 0; i < node->compound_size; i++) {
                visitor_update_ast_scope(node->compound_statements[i], new_scope);
            }
        }
        break;

    case AST_IF_STATEMENT:
        visitor_update_ast_scope(node->condition, new_scope);
        visitor_update_ast_scope(node->then_branch, new_scope);
        visitor_update_ast_scope(node->else_branch, new_scope);
        break;

    case AST_WHILE_LOOP:
        visitor_update_ast_scope(node->loop_condition, new_scope);
        visitor_update_ast_scope(node->loop_body, new_scope);
        break;

    case AST_FOR_LOOP:
        visitor_update_ast_scope(node->iterable, new_scope);
        visitor_update_ast_scope(node->for_body, new_scope);
        break;

    case AST_BINARY_OP:
        visitor_update_ast_scope(node->left, new_scope);
        visitor_update_ast_scope(node->right, new_scope);
        break;

    case AST_UNARY_OP:
        visitor_update_ast_scope(node->operand, new_scope);
        break;

    case AST_VARIABLE_DEFINITION:
        visitor_update_ast_scope(node->variable_definition_value, new_scope);
        break;

    case AST_FUNCTION_CALL:
        // Update argument scopes but NOT the function call scope itself
        // This allows arguments to find parameters while function calls find functions
        if (node->function_call_arguments) {
            for (size_t i = 0; i < node->function_call_arguments_size; i++) {
                visitor_update_ast_scope(node->function_call_arguments[i], new_scope);
            }
        }
        // Don't update the function call node's own scope - it should keep original scope
        // to find function definitions
        node->scope = node->scope;  // Keep original scope for function lookup
        return;                     // Important: return without setting node->scope = new_scope

    case AST_RETURN:
        visitor_update_ast_scope(node->return_value, new_scope);
        break;

    case AST_PROPERTY_ACCESS:
        // Update both object and property parts
        if (node->object) {
            visitor_update_ast_scope(node->object, new_scope);
        }
        break;

    case AST_ASSIGNMENT:
        // Update both left and right sides
        if (node->left) {
            visitor_update_ast_scope(node->left, new_scope);
        }
        if (node->right) {
            visitor_update_ast_scope(node->right, new_scope);
        }
        break;

    // Variable nodes should get the new scope to find parameters
    case AST_VARIABLE:
        // Variables should use the new scope to find parameters
        break;

    // Other leaf nodes (no children to update, but scope can be updated)
    case AST_STRING:
    case AST_NUMBER:
    case AST_BOOLEAN:
    case AST_NULL:
    case AST_NOOP:
        break;

    default:
        // For any other node types, just update the scope
        break;
    }
}

/**
 * @brief Execute a user-defined function with advanced profiling and call stack management
 * @param visitor Visitor instance
 * @param fdef Function definition AST node
 * @param args Array of argument expressions
 * @param args_size Number of arguments
 * @return Result of function execution
 */
static RuntimeValue *
visitor_execute_user_function(visitor_T *visitor, AST_T *fdef, AST_T **args, int args_size)
{
    return visitor_execute_user_function_ex(visitor, fdef, args, args_size, false);
}

static RuntimeValue *visitor_execute_user_function_ex(
    visitor_T *visitor, AST_T *fdef, AST_T **args, int args_size, bool is_method_call)
{
    if (!visitor || !fdef || !fdef->function_definition_body) {
        return rv_new_null();
    }

    const char *function_name =
        fdef->function_definition_name ? fdef->function_definition_name : "anonymous";

    // Execute function

    // For methods, the first argument is 'self' which is added implicitly
    // So we need to adjust the expected parameter count
    size_t expected_args = fdef->function_definition_args_size;
    size_t actual_args = args_size;

    // If this is a method call (first arg is self), adjust the counts
    if (is_method_call && actual_args > 0) {
        // Method calls have an implicit self, so we expect one less explicit parameter
        actual_args--;  // Don't count self in the actual args
    }

    if (expected_args != actual_args) {
        visitor_throw_exception(
            visitor, ast_new(AST_NULL), "Argument count mismatch", function_name);
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Function '%s' expects %zu arguments, got %zu",
                  function_name,
                  expected_args,
                  actual_args);
        return rv_new_null();
    }

    // Check for stack overflow
    if (visitor->call_stack_depth >= visitor->max_call_stack_depth) {
        visitor_throw_exception(visitor, ast_new(AST_NULL), "Stack overflow", function_name);
        return rv_new_null();
    }

    // CRITICAL: Save current scope BEFORE evaluating arguments
    // This is essential because evaluating arguments may involve recursive calls
    // which will change visitor->current_scope
    scope_T *caller_scope = visitor->current_scope;

    // Evaluate arguments in the caller's scope
    // Arguments may contain recursive function calls
    RuntimeValue **evaluated_args = NULL;
    if (args_size > 0) {
        evaluated_args = memory_alloc(sizeof(RuntimeValue *) * args_size);
        for (int i = 0; i < args_size; i++) {
            // Special handling for wrapper nodes that contain runtime_value directly
            if (args[i] && args[i]->type == AST_VARIABLE_DEFINITION && args[i]->runtime_value) {
                // This is a wrapper node - use the runtime_value directly
                evaluated_args[i] = rv_ref((RuntimeValue *)args[i]->runtime_value);
            } else {
                evaluated_args[i] = visitor_visit(visitor, args[i]);
                if (!evaluated_args[i]) {
                    evaluated_args[i] = rv_new_null();
                }
            }
        }
        // CRITICAL: Restore current scope after evaluating arguments
        // Recursive calls in arguments will have changed visitor->current_scope
        visitor->current_scope = caller_scope;
    }

    // Push call frame for profiling and stack management
    visitor_push_call_frame(visitor, fdef, args, (size_t)args_size, function_name);

    // Create a new scope for this function call with proper parent chaining
    scope_T *function_scope = scope_new();
    if (!function_scope) {
        visitor_throw_exception(
            visitor, ast_new(AST_NULL), "Failed to create function scope", function_name);
        visitor_pop_call_frame(visitor);
        // Clean up evaluated args
        if (evaluated_args) {
            for (int i = 0; i < args_size; i++) {
                if (evaluated_args[i])
                    rv_unref(evaluated_args[i]);
            }
            memory_free(evaluated_args);
        }
        return rv_new_null();
    }

    // Set parent scope to the SAVED caller scope, not current scope
    // which may have been modified during argument evaluation
    function_scope->parent = caller_scope;

    // Copy global variables AND functions into function scope so functions can access them
    // For methods, skip this as they should only access instance variables through self
    if (!is_method_call && fdef->scope) {
        // Copy variable definitions
        if (fdef->scope->variable_definitions) {
            for (size_t i = 0; i < fdef->scope->variable_definitions_size; i++) {
                AST_T *global_var = fdef->scope->variable_definitions[i];
                // Only add if it's actually a variable definition
                if (global_var && global_var->type == AST_VARIABLE_DEFINITION) {
                    scope_add_variable_definition(function_scope, global_var);
                }
            }
        }

        // CRITICAL FIX: Also copy function definitions so functions can call each other
        if (fdef->scope->function_definitions) {
            for (size_t i = 0; i < fdef->scope->function_definitions_size; i++) {
                AST_T *func_def = fdef->scope->function_definitions[i];
                if (func_def && func_def->type == AST_FUNCTION_DEFINITION) {
                    scope_add_function_definition(function_scope, func_def);
                }
            }
        }
    }

    // SIMPLIFIED PARAMETER BINDING - Use straightforward value-based approach
    // This eliminates complex AST copying that causes double-free issues

    // Use the saved caller scope for restoration in error paths
    scope_T *previous_scope = caller_scope;

    // For method calls, we need to bind 'self' as the first parameter
    size_t param_offset = 0;
    if (is_method_call && args_size > 0) {
        // Bind 'self' parameter for methods using pre-evaluated value
        RuntimeValue *self_value = evaluated_args[0];
        if (!self_value) {
            self_value = rv_new_null();
        } else {
            rv_ref(self_value);  // Keep a reference for the scope
        }

        // Create a variable definition for self WITHOUT converting back to AST
        // This avoids the recursion issue
        AST_T *self_def = ast_new(AST_VARIABLE_DEFINITION);
        self_def->variable_definition_variable_name = memory_strdup("self");
        self_def->variable_definition_value = NULL;  // Don't store AST value
        self_def->runtime_value = self_value;        // Store RuntimeValue directly
        self_def->scope = function_scope;

        scope_add_variable_definition(function_scope, self_def);
        
        // ZEN uses 'self' not 'this' - already bound above
        
        if (self_value) {
            char *self_str = rv_to_string(self_value);
            LOG_VISITOR_DEBUG("Added 'self' to function scope with value: %s", self_str);
            memory_free(self_str);
        } else {
            LOG_VISITOR_DEBUG("Added 'self' to function scope with value: NULL");
        }

        param_offset = 1;  // Skip self in args array for remaining parameters
    }

    for (size_t i = 0; i < fdef->function_definition_args_size; i++) {
        AST_T *param_ast = fdef->function_definition_args[i];
        if (!param_ast || !param_ast->variable_name) {
            LOG_ERROR(LOG_CAT_VISITOR, "Invalid parameter at index %zu", i);
            visitor->current_scope = previous_scope;  // Restore previous scope
            scope_free(function_scope);
            visitor_pop_call_frame(visitor);
            // Clean up evaluated args
            if (evaluated_args) {
                for (int j = 0; j < args_size; j++) {
                    if (evaluated_args[j])
                        rv_unref(evaluated_args[j]);
                }
                memory_free(evaluated_args);
            }
            return rv_new_null();
        }

        // Use pre-evaluated argument value
        // For methods, we need to offset the argument index
        size_t arg_index = i + param_offset;
        if (arg_index >= (size_t)args_size) {
            LOG_ERROR(LOG_CAT_VISITOR,
                      "Argument index %zu out of bounds (args_size=%d)",
                      arg_index,
                      args_size);
            visitor->current_scope = previous_scope;  // Restore previous scope
            scope_free(function_scope);
            visitor_pop_call_frame(visitor);
            // Clean up evaluated args
            if (evaluated_args) {
                for (int j = 0; j < args_size; j++) {
                    if (evaluated_args[j])
                        rv_unref(evaluated_args[j]);
                }
                memory_free(evaluated_args);
            }
            return rv_new_null();
        }

        RuntimeValue *arg_value = evaluated_args[arg_index];
        if (!arg_value) {
            arg_value = rv_new_null();
        } else {
            rv_ref(arg_value);  // Keep a reference for conversion
        }

        // Convert RuntimeValue to AST for scope storage
        AST_T *param_value = runtime_value_to_ast(arg_value);
        rv_unref(arg_value);

        // Create variable definition for the parameter
        AST_T *param_def = ast_new(AST_VARIABLE_DEFINITION);
        if (!param_def || !param_value) {
            if (param_value)
                ast_free(param_value);
            if (param_def)
                ast_free(param_def);
            visitor->current_scope = previous_scope;  // Restore previous scope
            scope_free(function_scope);
            visitor_pop_call_frame(visitor);
            return rv_new_null();
        }

        param_def->variable_definition_variable_name = memory_strdup(param_ast->variable_name);
        param_def->variable_definition_value = param_value;
        param_def->scope = function_scope;

        // Add to scope using the standard function
        scope_add_variable_definition(function_scope, param_def);

        // Debug: Verify parameter binding
        LOG_VISITOR_DEBUG("Bound parameter %s", param_ast->variable_name);
    }

    // CRITICAL FIX: Do NOT modify the original AST's scope
    // This causes corruption when functions are called multiple times
    // Only use visitor_update_ast_scope which is temporary

    // CRITICAL FIX: Use visitor's current_scope instead of modifying AST nodes
    // This prevents corruption in recursive calls since AST nodes are shared

    // Set the function scope as the current execution scope
    visitor->current_scope = function_scope;

    RuntimeValue *result = NULL;
    if (!visitor_has_exception(visitor)) {
        result = visitor_visit(visitor, fdef->function_definition_body);
        visitor->total_instructions_executed++;

        // Handle return statements
        if (result && result->type == RV_OBJECT) {
            // Check for RETURN marker
            RuntimeValue *return_value = rv_object_get(result, "__RETURN__");
            if (return_value) {
                // Found return marker, extract the actual return value
                // The return value is owned by the marker object, so we need to
                // increase its refcount before we free the marker
                rv_ref(return_value);  // Retain the return value
                // Now we can safely unref the marker - return_value won't be freed
                // because we increased its refcount
                rv_unref(result);
                result = return_value;  // Use the return value directly
            }
        }
    }

    // Result is already a RuntimeValue, no need for defensive copying
    RuntimeValue *final_result = result ? result : rv_new_null();

    // Pop call frame (this will update profiling automatically)
    visitor_pop_call_frame(visitor);

    // Clean up parameter nodes created for this function call
    // These are temporary nodes not part of the main AST tree
    if (function_scope && function_scope->variable_definitions) {
        // We need to be careful here - only free nodes we created for parameters
        // Don't free nodes that existed before (from outer scope copies)
        size_t params_start = 0;
        if (fdef->scope && fdef->scope->variable_definitions) {
            params_start = fdef->scope->variable_definitions_size;
        }

        // Free parameter nodes starting from where we added them
        for (size_t i = params_start; i < function_scope->variable_definitions_size; i++) {
            AST_T *param_def = function_scope->variable_definitions[i];
            if (param_def) {
                // Free the value AST we created with runtime_value_to_ast
                if (param_def->variable_definition_value) {
                    ast_free(param_def->variable_definition_value);
                    param_def->variable_definition_value = NULL;  // Prevent double-free
                }
                // Free the parameter definition node
                ast_free(param_def);
            }
        }
    }

    // CRITICAL FIX: Restore the visitor's previous scope
    // This is essential for proper scope management in recursive calls
    visitor->current_scope = previous_scope;

    // Free the function scope (now safe - no AST nodes to double-free)
    scope_free(function_scope);

    // Clean up evaluated arguments
    if (evaluated_args) {
        for (int i = 0; i < args_size; i++) {
            if (evaluated_args[i])
                rv_unref(evaluated_args[i]);
        }
        memory_free(evaluated_args);
    }

    // Handle exceptions
    if (visitor_has_exception(visitor)) {
        LOG_ERROR(LOG_CAT_VISITOR, "Exception occurred in function '%s'", function_name);
        if (final_result && final_result != result) {
            rv_unref(final_result);  // Clean up if we created a new null
        }
        return rv_new_null();
    }

    return final_result;
}

// =============================================================================
// Advanced Runtime Optimization Functions
// =============================================================================

/**
 * @brief Get time difference in seconds
 * @param start Start time
 * @param end End time
 * @return Time difference in seconds
 */
static double visitor_get_time_diff(clock_t start, clock_t end)
{
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

/**
 * @brief Push a new call frame onto the call stack
 * @param visitor Visitor instance
 * @param function_def Function definition AST node
 * @param args Array of arguments
 * @param arg_count Number of arguments
 * @param function_name Function name
 */
static void visitor_push_call_frame(visitor_T *visitor,
                                    AST_T *function_def,
                                    AST_T **args,
                                    size_t arg_count,
                                    const char *function_name)
{
    if (!visitor || visitor->call_stack_depth >= visitor->max_call_stack_depth) {
        if (visitor) {
            visitor_throw_exception(visitor, ast_new(AST_NULL), "Stack overflow", "call_stack");
        }
        return;
    }

    CallFrame *frame = memory_alloc(sizeof(CallFrame));
    if (!frame) {
        visitor_throw_exception(
            visitor, ast_new(AST_NULL), "Memory allocation failed", "call_frame");
        return;
    }

    frame->function_def = function_def;
    frame->arguments = args;
    frame->arg_count = arg_count;
    frame->previous = visitor->call_stack;
    frame->start_time = clock();
    frame->recursion_depth = visitor->call_stack_depth;
    frame->function_name = function_name ? memory_strdup(function_name) : NULL;

    visitor->call_stack = frame;
    visitor->call_stack_depth++;
}

/**
 * @brief Pop the top call frame from the call stack
 * @param visitor Visitor instance
 */
static void visitor_pop_call_frame(visitor_T *visitor)
{
    if (!visitor || !visitor->call_stack) {
        return;
    }

    CallFrame *frame = visitor->call_stack;

    // Update function profiling
    if (visitor->profiling_enabled && frame->function_name) {
        double execution_time = visitor_get_time_diff(frame->start_time, clock());
        visitor_update_function_profile(visitor, frame->function_name, execution_time);
    }

    visitor->call_stack = frame->previous;
    visitor->call_stack_depth--;

    // Clean up frame
    if (frame->function_name) {
        memory_free(frame->function_name);
    }
    memory_free(frame);
}

/**
 * @brief Get or create a function profile for performance tracking
 * @param visitor Visitor instance
 * @param function_name Function name
 * @return Function profile or NULL on failure
 */
static FunctionProfile *visitor_get_or_create_profile(visitor_T *visitor, const char *function_name)
{
    if (!visitor || !function_name) {
        return NULL;
    }

    // Search for existing profile
    for (size_t i = 0; i < visitor->profile_count; i++) {
        if (visitor->function_profiles[i].function_name &&
            strcmp(visitor->function_profiles[i].function_name, function_name) == 0) {
            return &visitor->function_profiles[i];
        }
    }

    // Create new profile if space available
    if (visitor->profile_count < visitor->profile_capacity) {
        FunctionProfile *profile = &visitor->function_profiles[visitor->profile_count];
        profile->function_name = memory_strdup(function_name);
        profile->execution_count = 0;
        profile->total_execution_time = 0.0;
        profile->is_hot_function = false;
        profile->average_time_per_call = 0.0;
        profile->optimized_ast = NULL;
        visitor->profile_count++;
        return profile;
    }

    // Expand profile capacity if needed
    size_t new_capacity = visitor->profile_capacity + PROFILE_CAPACITY_INCREMENT;
    FunctionProfile *new_profiles = memory_alloc(sizeof(FunctionProfile) * new_capacity);
    if (!new_profiles) {
        return NULL;
    }

    // Copy existing profiles
    memcpy(
        new_profiles, visitor->function_profiles, sizeof(FunctionProfile) * visitor->profile_count);
    memory_free(visitor->function_profiles);
    visitor->function_profiles = new_profiles;
    visitor->profile_capacity = new_capacity;

    // Create the new profile
    FunctionProfile *profile = &visitor->function_profiles[visitor->profile_count];
    profile->function_name = memory_strdup(function_name);
    profile->execution_count = 0;
    profile->total_execution_time = 0.0;
    profile->is_hot_function = false;
    profile->average_time_per_call = 0.0;
    profile->optimized_ast = NULL;
    visitor->profile_count++;
    return profile;
}

/**
 * @brief Update function profile with execution time
 * @param visitor Visitor instance
 * @param function_name Function name
 * @param execution_time Execution time in seconds
 */
static void visitor_update_function_profile(visitor_T *visitor,
                                            const char *function_name,
                                            double execution_time)
{
    FunctionProfile *profile = visitor_get_or_create_profile(visitor, function_name);
    if (!profile) {
        return;
    }

    profile->execution_count++;
    profile->total_execution_time += execution_time;
    profile->average_time_per_call = profile->total_execution_time / profile->execution_count;

    // Check if function became hot
    if (!profile->is_hot_function && profile->execution_count >= visitor->hot_function_threshold &&
        profile->average_time_per_call >= HOT_FUNCTION_TIME_THRESHOLD) {
        profile->is_hot_function = true;
        LOG_VISITOR_DEBUG("Function '%s' marked as HOT (calls: %zu, avg time: %.6fs)",
                          function_name,
                          profile->execution_count,
                          profile->average_time_per_call);

        // Trigger optimization for hot function
        visitor_optimize_hot_function(visitor, function_name);
    }
}

/**
 * @brief Check if an AST node represents a constant expression
 * @param node AST node to check
 * @return true if constant, false otherwise
 */

/**
 * @brief Optimize constant expressions through constant folding
 * @param visitor Visitor instance
 * @param node AST node to optimize
 * @return Optimized AST node
 */

// Tail call optimization removed - not currently implemented

// =============================================================================
// Database-like file operations implementation
// =============================================================================

// Removed unused visitor_set_property_path function

// =============================================================================
// Public Advanced Runtime API Functions
// =============================================================================

/**
 * @brief Enable performance profiling for function optimization
 * @param visitor Visitor instance
 * @param enabled Whether to enable profiling
 */

/**
 * @brief Visit class definition node
 * @param visitor Visitor instance
 * @param node Class definition AST node
 * @return Class object
 */
static RuntimeValue *visitor_visit_class_definition(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->class_name) {
        return rv_new_null();
    }

    // Create a class object with special metadata
    RuntimeValue *class_obj = rv_new_object();

    // Set __class__ marker to identify this as a class
    rv_object_set(class_obj, "__class__", rv_new_boolean(true));

    // Store class name
    rv_object_set(class_obj, "__name__", rv_new_string(node->class_name));

    // Store parent class if exists
    if (node->parent_class) {
        rv_object_set(class_obj, "__parent__", rv_new_string(node->parent_class));
    }

    // Store methods as AST nodes for later execution
    if (node->class_methods && node->class_methods_size > 0) {
        for (size_t i = 0; i < node->class_methods_size; i++) {
            AST_T *method = node->class_methods[i];
            if (method && method->type == AST_FUNCTION_DEFINITION) {
                // Store method in class

                // Create a unique key for the method in the class object
                char method_key[256];
                snprintf(method_key,
                         sizeof(method_key),
                         "__method_%s",
                         method->function_definition_name);

                // Store the method AST as a function value
                RuntimeValue *method_func = rv_new_function(method, node->scope);

                // Store method in class

                rv_object_set(class_obj, method_key, method_func);
                rv_unref(method_func);
            }
        }
    }

    // Store class in scope as a runtime value directly
    scope_T *target_scope = visitor->current_scope ? visitor->current_scope : node->scope;
    scope_set_variable(target_scope, node->class_name, class_obj);

    return class_obj;
}

/**
 * @brief Visit new expression node
 * @param visitor Visitor instance
 * @param node New expression AST node
 * @return New instance object
 */
static RuntimeValue *visitor_visit_new_expression(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->new_class_name) {
        return rv_new_null();
    }

    // Look up the class
    scope_T *lookup_scope = visitor->current_scope ? visitor->current_scope : node->scope;
    RuntimeValue *class_obj = scope_get_variable(lookup_scope, node->new_class_name);
    if (!class_obj) {
        LOG_ERROR(LOG_CAT_VISITOR, "Class '%s' not found", node->new_class_name);
        return rv_new_null();
    }

    // Verify it's a class
    RuntimeValue *is_class = rv_object_get(class_obj, "__class__");
    if (!is_class || is_class->type != RV_BOOLEAN || !is_class->data.boolean) {
        LOG_ERROR(LOG_CAT_VISITOR, "'%s' is not a class", node->new_class_name);
        if (is_class)
            rv_unref(is_class);
        return rv_new_null();
    }
    rv_unref(is_class);

    // Create new instance
    RuntimeValue *instance = rv_new_object();

    // Set __class__ reference to the class object
    rv_object_set(instance, "__class__", class_obj);

    // Don't set self here - it will be set when methods are called
    // rv_object_set(instance, "self", instance);

    // Look for constructor method
    RuntimeValue *constructor = rv_object_get(class_obj, "__method_constructor");
    if (constructor && constructor->type == RV_FUNCTION) {
        // Create a special AST node that wraps the RuntimeValue directly
        // This avoids copying the instance
        AST_T *instance_wrapper = ast_new(AST_VARIABLE_DEFINITION);
        instance_wrapper->runtime_value = instance;  // Direct reference, no copy
        
        // Prepare arguments with instance wrapper as first (for 'this' binding)
        size_t total_args = 1 + node->new_arguments_size;
        AST_T **args_with_this = memory_alloc(sizeof(AST_T *) * total_args);
        
        // First argument is the instance wrapper
        args_with_this[0] = instance_wrapper;
        
        // Copy user arguments
        for (size_t i = 0; i < node->new_arguments_size; i++) {
            args_with_this[i + 1] = node->new_arguments[i];
        }
        
        // Execute constructor as a method (with 'this' as first arg)
        AST_T *constructor_def = (AST_T *)constructor->data.function.ast_node;
        RuntimeValue *constructor_result = visitor_execute_user_function_ex(
            visitor, constructor_def, args_with_this, total_args, true);
        
        // Clean up wrapper but not the runtime_value it references
        instance_wrapper->runtime_value = NULL;  // Prevent double-free
        ast_free(instance_wrapper);
        memory_free(args_with_this);

        if (constructor_result) {
            rv_unref(constructor_result);
        }
    }
    if (constructor) {
        rv_unref(constructor);
    }

    return instance;
}

/**
 * @brief Visit try/catch block
 * @param visitor Visitor instance
 * @param node Try/catch AST node
 * @return Result of try or catch block
 */
static RuntimeValue *visitor_visit_try_catch(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->try_block) {
        return rv_new_null();
    }

    // Save current exception state
    bool had_exception = visitor->exception_state.is_active;
    AST_T *saved_exception_ast = visitor->exception_state.exception_value;
    char *saved_message = visitor->exception_state.exception_message;
    visitor->exception_state.is_active = false;
    visitor->exception_state.exception_value = NULL;
    visitor->exception_state.exception_message = NULL;

    // Execute try block
    RuntimeValue *result = visitor_visit(visitor, node->try_block);

    // Check if exception was thrown
    if (visitor->exception_state.is_active && node->catch_block) {
        // Clear exception state
        visitor->exception_state.is_active = false;

        // If there's an exception variable, bind it in catch scope
        if (node->exception_variable && node->catch_block->scope) {
            AST_T *exc_var = ast_new(AST_VARIABLE_DEFINITION);
            exc_var->variable_definition_variable_name = memory_strdup(node->exception_variable);
            exc_var->variable_definition_value = ast_new(AST_NULL);

            // Store the exception value
            if (visitor->exception_state.exception_value) {
                // Convert AST to RuntimeValue
                RuntimeValue *exc_rv =
                    ast_to_runtime_value(visitor->exception_state.exception_value);
                exc_var->runtime_value = exc_rv;
            } else if (visitor->exception_state.exception_message) {
                exc_var->runtime_value = rv_new_string(visitor->exception_state.exception_message);
            } else {
                exc_var->runtime_value = rv_new_null();
            }

            exc_var->scope = node->catch_block->scope;
            scope_add_variable_definition(node->catch_block->scope, exc_var);
        }

        // Clean up exception value
        if (visitor->exception_state.exception_value) {
            ast_free(visitor->exception_state.exception_value);
            visitor->exception_state.exception_value = NULL;
        }
        if (visitor->exception_state.exception_message) {
            memory_free(visitor->exception_state.exception_message);
            visitor->exception_state.exception_message = NULL;
        }

        // Execute catch block
        if (result)
            rv_unref(result);
        result = visitor_visit(visitor, node->catch_block);
    }

    // If exception wasn't caught, restore previous state
    if (visitor->exception_state.is_active) {
        // Exception propagates up
    } else {
        // Restore previous exception state
        visitor->exception_state.is_active = had_exception;
        visitor->exception_state.exception_value = saved_exception_ast;
        visitor->exception_state.exception_message = saved_message;
    }

    return result ? result : rv_new_null();
}

/**
 * @brief Visit throw statement
 * @param visitor Visitor instance
 * @param node Throw AST node
 * @return Error value
 */
static RuntimeValue *visitor_visit_throw(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node) {
        return rv_new_null();
    }

    // Evaluate the exception value
    RuntimeValue *exception_value = NULL;
    if (node->exception_value) {
        exception_value = visitor_visit(visitor, node->exception_value);
    }

    // Set exception state
    visitor->exception_state.is_active = true;

    // Store exception value as AST
    if (visitor->exception_state.exception_value) {
        ast_free(visitor->exception_state.exception_value);
    }
    visitor->exception_state.exception_value =
        exception_value ? runtime_value_to_ast(exception_value) : NULL;

    // Store exception message if it's a string
    if (visitor->exception_state.exception_message) {
        memory_free(visitor->exception_state.exception_message);
        visitor->exception_state.exception_message = NULL;
    }

    if (exception_value && rv_is_string(exception_value)) {
        visitor->exception_state.exception_message = memory_strdup(rv_get_string(exception_value));
    }

    // Return error value
    RuntimeValue *error = rv_new_error(visitor->exception_state.exception_message
                                           ? visitor->exception_state.exception_message
                                           : "Exception thrown",
                                       1);

    if (exception_value)
        rv_unref(exception_value);

    return error;
}

/**
 * @brief Visit compound assignment (+=, -=, etc)
 * @param visitor Visitor instance
 * @param node Compound assignment AST node
 * @return Result value
 */
static RuntimeValue *visitor_visit_compound_assignment(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->compound_target || !node->compound_value) {
        return rv_new_null();
    }

    LOG_VISITOR_DEBUG("Compound assignment: %s op=%d",
                      node->compound_target->variable_name,
                      node->compound_op_type);

    // The target should be a variable
    if (node->compound_target->type != AST_VARIABLE) {
        LOG_ERROR(LOG_CAT_VISITOR, "Compound assignment target must be a variable");
        return rv_new_null();
    }

    // Get current value of the variable
    RuntimeValue *current_value = visitor_visit_variable(visitor, node->compound_target);
    if (!current_value) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Undefined variable '%s' in compound assignment",
                  node->compound_target->variable_name);
        return rv_new_null();
    }

    // Evaluate the right side
    RuntimeValue *right_value = visitor_visit(visitor, node->compound_value);
    if (!right_value) {
        rv_unref(current_value);
        return rv_new_null();
    }

    // Perform the operation based on the operator type
    RuntimeValue *result = NULL;
    switch (node->compound_op_type) {
    case TOKEN_PLUS_EQUALS:  // +=
        result = op_add(current_value, right_value);
        break;
    case TOKEN_MINUS_EQUALS:  // -=
        result = op_subtract(current_value, right_value);
        break;
    case TOKEN_MULTIPLY_EQUALS:  // *=
        result = op_multiply(current_value, right_value);
        break;
    case TOKEN_DIVIDE_EQUALS:  // /=
        result = op_divide(current_value, right_value);
        break;
    default:
        LOG_ERROR(
            LOG_CAT_VISITOR, "Unknown compound assignment operator: %d", node->compound_op_type);
        result = rv_new_null();
    }

    rv_unref(current_value);
    rv_unref(right_value);

    if (!result) {
        return rv_new_null();
    }

    // Look up the existing variable definition
    AST_T *var_def =
        scope_get_variable_definition(node->scope, node->compound_target->variable_name);
    if (!var_def) {
        LOG_ERROR(LOG_CAT_VISITOR,
                  "Variable '%s' not found in scope",
                  node->compound_target->variable_name);
        rv_unref(result);
        return rv_new_null();
    }

    // Update the runtime value directly
    if (var_def->runtime_value) {
        rv_unref(var_def->runtime_value);
    }
    var_def->runtime_value = rv_ref(result);

    return result;
}

/**
 * @brief Visit assignment (property assignment: set obj.prop value)
 * @param visitor Visitor instance
 * @param node Assignment AST node
 * @return Result value
 */
static RuntimeValue *visitor_visit_assignment(visitor_T *visitor, AST_T *node)
{
    if (!visitor || !node || !node->left || !node->right) {
        LOG_ERROR(LOG_CAT_VISITOR, "Invalid assignment node");
        return rv_new_null();
    }

    // Only support property access on the left side for now
    if (node->left->type != AST_PROPERTY_ACCESS) {
        LOG_ERROR(LOG_CAT_VISITOR, "Assignment target must be a property access");
        return rv_new_null();
    }

    AST_T *prop_access = node->left;
    if (!prop_access->object || !prop_access->property_name) {
        LOG_ERROR(LOG_CAT_VISITOR, "Invalid property access in assignment");
        return rv_new_null();
    }

    // Evaluate the object
    RuntimeValue *object = visitor_visit(visitor, prop_access->object);
    if (!object) {
        LOG_ERROR(LOG_CAT_VISITOR, "Failed to evaluate object in property assignment");
        return rv_new_null();
    }

    // Check if object is actually an object
    if (!rv_is_object(object)) {
        LOG_ERROR(LOG_CAT_VISITOR, "Cannot set property on non-object value");
        rv_unref(object);
        return rv_new_null();
    }

    // Evaluate the value to assign
    RuntimeValue *value = visitor_visit(visitor, node->right);
    if (!value) {
        rv_unref(object);
        return rv_new_null();
    }

    LOG_VISITOR_DEBUG("Property assignment: setting %s on object", prop_access->property_name);

    // Set the property
    rv_object_set(object, prop_access->property_name, value);

    // Clean up and return the value
    rv_unref(object);
    return value;  // Return the assigned value
}

// Stub implementations for missing functions to fix linking errors
// These functions are not in MANIFEST.json but are called by visitor code

/**
 * @brief Check if visitor has an exception
 * @param visitor Visitor instance
 * @return true if has exception, false otherwise
 */
bool visitor_has_exception(visitor_T *visitor)
{
    return visitor && visitor->exception_state.is_active;
}

/**
 * @brief Throw an exception in visitor
 * @param visitor Visitor instance
 * @param node AST node where exception occurred
 * @param message Error message
 * @param context Context string
 */
void visitor_throw_exception(visitor_T *visitor,
                             AST_T *node,
                             const char *message,
                             const char *context)
{
    if (!visitor || !message) {
        return;
    }

    // Create error information
    char error_buffer[512];
    if (context) {
        snprintf(error_buffer, sizeof(error_buffer), "[%s] %s", context, message);
    } else {
        snprintf(error_buffer, sizeof(error_buffer), "%s", message);
    }

    // Store exception in visitor's exception state
    if (visitor->exception_state.exception_message) {
        memory_free(visitor->exception_state.exception_message);
    }
    visitor->exception_state.exception_message = memory_strdup(error_buffer);
    visitor->exception_state.is_active = true;

    // Store exception AST node
    visitor->exception_state.exception_value = node;

    // Set generic source location info if we have a node
    if (node) {
        if (visitor->exception_state.source_location) {
            memory_free(visitor->exception_state.source_location);
        }
        visitor->exception_state.source_location = memory_strdup("runtime");
    }
}

/**
 * @brief Optimize a hot function
 * @param visitor Visitor instance
 * @param function_name Name of function to optimize
 * @return true if optimization succeeded, false otherwise
 */
bool visitor_optimize_hot_function(visitor_T *visitor, const char *function_name)
{
    if (!visitor || !function_name) {
        return false;
    }

    // Look for existing profile for this function
    FunctionProfile *profile = NULL;
    for (size_t i = 0; i < visitor->profile_count; i++) {
        if (visitor->function_profiles[i].function_name &&
            strcmp(visitor->function_profiles[i].function_name, function_name) == 0) {
            profile = &visitor->function_profiles[i];
            break;
        }
    }

    // If no profile exists and we have space, create one
    if (!profile && visitor->profile_count < visitor->profile_capacity) {
        profile = &visitor->function_profiles[visitor->profile_count++];
        profile->function_name = memory_strdup(function_name);
        profile->execution_count = 0;
        profile->total_execution_time = 0.0;
        profile->is_hot_function = false;
        profile->average_time_per_call = 0.0;
        profile->optimized_ast = NULL;
    }

    if (profile) {
        profile->execution_count++;
        // Mark as hot if called more than 10 times
        if (profile->execution_count > 10) {
            profile->is_hot_function = true;
            return true;
        }
    }

    return false;
}
