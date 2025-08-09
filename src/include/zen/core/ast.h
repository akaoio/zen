#ifndef ZEN_CORE_AST_H
#define ZEN_CORE_AST_H
#include <stdbool.h>
#include <stdlib.h>

typedef struct AST_STRUCT {
    enum {
        // Basic constructs
        AST_VARIABLE_DEFINITION,
        AST_FUNCTION_DEFINITION,
        AST_VARIABLE,
        AST_FUNCTION_CALL,
        AST_COMPOUND,
        AST_NOOP,

        // Literals
        AST_STRING,
        AST_NUMBER,
        AST_BOOLEAN,
        AST_NULL,
        AST_UNDECIDABLE,
        AST_ARRAY,
        AST_OBJECT,

        // Operators
        AST_BINARY_OP,
        AST_UNARY_OP,
        AST_ASSIGNMENT,

        // Control flow
        AST_IF_STATEMENT,
        AST_WHILE_LOOP,
        AST_FOR_LOOP,
        AST_BREAK,
        AST_CONTINUE,
        AST_RETURN,

        // Object-oriented
        AST_CLASS_DEFINITION,
        AST_NEW_EXPRESSION,
        AST_METHOD_CALL,
        AST_PROPERTY_ACCESS,

        // Module system
        AST_IMPORT,
        AST_EXPORT,

        // Database-like operations
        AST_FILE_GET,
        AST_FILE_PUT,
        AST_FILE_REFERENCE,  // For @ prefix cross-file references

        // Error handling
        AST_TRY_CATCH,
        AST_THROW,

        // Advanced features
        AST_LAMBDA,
        AST_AWAIT,
        AST_ASYNC_FUNCTION,

        // Modern syntax features
        AST_TERNARY,
        AST_COMPOUND_ASSIGNMENT,
        AST_RANGE,
        AST_SPREAD,
        AST_DESTRUCTURING,
        AST_OPTIONAL_CHAINING,
        AST_NULL_COALESCING,

        // Formal Logic System - AST-based structured data (not strings!)
        AST_LOGICAL_QUANTIFIER,       // ∀x, ∃x (for all, there exists)
        AST_LOGICAL_PREDICATE,        // P(x), Q(x,y) (predicates with arguments)
        AST_LOGICAL_CONNECTIVE,       // ∧, ∨, →, ↔, ¬ (and, or, implies, iff, not)
        AST_LOGICAL_VARIABLE,         // Bound and free variables
        AST_LOGICAL_PROPOSITION,      // Atomic propositions
        AST_LOGICAL_THEOREM,          // Theorem statements
        AST_LOGICAL_AXIOM,            // Axiom statements
        AST_LOGICAL_PROOF_STEP,       // Individual proof steps
        AST_LOGICAL_PREMISE,          // Proof premises
        AST_LOGICAL_CONCLUSION,       // Proof conclusions
        AST_LOGICAL_INFERENCE,        // Inference rules (modus ponens, etc.)
        AST_LOGICAL_SUBSTITUTION,     // Variable substitutions
        AST_MATHEMATICAL_EQUATION,    // Mathematical equations a² + b² = c²
        AST_MATHEMATICAL_INEQUALITY,  // Mathematical inequalities x > 0
        AST_MATHEMATICAL_FUNCTION     // Mathematical functions f(x) = x²
    } type;

    struct SCOPE_STRUCT *scope;

    /* AST_VARIABLE_DEFINITION */
    char *variable_definition_variable_name;
    struct AST_STRUCT *variable_definition_value;

    /* AST_FUNCTION_DEFINITION */
    struct AST_STRUCT *function_definition_body;
    char *function_definition_name;
    struct AST_STRUCT **function_definition_args;
    size_t function_definition_args_size;

    /* AST_VARIABLE */
    char *variable_name;

    /* AST_FUNCTION_CALL */
    char *function_call_name;
    struct AST_STRUCT **function_call_arguments;
    size_t function_call_arguments_size;

    /* AST_STRING */
    char *string_value;

    /* AST_NUMBER */
    double number_value;

    /* AST_BOOLEAN */
    int boolean_value;

    /* AST_ARRAY */
    struct AST_STRUCT **array_elements;
    size_t array_size;

    /* AST_OBJECT */
    char **object_keys;
    struct AST_STRUCT **object_values;
    size_t object_size;

    /* AST_BINARY_OP */
    int operator_type;
    struct AST_STRUCT *left;
    struct AST_STRUCT *right;

    /* AST_UNARY_OP */
    struct AST_STRUCT *operand;

    /* AST_IF_STATEMENT */
    struct AST_STRUCT *condition;
    struct AST_STRUCT *then_branch;
    struct AST_STRUCT *else_branch;

    /* AST_WHILE_LOOP */
    struct AST_STRUCT *loop_condition;
    struct AST_STRUCT *loop_body;

    /* AST_FOR_LOOP */
    char *iterator_variable;
    struct AST_STRUCT *iterable;
    struct AST_STRUCT *for_body;

    /* AST_RETURN */
    struct AST_STRUCT *return_value;

    /* AST_CLASS_DEFINITION */
    char *class_name;
    char *parent_class;
    struct AST_STRUCT **class_methods;
    size_t class_methods_size;

    /* AST_NEW_EXPRESSION */
    char *new_class_name;
    struct AST_STRUCT **new_arguments;
    size_t new_arguments_size;

    /* AST_PROPERTY_ACCESS */
    struct AST_STRUCT *object;
    char *property_name;

    /* AST_IMPORT */
    char *import_path;
    char **import_names;
    size_t import_names_size;

    /* AST_EXPORT */
    char *export_name;
    struct AST_STRUCT *export_value;

    /* AST_TRY_CATCH */
    struct AST_STRUCT *try_block;
    struct AST_STRUCT *catch_block;
    char *exception_variable;

    /* AST_THROW */
    struct AST_STRUCT *exception_value;

    /* AST_LAMBDA */
    struct AST_STRUCT **lambda_args;
    size_t lambda_args_size;
    struct AST_STRUCT *lambda_body;

    /* AST_TERNARY */
    struct AST_STRUCT *ternary_condition;
    struct AST_STRUCT *ternary_true_expr;
    struct AST_STRUCT *ternary_false_expr;

    /* AST_COMPOUND_ASSIGNMENT */
    int compound_op_type;
    struct AST_STRUCT *compound_target;
    struct AST_STRUCT *compound_value;

    /* AST_RANGE */
    struct AST_STRUCT *range_start;
    struct AST_STRUCT *range_end;
    bool range_inclusive;

    /* AST_SPREAD */
    struct AST_STRUCT *spread_expression;

    /* AST_DESTRUCTURING */
    char **destructure_names;
    size_t destructure_count;
    struct AST_STRUCT *destructure_source;

    /* AST_OPTIONAL_CHAINING */
    struct AST_STRUCT *chain_object;
    char *chain_property;

    /* AST_NULL_COALESCING */
    struct AST_STRUCT *coalesce_left;
    struct AST_STRUCT *coalesce_right;

    /* AST_COMPOUND */
    struct AST_STRUCT **compound_statements;
    size_t compound_size;

    /* FORMAL LOGIC - AST-based structured data (Gödel approved!) */

    /* AST_LOGICAL_QUANTIFIER */
    enum {
        QUANTIFIER_UNIVERSAL,   // ∀ (for all)
        QUANTIFIER_EXISTENTIAL  // ∃ (there exists)
    } quantifier_type;
    char *quantified_variable;             // Variable being quantified
    struct AST_STRUCT *quantified_domain;  // Domain of quantification
    struct AST_STRUCT *quantified_body;    // Body of quantified expression

    /* AST_LOGICAL_PREDICATE */
    char *predicate_name;                // Name of predicate (P, Q, R, etc.)
    struct AST_STRUCT **predicate_args;  // Arguments to predicate
    size_t predicate_args_size;          // Number of arguments

    /* AST_LOGICAL_CONNECTIVE */
    enum {
        CONNECTIVE_AND,      // ∧ (and)
        CONNECTIVE_OR,       // ∨ (or)
        CONNECTIVE_IMPLIES,  // → (implies)
        CONNECTIVE_IFF,      // ↔ (if and only if)
        CONNECTIVE_NOT       // ¬ (not)
    } connective_type;
    struct AST_STRUCT *connective_left;   // Left operand (null for NOT)
    struct AST_STRUCT *connective_right;  // Right operand

    /* AST_LOGICAL_VARIABLE */
    char *logical_var_name;                 // Variable name
    bool is_bound;                          // Whether variable is bound by quantifier
    struct AST_STRUCT *binding_quantifier;  // Quantifier that binds this var

    /* AST_LOGICAL_PROPOSITION */
    char *proposition_name;  // Atomic proposition name
    bool proposition_value;  // Truth value (if known)

    /* AST_LOGICAL_THEOREM */
    char *theorem_name;                      // Name of theorem
    struct AST_STRUCT **theorem_hypotheses;  // Array of hypothesis ASTs
    size_t theorem_hypotheses_size;          // Number of hypotheses
    struct AST_STRUCT *theorem_conclusion;   // Conclusion AST
    struct AST_STRUCT *theorem_proof;        // Proof AST (if proven)

    /* AST_LOGICAL_AXIOM */
    char *axiom_name;                    // Name of axiom
    struct AST_STRUCT *axiom_statement;  // Axiom statement AST
    bool axiom_is_consistent;            // Consistency flag

    /* AST_LOGICAL_PROOF_STEP */
    struct AST_STRUCT *step_statement;      // Statement being asserted
    struct AST_STRUCT *step_justification;  // Justification (inference rule, etc.)
    struct AST_STRUCT **step_premises;      // Premises used in this step
    size_t step_premises_size;              // Number of premises

    /* AST_LOGICAL_INFERENCE */
    enum {
        INFERENCE_MODUS_PONENS,     // P→Q, P ⊢ Q
        INFERENCE_MODUS_TOLLENS,    // P→Q, ¬Q ⊢ ¬P
        INFERENCE_UNIVERSAL_INST,   // ∀x P(x) ⊢ P(a)
        INFERENCE_EXISTENTIAL_GEN,  // P(a) ⊢ ∃x P(x)
        INFERENCE_ASSUMPTION,       // Direct assumption
        INFERENCE_AXIOM             // Axiom application
    } inference_type;
    struct AST_STRUCT **inference_premises;   // Input premises
    size_t inference_premises_size;           // Number of premises
    struct AST_STRUCT *inference_conclusion;  // Derived conclusion

    /* AST_MATHEMATICAL_EQUATION */
    struct AST_STRUCT *equation_left;   // Left side of equation
    struct AST_STRUCT *equation_right;  // Right side of equation

    /* AST_MATHEMATICAL_INEQUALITY */
    enum {
        INEQUALITY_LT,  // <
        INEQUALITY_LE,  // ≤
        INEQUALITY_GT,  // >
        INEQUALITY_GE   // ≥
    } inequality_type;
    struct AST_STRUCT *inequality_left;   // Left side
    struct AST_STRUCT *inequality_right;  // Right side

    /* AST_MATHEMATICAL_FUNCTION */
    char *math_function_name;                // Function name (f, g, sin, cos, etc.)
    struct AST_STRUCT **math_function_args;  // Function arguments
    size_t math_function_args_size;          // Number of arguments

    /* AST_FILE_GET */
    struct AST_STRUCT *file_get_path;  // File path expression (can be string literal or variable)
    struct AST_STRUCT *file_get_property;  // Property path expression (dot-separated path)

    /* AST_FILE_PUT */
    struct AST_STRUCT *file_put_path;      // File path expression
    struct AST_STRUCT *file_put_property;  // Property path expression
    struct AST_STRUCT *file_put_value;     // Value to put

    /* AST_FILE_REFERENCE */
    char *file_ref_target_file;    // Target file path (e.g., "../addresses.json")
    char *file_ref_property_path;  // Property path in target file (e.g., "office.alice")

    /* Memory pool management */
    bool pooled;              ///< true if allocated from memory pool
    struct AST_STRUCT *next;  ///< Next node in free list (when pooled)
} AST_T;

