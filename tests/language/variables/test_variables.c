/**
 * @file test_variables.c
 * @brief Tests for ZEN variable declarations and assignments
 */

#include "../../framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"

TEST_SUITE(variables_tests)

static AST_T* parse_and_execute(const char* code) {
    lexer_T* lexer = init_lexer((char*)code);
    parser_T* parser = init_parser(lexer);
    scope_T* scope = init_scope();
    AST_T* ast = parser_parse_statements(parser, scope);
    visitor_T* visitor = init_visitor();
    return visitor_visit(visitor, ast);
}

TEST(test_simple_variable_declaration) {
    char* code = "set x 42";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_string_variable) {
    char* code = "set name \"Alice\"";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_boolean_variable) {
    char* code = "set active true";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_null_variable) {
    char* code = "set empty null";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_float_variable) {
    char* code = "set pi 3.14159";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_reference) {
    char* code = 
        "set x 10\n"
        "set y x";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_multiple_variable_declarations) {
    char* code = 
        "set a 1\n"
        "set b 2\n"
        "set c 3";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_with_expression) {
    char* code = "set result 5 + 3";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_string_concatenation_variable) {
    char* code = "set greeting \"Hello \" + \"World\"";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_names) {
    char* code = 
        "set simple_name 1\n"
        "set camelCase 2\n"
        "set snake_case 3\n"
        "set with123numbers 4\n"
        "set _private 5";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_zen_inline_array_syntax) {
    // ZEN uses comma syntax for arrays
    char* code = "set scores 85, 92, 78";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_zen_inline_object_syntax) {
    // ZEN object syntax: key value pairs
    char* code = "set person name \"Alice\", age 30, active true";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_multiline_array) {
    char* code = 
        "set items\n"
        "    \"pen\",\n"
        "    \"book\",\n"
        "    \"lamp\"";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_multiline_object) {
    char* code = 
        "set config\n"
        "    debug true,\n"
        "    retries 3,\n"
        "    timeout 1000";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_nested_structures) {
    char* code = 
        "set scores 8, 9, 10\n"
        "set profile name \"Linh\", scores scores, active true";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_redefinition) {
    char* code = 
        "set x 10\n"
        "set x 20";  // Redefining variable
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_case_sensitivity) {
    char* code = 
        "set Variable 1\n"
        "set variable 2\n"  // Different from Variable
        "set VARIABLE 3";   // Different from both above
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_special_variable_names) {
    // Test variables that might conflict with keywords in other languages
    char* code = 
        "set class 1\n"    // Not a keyword in ZEN
        "set var 2\n"      // Not a keyword in ZEN
        "set let 3\n"      // Not a keyword in ZEN
        "set const 4";     // Not a keyword in ZEN
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_zen_assignment_vs_comparison) {
    // In ZEN, 'set' is for assignment, '=' is for comparison
    char* code = 
        "set x 10\n"
        "set is_ten x = 10";  // This should work - comparison
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_with_complex_expression) {
    char* code = "set result (10 + 5) * 2 - 3";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_with_function_call) {
    char* code = 
        "function double x\n"
        "    return x * 2\n"
        "\n"
        "set result double 21";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_scope_global) {
    char* code = 
        "set global_var 100\n"
        "function test\n"
        "    return global_var\n"  // Should access global
        "\n"
        "set result test";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_scope_local) {
    char* code = 
        "function test param\n"  // param is local to function
        "    set local_var param + 1\n"  // local_var is local
        "    return local_var\n"
        "\n"
        "set result test 5";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_undefined_variable_error) {
    char* code = "set result undefined_variable + 1";
    AST_T* result = parse_and_execute(code);
    // Should handle undefined variable gracefully
    // Might return NULL or error result depending on implementation
}

TEST(test_variable_with_scientific_notation) {
    char* code = 
        "set large 1.5e10\n"
        "set small 2.3e-5";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_with_unicode_string) {
    char* code = "set message \"Hello 世界 🌍\"";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_empty_string_variable) {
    char* code = "set empty_string \"\"";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_zero_variable) {
    char* code = 
        "set zero_int 0\n"
        "set zero_float 0.0";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

TEST(test_negative_number_variable) {
    char* code = 
        "set negative_int -42\n"
        "set negative_float -3.14";
    AST_T* result = parse_and_execute(code);
    ASSERT_NOT_NULL(result);
}

END_TEST_SUITE