#ifndef VISITOR_H
#define VISITOR_H
#include "zen/core/ast.h"
#include "zen/core/runtime_value.h"

#include <stdbool.h>
#include <time.h>

// Advanced runtime optimization features
typedef enum {
    EXECUTION_MODE_INTERPRETED,  // Standard interpretation
    EXECUTION_MODE_CACHED,       // Cached expression results
    EXECUTION_MODE_OPTIMIZED     // Hot function optimization
} ExecutionMode;

// Runtime profiling data for function optimization
typedef struct {
    char *function_name;
    size_t execution_count;
    double total_execution_time;
    bool is_hot_function;
    double average_time_per_call;
    AST_T *optimized_ast;  // Cached optimized version
} FunctionProfile;

// Call frame for advanced stack management
typedef struct CallFrame {
    AST_T *function_def;
    AST_T **arguments;
    size_t arg_count;
    struct CallFrame *previous;
    clock_t start_time;
    size_t recursion_depth;
    char *function_name;
} CallFrame;

// Exception handling structure
typedef struct {
    AST_T *exception_value;
    char *exception_message;
    char *source_location;
    bool is_active;
} ExceptionState;

// Advanced visitor structure with enterprise-grade features
typedef struct VISITOR_STRUCT {
    // Basic execution state
    ExecutionMode mode;

    // Advanced call stack management
    CallFrame *call_stack;
    size_t call_stack_depth;
    size_t max_call_stack_depth;

    // Performance profiling system
    FunctionProfile *function_profiles;
    size_t profile_count;
    size_t profile_capacity;
    bool profiling_enabled;

    // Runtime optimizations
    bool tail_call_optimization;
    bool constant_folding;
    bool dead_code_elimination;
    bool inline_small_functions;

    // Exception handling
    ExceptionState exception_state;

    // Execution analytics
    size_t total_instructions_executed;
    double total_execution_time;
    size_t memory_allocations;
    size_t cache_hits;
    size_t cache_misses;

    // Hot function detection
    size_t hot_function_threshold;
} visitor_T;

/**
 * @brief Create a new visitor instance
 * @return New visitor instance or NULL on failure
 */
visitor_T *visitor_new();

/**
 * @brief Free a visitor instance and its resources
 * @param visitor The visitor instance to free
 */
void visitor_free(visitor_T *visitor);

/**
 * @brief Visit and evaluate an AST node
 * @param visitor Visitor instance
 * @param node AST node to visit
 * @return Result of evaluation
 */
RuntimeValue *visitor_visit(visitor_T *visitor, AST_T *node);

/**
 * @brief Visit variable definition node
 * @param visitor Visitor instance
 * @param node Variable definition AST node
 * @return The defined variable value
 */
RuntimeValue *visitor_visit_variable_definition(visitor_T *visitor, AST_T *node);

/**
 * @brief Visit function definition node
 * @param visitor Visitor instance
 * @param node Function definition AST node
 * @return The function definition node
 */
RuntimeValue *visitor_visit_function_definition(visitor_T *visitor, AST_T *node);

/**
 * @brief Visit variable node
 * @param visitor Visitor instance
 * @param node Variable AST node
 * @return Variable value or NULL
 */
RuntimeValue *visitor_visit_variable(visitor_T *visitor, AST_T *node);

/**
 * @brief Visit function call node
 * @param visitor Visitor instance
 * @param node Function call AST node
 * @return Result of function call
 */
RuntimeValue *visitor_visit_function_call(visitor_T *visitor, AST_T *node);

/**
 * @brief Visit string node
 * @param visitor Visitor instance
 * @param node String AST node
 * @return The string node as-is
 */
RuntimeValue *visitor_visit_string(visitor_T *visitor, AST_T *node);

/**
 * @brief Visit compound node (list of statements)
 * @param visitor Visitor instance
 * @param node Compound AST node
 * @return Result of last statement or NOOP
 */
RuntimeValue *visitor_visit_compound(visitor_T *visitor, AST_T *node);

// Advanced runtime optimization functions

/**
 * @brief Enable performance profiling for function optimization
 * @param visitor Visitor instance
 * @param enabled Whether to enable profiling
 */
void visitor_enable_profiling(visitor_T *visitor, bool enabled);

/**
 * @brief Set hot function threshold for optimization
 * @param visitor Visitor instance
 * @param threshold Number of calls before a function is considered "hot"
 */
void visitor_set_hot_function_threshold(visitor_T *visitor, size_t threshold);

/**
 * @brief Get execution statistics
 * @param visitor Visitor instance
 * @param stats_out Output buffer for statistics
 */
void visitor_get_execution_stats(visitor_T *visitor, char *stats_out, size_t buffer_size);

/**
 * @brief Enable specific runtime optimizations
 * @param visitor Visitor instance
 * @param tail_call_opt Enable tail call optimization
 * @param constant_fold Enable constant folding
 * @param dead_code_elim Enable dead code elimination
 */
void visitor_enable_optimizations(visitor_T *visitor,
                                  bool tail_call_opt,
                                  bool constant_fold,
                                  bool dead_code_elim);

/**
 * @brief Throw an exception during execution
 * @param visitor Visitor instance
 * @param exception_value The exception value
 * @param message Exception message
 * @param source_location Source code location
 */
void visitor_throw_exception(visitor_T *visitor,
                             AST_T *exception_value,
                             const char *message,
                             const char *source_location);

/**
 * @brief Check if an exception is currently active
 * @param visitor Visitor instance
 * @return true if exception is active, false otherwise
 */
bool visitor_has_exception(visitor_T *visitor);

/**
 * @brief Clear the current exception state
 * @param visitor Visitor instance
 */
void visitor_clear_exception(visitor_T *visitor);

/**
 * @brief Get the current call stack depth
 * @param visitor Visitor instance
 * @return Current call stack depth
 */
size_t visitor_get_call_depth(visitor_T *visitor);

/**
 * @brief Optimize a hot function by creating cached optimized version
 * @param visitor Visitor instance
 * @param function_name Name of function to optimize
 * @return true if optimization successful, false otherwise
 */
bool visitor_optimize_hot_function(visitor_T *visitor, const char *function_name);

/**
 * @brief Execute import statement
 * @param visitor Visitor instance
 * @param node Import AST node
 * @return RuntimeValue* Result of import execution
 */
RuntimeValue *visitor_visit_import(visitor_T *visitor, AST_T *node);

/**
 * @brief Execute export statement
 * @param visitor Visitor instance
 * @param node Export AST node
 * @return RuntimeValue* Result of export execution
 */
RuntimeValue *visitor_visit_export(visitor_T *visitor, AST_T *node);

/**
 * @brief Convert Value to AST node (helper function)
 * @param value Value to convert
 * @return AST_T* Corresponding AST node
 */
AST_T *value_to_ast(RuntimeValue *value);

#endif