/**
 * @brief Create new AST node
 * @param type The AST node type
 * @return Pointer to newly allocated AST node
 */
AST_T *ast_new(int type);

/**
 * @brief Create a new number AST node
 * @param value The numeric value
 * @return Pointer to newly allocated AST node of type AST_NUMBER
 */
AST_T *ast_new_number(double value);

/**
 * @brief Create a new boolean AST node
 * @param value The boolean value (0 for false, non-zero for true)
 * @return Pointer to newly allocated AST node of type AST_BOOLEAN
 */
AST_T *ast_new_boolean(int value);

/**
 * @brief Create a new null AST node
 * @return Pointer to newly allocated AST node of type AST_NULL
 */
AST_T *ast_new_null(void);

/**
 * @brief Create a new undecidable AST node
 * @return Pointer to newly allocated AST node of type AST_UNDECIDABLE
 */
AST_T *ast_new_undecidable(void);

/**
 * @brief Create a new string AST node
 * @param value The string value (will be copied)
 * @return Pointer to newly allocated AST node of type AST_STRING
 */
AST_T *ast_new_string(const char *value);

/**
 * @brief Create a new binary operation AST node
 * @param operator_type The operator type (from token types)
 * @param left Left operand AST node
 * @param right Right operand AST node
 * @return Pointer to newly allocated AST node of type AST_BINARY_OP
 */
