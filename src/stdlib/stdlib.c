/*
 * stdlib.c
 * ZEN Standard Library - Main registry and initialization
 *
 * This module provides the main interface for registering all stdlib functions
 * with the ZEN runtime system.
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/stdlib.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/convert.h"
#include "zen/stdlib/http.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/json.h"
#include "zen/stdlib/logging.h"
#include "zen/stdlib/logic.h"
#include "zen/stdlib/math.h"
#include "zen/stdlib/regex.h"
#include "zen/stdlib/string.h"
#include "zen/stdlib/yaml.h"

#include <stdlib.h>
#include <string.h>

// Forward declarations for functions not in headers yet
extern RuntimeValue *array_push_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *array_pop_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *array_length_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *array_get_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *array_set_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *array_slice_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *system_exec(RuntimeValue **args, size_t argc);
extern RuntimeValue *datetime_now(RuntimeValue **args, size_t argc);
extern RuntimeValue *datetime_add(RuntimeValue **args, size_t argc);
extern RuntimeValue *datetime_subtract(RuntimeValue **args, size_t argc);
extern RuntimeValue *datetime_diff(RuntimeValue **args, size_t argc);
extern RuntimeValue *datetime_timezone(RuntimeValue **args, size_t argc);
extern RuntimeValue *datetime_utc(RuntimeValue **args, size_t argc);

// Datastructures functions
extern RuntimeValue *datastructures_set_new(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_set_add(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_set_has(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_set_remove(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_set_size(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_pqueue_new(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_pqueue_push(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_pqueue_pop(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_pqueue_peek(RuntimeValue **args, size_t argc);
extern RuntimeValue *datastructures_pqueue_size(RuntimeValue **args, size_t argc);

// Wrapper functions for file I/O that need stdlib interface
extern RuntimeValue *json_load_file(RuntimeValue **args, size_t argc);
extern RuntimeValue *yaml_load_file_wrapper(RuntimeValue **args, size_t argc);

// Forward declarations for module system functions
extern RuntimeValue *module_import_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *module_require_stdlib(RuntimeValue **args, size_t argc);

// Forward declarations for logging functions
extern RuntimeValue *logging_debug(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_info(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_warn(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_error(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_debugf(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_infof(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_warnf(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_errorf(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_debug_if(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_set_level(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_with_context(RuntimeValue **args, size_t argc);
extern RuntimeValue *logging_get_level(RuntimeValue **args, size_t argc);

// Forward declarations for HTTP wrapper functions
extern RuntimeValue *http_get_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *http_post_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *http_put_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *http_delete_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *http_timeout_stdlib(RuntimeValue **args, size_t argc);

// Forward declarations for regex wrapper functions
extern RuntimeValue *regex_match_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *regex_replace_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *regex_split_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *regex_compile_stdlib(RuntimeValue **args, size_t argc);

// Forward declarations for stdlib functions
extern RuntimeValue *json_stringify_pretty_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *json_parse_file_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *json_parse_stdlib(RuntimeValue **args, size_t argc);
extern RuntimeValue *json_stringify_stdlib(RuntimeValue **args, size_t argc);

// Native function typedef for runtime integration
typedef RuntimeValue *(*ZenNativeFunc)(RuntimeValue **args, size_t argc);

// Global registry of stdlib functions
static ZenStdlibFunction stdlib_functions[] = {
    // I/O Functions
    {"print", io_print, "Print value to stdout with newline"},
    {"input", io_input, "Read line from stdin"},
    {"readFile", io_read_file, "Read entire file contents"},
    {"writeFile", io_write_file, "Write string to file"},
    {"appendFile", io_append_file, "Append string to file"},
    {"fileExists", io_file_exists, "Check if file exists"},

    // String Functions
    {"len", string_length, "Get length of string or array"},
    {"upper", string_upper, "Convert string to uppercase"},
    {"lower", string_lower, "Convert string to lowercase"},
    {"trim", string_trim, "Trim whitespace from string"},
    {"split", string_split, "Split string by delimiter"},
    {"contains", string_contains, "Check if string contains substring"},
    {"replace", string_replace, "Replace substring in string"},
    {"substring", string_substring, "Extract substring from string"},
    {"indexOf", string_index_of, "Find index of substring in string"},
    {"concat", string_concat, "Concatenate multiple strings"},
    {"startsWith", string_starts_with, "Check if string starts with prefix"},
    {"endsWith", string_ends_with, "Check if string ends with suffix"},
    {"toUpper", string_to_upper, "Convert string to uppercase (alias)"},
    {"toLower", string_to_lower, "Convert string to lowercase (alias)"},

    // Math Functions
    {"abs", math_abs, "Absolute value"},
    {"floor", math_floor, "Floor function"},
    {"ceil", math_ceil, "Ceiling function"},
    {"round", math_round, "Round to nearest integer"},
    {"sqrt", math_sqrt, "Square root"},
    {"pow", math_pow, "Power function"},
    {"sin", math_sin, "Sine function"},
    {"cos", math_cos, "Cosine function"},
    {"tan", math_tan, "Tangent function"},
    {"log", math_log, "Natural logarithm"},
    {"random", math_random, "Random number 0-1"},
    {"randomInt", math_random_int, "Random integer in range"},
    {"min", math_min, "Minimum of two numbers"},
    {"max", math_max, "Maximum of two numbers"},
    {"isNaN", math_is_nan, "Check if value is NaN"},
    {"isInfinite", math_is_infinite, "Check if value is infinite"},

    // Type Conversion Functions
    {"toString", convert_to_string, "Convert value to string"},
    {"toNumber", convert_to_number, "Convert value to number"},
    {"toBoolean", convert_to_boolean, "Convert value to boolean"},
    {"typeOf", convert_type_of, "Get type name of value"},
    {"isType", convert_is_type, "Check if value is of specific type"},
    {"parseInt", convert_parse_int, "Parse integer from string"},
    {"parseFloat", convert_parse_float, "Parse float from string"},

    // JSON Functions
    {"jsonParse", json_parse_stdlib, "Parse JSON string"},
    {"jsonStringify", json_stringify_stdlib, "Convert value to JSON string"},
    {"jsonPretty",
     json_stringify_pretty_stdlib,
     "Convert value to formatted JSON with indentation"},
    {"jsonLoadFile", json_load_file, "Load and parse JSON from file"},

    // Array Functions
    {"push", array_push_stdlib, "Add element to end of array"},
    {"pop", array_pop_stdlib, "Remove and return last element"},
    {"arrayLength", array_length_stdlib, "Get length of array"},
    {"arrayGet", array_get_stdlib, "Get element at index"},
    {"arraySet", array_set_stdlib, "Set element at index"},
    {"slice", array_slice_stdlib, "Get slice of array"},

    // Set Functions
    {"setNew", datastructures_set_new, "Create new set"},
    {"setAdd", datastructures_set_add, "Add element to set"},
    {"setHas", datastructures_set_has, "Check if set contains element"},
    {"setRemove", datastructures_set_remove, "Remove element from set"},
    {"setSize", datastructures_set_size, "Get size of set"},

    // Priority Queue Functions
    {"pqueueNew", datastructures_pqueue_new, "Create new priority queue"},
    {"pqueuePush", datastructures_pqueue_push, "Add element with priority to queue"},
    {"pqueuePop", datastructures_pqueue_pop, "Remove and return highest priority element"},
    {"pqueuePeek", datastructures_pqueue_peek, "Get highest priority element without removing"},
    {"pqueueSize", datastructures_pqueue_size, "Get size of priority queue"},

    // Datetime Functions - Phase 2B
    {"now", datetime_now, "Get current date/time as object"},
    {"dateNow", datetime_now, "Get current date/time as object (alias)"},
    {"formatDate", datetime_format, "Format timestamp as string"},
    {"parseDate", datetime_parse, "Parse date string to timestamp"},
    {"dateAdd", datetime_add, "Add time to datetime object"},
    {"dateSubtract", datetime_subtract, "Subtract time from datetime object"},
    {"dateDiff", datetime_diff, "Calculate difference between datetime objects"},
    {"dateTimezone", datetime_timezone, "Get timezone info for datetime object"},
    {"dateUtc", datetime_utc, "Convert datetime to UTC"},

    // System/OS Functions - Phase 2B
    {"exec", system_exec, "Execute system command"},
    {"envGet", system_env_get, "Get environment variable"},
    {"envSet", system_env_set, "Set environment variable"},
    {"envList", system_env_list, "List all environment variables"},
    {"processList", system_process_list, "List running processes"},
    {"processKill", system_process_kill, "Kill a process by PID"},
    {"filesystemList", system_filesystem_list, "List files in directory"},
    {"filesystemInfo", system_filesystem_info, "Get file/directory information"},
    {"hardwareCpu", system_hardware_cpu, "Get CPU hardware information"},
    {"hardwareMemory", system_hardware_memory, "Get memory hardware information"},

    // HTTP Functions - Phase 2B
    {"httpGet", http_get_stdlib, "Perform HTTP GET request"},
    {"httpPost", http_post_stdlib, "Perform HTTP POST request"},
    {"httpPut", http_put_stdlib, "Perform HTTP PUT request"},
    {"httpDelete", http_delete_stdlib, "Perform HTTP DELETE request"},
    {"httpTimeout", http_timeout_stdlib, "Set HTTP timeout configuration"},

    // Regex Functions - Phase 2B
    {"regexMatch", regex_match_stdlib, "Match text against regex pattern"},
    {"regexReplace", regex_replace_stdlib, "Replace pattern matches with replacement"},
    {"regexSplit", regex_split_stdlib, "Split text by regex pattern"},
    {"regexCompile", regex_compile_stdlib, "Compile regex pattern for reuse"},

    // Logic and Formal Reasoning Functions - Phase 3
    {"theoremDefine", logic_theorem_define, "Define a new theorem with natural language statement"},
    {"proofVerify", logic_proof_verify, "Verify a proof for a given theorem"},
    {"axiomAdd", logic_axiom_add, "Add a new axiom to the formal logic system"},
    {"axiomList", logic_axiom_list, "List all axioms in the logic system"},
    {"axiomValidate", logic_axiom_validate, "Validate axiom consistency"},
    {"proofStep", logic_proof_step, "Add a step to interactive proof construction"},
    {"theoremStore", logic_theorem_store, "Store theorem in knowledge base"},
    {"theoremVerify", logic_theorem_verify, "Verify theorem validity and consistency"},
    {"theoremGet", logic_theorem_get, "Retrieve theorem by name from knowledge base"},
    {"theoremList", logic_theorem_list, "List all theorems in the knowledge base"},
    {"logicStats", logic_system_stats, "Get logic system statistics"},
    {"logicReset", logic_system_reset, "Reset the logic system"},

    // Advanced Logic Functions
    {"logicEntails", logic_entails, "Check if statement follows logically from premises"},
    {"logicModusPonens", logic_modus_ponens, "Apply modus ponens inference rule"},
    {"logicModusTollens", logic_modus_tollens, "Apply modus tollens inference rule"},
    {"logicEquivalent", logic_equivalent, "Check logical equivalence of statements"},
    {"logicTruthTable", logic_truth_table, "Generate truth table for logical expression"},
    {"logicParseStatement", logic_parse_statement, "Parse natural language logical statement"},
    {"logicToNatural", logic_to_natural, "Convert formal logic to natural language"},
    {"logicValidateReasoning",
     logic_validate_reasoning,
     "Validate natural language mathematical reasoning"},

    // Mathematical Reasoning Functions
    {"mathWellFormed", logic_math_well_formed, "Check if mathematical statement is well-formed"},
    {"mathApplyRule", logic_math_apply_rule, "Apply algebraic manipulation rules"},
    {"mathSimplify", logic_math_simplify, "Simplify mathematical expression"},
    {"mathEqual", logic_math_equal, "Check mathematical equality with symbolic reasoning"},

    // Proof System Utilities
    {"proofStats", logic_proof_stats, "Get proof statistics for a theorem"},
    {"proofExport", logic_proof_export, "Export proof to different formats"},
    {"proofImport", logic_proof_import, "Import theorem and proof from external format"},

    // Logging Functions
    {"logDebug", logging_debug, "Log a debug message"},
    {"logInfo", logging_info, "Log an info message"},
    {"logWarn", logging_warn, "Log a warning message"},
    {"logError", logging_error, "Log an error message"},
    {"logDebugf", logging_debugf, "Log a formatted debug message"},
    {"logInfof", logging_infof, "Log a formatted info message"},
    {"logWarnf", logging_warnf, "Log a formatted warning message"},
    {"logErrorf", logging_errorf, "Log a formatted error message"},
    {"logDebugIf", logging_debug_if, "Log debug message if condition is true"},
    {"logSetLevel", logging_set_level, "Set minimum logging level"},
    {"logWithContext", logging_with_context, "Log message with context information"},
    {"logGetLevel", logging_get_level, "Get current logging level"},

    // Module System Functions
    {"import", module_import_stdlib, "Import module by path or semantic name"},
    {"requireModule", module_require_stdlib, "Require module with error if not found"},

    // Introspection Functions

    // Sentinel - must be last
    {NULL, NULL, NULL}};

/**
 * @brief Get count of stdlib functions
 * @param None - takes no parameters
 * @return Number of stdlib functions available
 */
size_t stdlib_count()
{
    size_t count = 0;
    while (stdlib_functions[count].name != NULL) {
        count++;
    }
    return count;
}

/**
 * @brief Look up stdlib function by name
 * @param name Function name to look up
 * @return Pointer to stdlib function entry, or NULL if not found
 */
const ZenStdlibFunction *stdlib_get(const char *name)
{
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; stdlib_functions[i].name != NULL; i++) {
        if (strcmp(stdlib_functions[i].name, name) == 0) {
            return &stdlib_functions[i];
        }
    }

    return NULL;
}

/**
 * @brief Get all stdlib functions for runtime discovery
 * @param None - takes no parameters
 * @return Pointer to array of all stdlib functions
 */
const ZenStdlibFunction *stdlib_get_all()
{
    // Return the complete array of stdlib functions
    // The array is NULL-terminated for safe iteration
    // This enables runtime discovery of all available stdlib functions
    return stdlib_functions;
}