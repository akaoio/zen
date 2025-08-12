/**
 * @file test_variables.c
 * @brief Tests for ZEN variable declarations and assignments
 */

#include "../../framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

// Use the same pattern as integration tests but with explicit double-free protection
static RuntimeValue* execute_code_safe(const char* code) {
    lexer_T* lexer = lexer_new((char*)code);
    if (!lexer) return NULL;
    
    parser_T* parser = parser_new(lexer);
    if (!parser) {
        lexer_free(lexer);
        return NULL;
    }
    
    scope_T* scope = scope_new();
    if (!scope) {
        parser_free(parser);
        lexer_free(lexer);
        return NULL;
    }
    
    AST_T* ast = parser_parse_statements(parser, scope);
    if (!ast) {
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return NULL;
    }
    
    visitor_T* visitor = visitor_new();
    if (!visitor) {
        ast_free(ast);
        scope_free(scope);
        parser_free(parser);
        lexer_free(lexer);
        return NULL;
    }
    
    RuntimeValue* result = visitor_visit(visitor, ast);
    
    // Clean up resources
    visitor_free(visitor);
    
    // CRITICAL FIX: Don't free scope here - parser_free will free parser->scope
    // The scope we created is used by AST nodes but not owned by them
    // We need to free it after AST is freed to avoid use-after-free
    
    // Free AST first (it might reference the scope but doesn't own it)
    ast_free(ast);
    
    // Now free the scope we created (not owned by parser)
    scope_free(scope);
    
    // Free parser (this frees parser->scope which is different from our scope)
    parser_free(parser);
    lexer_free(lexer);
    
    return result;
}

TEST(test_simple_variable_declaration) {
    char* code = "set x 42";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_string_variable) {
    char* code = "set name \"Alice\"";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_boolean_variable) {
    char* code = "set active true";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_null_variable) {
    char* code = "set empty null";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_float_variable) {
    char* code = "set pi 3.14159";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_reference) {
    char* code = 
        "set x 10\n"
        "set y x";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_multiple_variable_declarations) {
    char* code = 
        "set a 1\n"
        "set b 2\n"
        "set c 3";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_with_expression) {
    char* code = "set result 5 + 3";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_string_concatenation_variable) {
    char* code = "set greeting \"Hello \" + \"World\"";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_names) {
    char* code = 
        "set simple_name 1\n"
        "set camelCase 2\n"
        "set snake_case 3\n"
        "set with123numbers 4\n"
        "set _private 5";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_zen_inline_array_syntax) {
    // ZEN uses comma syntax for arrays
    char* code = "set scores 85, 92, 78";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_zen_inline_object_syntax) {
    // ZEN object syntax: key value pairs
    char* code = "set person name \"Alice\", age 30, active true";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_multiline_array) {
    char* code = 
        "set items\n"
        "    \"pen\",\n"
        "    \"book\",\n"
        "    \"lamp\"";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_multiline_object) {
    char* code = 
        "set config\n"
        "    debug true,\n"
        "    retries 3,\n"
        "    timeout 1000";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_nested_structures) {
    char* code = 
        "set scores 8, 9, 10\n"
        "set profile name \"Linh\", scores scores, active true";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_redefinition) {
    char* code = 
        "set x 10\n"
        "set x 20";  // Redefining variable
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_case_sensitivity) {
    char* code = 
        "set Variable 1\n"
        "set variable 2\n"  // Different from Variable
        "set VARIABLE 3";   // Different from both above
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_special_variable_names) {
    // Test variables that might conflict with keywords in other languages
    char* code = 
        "set class 1\n"    // Not a keyword in ZEN
        "set var 2\n"      // Not a keyword in ZEN
        "set let 3\n"      // Not a keyword in ZEN
        "set const 4";     // Not a keyword in ZEN
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_zen_assignment_vs_comparison) {
    // In ZEN, 'set' is for assignment, '=' is for comparison
    char* code = 
        "set x 10\n"
        "set is_ten x = 10";  // This should work - comparison
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_with_complex_expression) {
    char* code = "set result (10 + 5) * 2 - 3";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_with_function_call) {
    char* code = 
        "function double x\n"
        "    return x * 2\n"
        "\n"
        "set result double 21";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_scope_global) {
    char* code = 
        "set global_var 100\n"
        "function test\n"
        "    return global_var\n"  // Should access global
        "\n"
        "set result test";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_scope_local) {
    char* code = 
        "function test param\n"  // param is local to function
        "    set local_var param + 1\n"  // local_var is local
        "    return local_var\n"
        "\n"
        "set result test 5";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_undefined_variable_error) {
    char* code = "set result undefined_variable + 1";
    RuntimeValue* result = execute_code_safe(code);
    (void)result; // Suppress unused variable warning
    // Should handle undefined variable gracefully
    // Might return NULL or error result depending on implementation
    if (result) {
        rv_unref(result);
    }
}

TEST(test_variable_with_scientific_notation) {
    char* code = 
        "set large 1.5e10\n"
        "set small 2.3e-5";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_variable_with_unicode_string) {
    char* code = "set message \"Hello 世界 🌍\"";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_empty_string_variable) {
    char* code = "set empty_string \"\"";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_zero_variable) {
    char* code = 
        "set zero_int 0\n"
        "set zero_float 0.0";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST(test_negative_number_variable) {
    char* code = 
        "set negative_int -42\n"
        "set negative_float -3.14";
    RuntimeValue* result = execute_code_safe(code);
    ASSERT_NOT_NULL(result);
    rv_unref(result);
}

TEST_SUITE_BEGIN(variables_tests)
    RUN_TEST(test_simple_variable_declaration);
    RUN_TEST(test_string_variable);
    RUN_TEST(test_boolean_variable);
    RUN_TEST(test_null_variable);
    RUN_TEST(test_float_variable);
    RUN_TEST(test_variable_reference);
    RUN_TEST(test_multiple_variable_declarations);
    RUN_TEST(test_variable_with_expression);
    RUN_TEST(test_string_concatenation_variable);
    RUN_TEST(test_variable_names);
    RUN_TEST(test_zen_inline_array_syntax);
    RUN_TEST(test_zen_inline_object_syntax);
    RUN_TEST(test_multiline_array);
    RUN_TEST(test_multiline_object);
    RUN_TEST(test_nested_structures);
    RUN_TEST(test_variable_redefinition);
    RUN_TEST(test_variable_case_sensitivity);
    RUN_TEST(test_special_variable_names);
    RUN_TEST(test_zen_assignment_vs_comparison);
    RUN_TEST(test_variable_with_complex_expression);
    RUN_TEST(test_variable_with_function_call);
    RUN_TEST(test_variable_scope_global);
    RUN_TEST(test_variable_scope_local);
    RUN_TEST(test_undefined_variable_error);
    RUN_TEST(test_variable_with_scientific_notation);
    RUN_TEST(test_variable_with_unicode_string);
    RUN_TEST(test_empty_string_variable);
    RUN_TEST(test_zero_variable);
    RUN_TEST(test_negative_number_variable);
TEST_SUITE_END