AST_T *ast_new_binary_op(int operator_type, AST_T *left, AST_T *right);

/**
 * @brief Create a new unary operation AST node
 * @param operator_type The operator type (from token types)
 * @param operand The operand AST node
 * @return Pointer to newly allocated AST node of type AST_UNARY_OP
 */
AST_T *ast_new_unary_op(int operator_type, AST_T *operand);

/**
 * @brief Create a new array AST node
 * @param elements Array of AST nodes representing array elements
 * @param size Number of elements in the array
 * @return Pointer to newly allocated AST node of type AST_ARRAY
 */
AST_T *ast_new_array(AST_T **elements, size_t size);

/**
 * @brief Create a new object AST node
 * @param keys Array of string keys
 * @param values Array of AST nodes representing values
 * @param size Number of key-value pairs
 * @return Pointer to newly allocated AST node of type AST_OBJECT
 */
AST_T *ast_new_object(char **keys, AST_T **values, size_t size);

/**
 * @brief Free an AST node and all its children recursively
 * @param ast The AST node to free
 */
void ast_free(AST_T *ast);

/* ============================================================================
 * CONVENIENCE FUNCTIONS FOR COMMON AST PATTERNS
 * ============================================================================ */

/**
 * @brief Create a new compound AST node with multiple statements
 * @param statements Array of AST statement nodes
 * @param count Number of statements
 * @return Pointer to newly allocated compound AST node
 */
AST_T *ast_new_compound(AST_T **statements, size_t count);

/**
 * @brief Create a new spread operator AST node for rest parameters
 * @param expression The expression to spread (typically a variable name)
 * @return Pointer to newly allocated AST node of type AST_SPREAD
 */
AST_T *ast_new_spread(AST_T *expression);

/* ============================================================================
 * FORMAL LOGIC AST CONSTRUCTOR FUNCTIONS - GÖDEL APPROVED!
 * ============================================================================ */

/**
 * @brief Create a new logical quantifier AST node
 * @param type QUANTIFIER_UNIVERSAL or QUANTIFIER_EXISTENTIAL
 * @param variable Name of quantified variable
 * @param domain Domain of quantification (can be NULL)
 * @param body Body of quantified expression
 * @return Pointer to AST_LOGICAL_QUANTIFIER node
 */
AST_T *ast_new_logical_quantifier(int type, const char *variable, AST_T *domain, AST_T *body);

/**
 * @brief Create a new logical predicate AST node
 * @param name Name of predicate (P, Q, etc.)
 * @param args Array of argument ASTs
 * @param args_size Number of arguments
 * @return Pointer to AST_LOGICAL_PREDICATE node
 */
AST_T *ast_new_logical_predicate(const char *name, AST_T **args, size_t args_size);

/**
 * @brief Create a new logical connective AST node
 * @param type CONNECTIVE_AND, CONNECTIVE_OR, CONNECTIVE_IMPLIES, CONNECTIVE_IFF, CONNECTIVE_NOT
 * @param left Left operand (NULL for NOT)
 * @param right Right operand
 * @return Pointer to AST_LOGICAL_CONNECTIVE node
 */
AST_T *ast_new_logical_connective(int type, AST_T *left, AST_T *right);

/**
 * @brief Create a new logical variable AST node
 * @param name Variable name
 * @param is_bound Whether variable is bound by quantifier
 * @return Pointer to AST_LOGICAL_VARIABLE node
 */
AST_T *ast_new_logical_variable(const char *name, bool is_bound);

/**
 * @brief Create a new logical proposition AST node
 * @param name Proposition name
 * @return Pointer to AST_LOGICAL_PROPOSITION node
 */
AST_T *ast_new_logical_proposition(const char *name);

/**
 * @brief Create a new logical theorem AST node
 * @param name Theorem name
 * @param hypotheses Array of hypothesis ASTs
 * @param hyp_size Number of hypotheses
 * @param conclusion Conclusion AST
 * @return Pointer to AST_LOGICAL_THEOREM node
 */
AST_T *
ast_new_logical_theorem(const char *name, AST_T **hypotheses, size_t hyp_size, AST_T *conclusion);

/**
 * @brief Create a new logical axiom AST node
 * @param name Axiom name
 * @param statement Axiom statement AST
 * @return Pointer to AST_LOGICAL_AXIOM node
 */
AST_T *ast_new_logical_axiom(const char *name, AST_T *statement);

/**
 * @brief Create a new logical proof step AST node
 * @param statement Statement being asserted
 * @param justification Justification AST
 * @param premises Array of premise ASTs
 * @param premises_size Number of premises
 * @return Pointer to AST_LOGICAL_PROOF_STEP node
 */
AST_T *ast_new_logical_proof_step(AST_T *statement,
                                  AST_T *justification,
                                  AST_T **premises,
                                  size_t premises_size);

/**
 * @brief Create a new mathematical equation AST node
 * @param left Left side expression
 * @param right Right side expression
 * @return Pointer to AST_MATHEMATICAL_EQUATION node
 */
AST_T *ast_new_mathematical_equation(AST_T *left, AST_T *right);

/**
 * @brief Create a new mathematical inequality AST node
 * @param type INEQUALITY_LT, INEQUALITY_LE, INEQUALITY_GT, INEQUALITY_GE
 * @param left Left side expression
 * @param right Right side expression
 * @return Pointer to AST_MATHEMATICAL_INEQUALITY node
 */
AST_T *ast_new_mathematical_inequality(int type, AST_T *left, AST_T *right);

/**
 * @brief Create a new mathematical function AST node
 * @param name Function name
 * @param args Array of argument ASTs
 * @param args_size Number of arguments
 * @return Pointer to AST_MATHEMATICAL_FUNCTION node
 */
AST_T *ast_new_mathematical_function(const char *name, AST_T **args, size_t args_size);

/**
 * @brief Create a new file get AST node
 * @param file_path File path expression (string or variable)
 * @param property_path Property path expression for accessing nested data
 * @return Pointer to AST_FILE_GET node
 */
AST_T *ast_new_file_get(AST_T *file_path, AST_T *property_path);

/**
 * @brief Create a new file put AST node
 * @param file_path File path expression (string or variable)
 * @param property_path Property path expression for nested data
 * @param value Value to store
 * @return Pointer to AST_FILE_PUT node
 */
AST_T *ast_new_file_put(AST_T *file_path, AST_T *property_path, AST_T *value);

/**
 * @brief Create a new file reference AST node for @ prefix cross-file references
 * @param target_file Target file path (e.g., "../addresses.json")
 * @param property_path Property path in target file (e.g., "office.alice")
 * @return Pointer to AST_FILE_REFERENCE node
 */
AST_T *ast_new_file_reference(const char *target_file, const char *property_path);

/**
 * @brief Create a new variable definition AST node
 * @param name Variable name
 * @param value Initial value AST node
 * @return Pointer to newly allocated variable definition AST node
 */
AST_T *ast_new_variable_definition(const char *name, AST_T *value);

/**
 * @brief Create a new variable reference AST node
 * @param name Variable name
 * @return Pointer to newly allocated variable AST node
 */
AST_T *ast_new_variable(const char *name);

/**
 * @brief Create a new function call AST node
 * @param name Function name
 * @param args Array of argument AST nodes
 * @param arg_count Number of arguments
 * @return Pointer to newly allocated function call AST node
 */
AST_T *ast_new_function_call(const char *name, AST_T **args, size_t arg_count);

/**
 * @brief Create a new function definition AST node
 * @param name Function name
 * @param args Array of parameter AST nodes
 * @param arg_count Number of parameters
 * @param body Function body AST node
 * @return Pointer to newly allocated function definition AST node
 */
AST_T *ast_new_function_definition(const char *name, AST_T **args, size_t arg_count, AST_T *body);

/**
 * @brief Create a new if statement AST node
 * @param condition Condition AST node
 * @param then_branch Then branch AST node
 * @param else_branch Else branch AST node (can be NULL)
 * @return Pointer to newly allocated if statement AST node
 */
AST_T *ast_new_if_statement(AST_T *condition, AST_T *then_branch, AST_T *else_branch);

/**
 * @brief Create a new while loop AST node
 * @param condition Loop condition AST node
 * @param body Loop body AST node
 * @return Pointer to newly allocated while loop AST node
 */
AST_T *ast_new_while_loop(AST_T *condition, AST_T *body);

/**
 * @brief Create a new for loop AST node
 * @param iterator Iterator variable name
 * @param iterable Iterable expression AST node
 * @param body Loop body AST node
 * @return Pointer to newly allocated for loop AST node
 */
AST_T *ast_new_for_loop(const char *iterator, AST_T *iterable, AST_T *body);

/**
 * @brief Create a new return statement AST node
 * @param value Return value AST node (can be NULL for empty return)
 * @return Pointer to newly allocated return statement AST node
 */
AST_T *ast_new_return_statement(AST_T *value);

/**
 * @brief Create a new property access AST node
 * @param object Object AST node
 * @param property Property name
 * @return Pointer to newly allocated property access AST node
 */
AST_T *ast_new_property_access(AST_T *object, const char *property);

/**
 * @brief Create a new assignment AST node
 * @param left Left-hand side AST node (variable or property access)
 * @param right Right-hand side value AST node
 * @return Pointer to newly allocated assignment AST node
 */
AST_T *ast_new_assignment(AST_T *left, AST_T *right);

/**
 * @brief Create a new NOOP AST node (no operation)
 * @return Pointer to newly allocated NOOP AST node
 */
AST_T *ast_new_noop(void);

/* Advanced AST node constructors */

/**
 * @brief Create a new lambda AST node
 * @param args Array of parameter AST nodes
 * @param arg_count Number of parameters
 * @param body Lambda body AST node
 * @return Pointer to newly allocated lambda AST node
 */
AST_T *ast_new_lambda(AST_T **args, size_t arg_count, AST_T *body);

/**
 * @brief Create a new ternary expression AST node
 * @param condition Condition AST node
 * @param true_expr Expression for true case
 * @param false_expr Expression for false case
 * @return Pointer to newly allocated ternary AST node
 */
AST_T *ast_new_ternary(AST_T *condition, AST_T *true_expr, AST_T *false_expr);

/**
 * @brief Create a new compound assignment AST node
 * @param op_type Compound operator type (+=, -=, etc.)
 * @param target Target variable/property AST node
 * @param value Value AST node
 * @return Pointer to newly allocated compound assignment AST node
 */
AST_T *ast_new_compound_assignment(int op_type, AST_T *target, AST_T *value);

/**
 * @brief Create a new range AST node
 * @param start Start value AST node
 * @param end End value AST node
 * @param inclusive Whether the range is inclusive
 * @return Pointer to newly allocated range AST node
 */
AST_T *ast_new_range(AST_T *start, AST_T *end, bool inclusive);

/**
 * @brief Create a new spread AST node
 * @param expression Expression to spread
 * @return Pointer to newly allocated spread AST node
 */
AST_T *ast_new_spread(AST_T *expression);

/**
 * @brief Create a new destructuring assignment AST node
 * @param names Array of variable names to destructure into
 * @param count Number of names
 * @param source Source AST node to destructure from
 * @return Pointer to newly allocated destructuring AST node
 */
AST_T *ast_new_destructuring(char **names, size_t count, AST_T *source);

/**
 * @brief Create a new optional chaining AST node
 * @param object Object AST node
 * @param property Property name
 * @return Pointer to newly allocated optional chaining AST node
 */
AST_T *ast_new_optional_chaining(AST_T *object, const char *property);

/**
 * @brief Create a new null coalescing AST node
 * @param left Left operand AST node
 * @param right Right operand AST node (default value)
 * @return Pointer to newly allocated null coalescing AST node
 */
AST_T *ast_new_null_coalescing(AST_T *left, AST_T *right);

/**
 * @brief Create a new class definition AST node
 * @param class_name Name of the class
 * @param parent_class Name of parent class (can be NULL)
 * @param methods Array of method definition AST nodes
 * @param methods_count Number of methods
 * @return Pointer to newly allocated class definition AST node
 */
AST_T *ast_new_class_definition(const char *class_name,
                                const char *parent_class,
                                AST_T **methods,
                                size_t methods_count);

/**
 * @brief Create a deep copy of an AST node with cycle detection
 * @param original Original AST node to copy
 * @return Deep copy of the AST node, or NULL on failure/infinite recursion
 *
 * This function performs a deep copy of an AST tree while detecting and handling
 * circular references that could cause infinite recursion. It uses a visited
 * nodes tracker to identify cycles and returns shared references for already
 * visited nodes, effectively converting circular structures into DAGs.
 *
 * Key features:
 * - Detects infinite recursion and circular references
 * - Limits recursion depth to prevent stack overflow
 * - Memory-safe with proper cleanup on failure
 * - Handles all AST node types comprehensively
 */
AST_T *ast_copy(AST_T *original);

#endif  // ZEN_CORE_AST_H
