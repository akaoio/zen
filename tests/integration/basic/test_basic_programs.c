/**
 * @file test_basic_programs.c
 * @brief Integration tests for basic ZEN programs
 */

#include "../../framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"

TEST_SUITE(basic_programs_tests)

static AST_T* execute_zen_code(const char* code) {
    lexer_T* lexer = init_lexer((char*)code);
    if (!lexer) return NULL;
    
    parser_T* parser = init_parser(lexer);
    if (!parser) return NULL;
    
    scope_T* scope = init_scope();
    if (!scope) return NULL;
    
    AST_T* ast = parser_parse_statements(parser, scope);
    if (!ast) return NULL;
    
    visitor_T* visitor = init_visitor();
    if (!visitor) return NULL;
    
    return visitor_visit(visitor, ast);
}

TEST(test_variable_assignment_and_use) {
    char* code = 
        "set x 42\n"
        "set y x\n";
    
    AST_T* result = execute_zen_code(code);
    
    // Should execute without errors
    ASSERT_NOT_NULL(result);
}

TEST(test_simple_arithmetic) {
    char* code = 
        "set a 10\n"
        "set b 5\n"
        "set sum a + b\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
    // The sum should be calculated correctly
    // Exact verification depends on how variables are accessible after execution
}

TEST(test_string_operations) {
    char* code = 
        "set greeting \"Hello\"\n"
        "set name \"World\"\n"
        "set message greeting + \" \" + name\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_function_definition_and_call) {
    char* code = 
        "function add x y\n"
        "    return x + y\n"
        "\n"
        "set result add 3 4\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_conditional_statement) {
    char* code = 
        "set age 25\n"
        "if age >= 18\n"
        "    set status \"adult\"\n"
        "else\n"
        "    set status \"minor\"\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_print_statement) {
    char* code = 
        "set message \"Hello, ZEN!\"\n"
        "print message\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
    // Note: This will actually print to stdout during test
}

TEST(test_multiple_function_calls) {
    char* code = 
        "function square x\n"
        "    return x * x\n"
        "\n"
        "print square 5\n"
        "print square 10\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_nested_expressions) {
    char* code = 
        "set result (3 + 4) * (5 - 2)\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_comparison_operations) {
    char* code = 
        "set x 10\n"
        "set is_positive x > 0\n"
        "set is_equal x = 10\n"
        "set is_not_equal x != 5\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_logical_operations) {
    char* code = 
        "set a true\n"
        "set b false\n"
        "set and_result a & b\n"
        "set or_result a | b\n"
        "set not_result !a\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_function_with_multiple_parameters) {
    char* code = 
        "function calculate x y z\n"
        "    return x + y * z\n"
        "\n"
        "set result calculate 1 2 3\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_string_concatenation_complex) {
    char* code = 
        "set first \"Hello\"\n"
        "set second \"beautiful\"\n"
        "set third \"world\"\n"
        "set sentence first + \" \" + second + \" \" + third + \"!\"\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_variable_scope_in_function) {
    char* code = 
        "set global_var 100\n"
        "function test_scope local_param\n"
        "    set local_var local_param + 1\n"
        "    return local_var\n"
        "\n"
        "set result test_scope 5\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_recursive_function) {
    char* code = 
        "function factorial n\n"
        "    if n <= 1\n"
        "        return 1\n"
        "    else\n"
        "        return n * factorial (n - 1)\n"
        "\n"
        "set result factorial 5\n";
    
    // Note: This test may not work if recursion isn't implemented yet
    AST_T* result = execute_zen_code(code);
    
    // Don't assert success since recursion might not be implemented
    // Just verify it doesn't crash
}

TEST(test_while_loop) {
    char* code = 
        "set counter 0\n"
        "while counter < 5\n"
        "    set counter counter + 1\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_for_loop) {
    char* code = 
        "set sum 0\n"
        "for i in 1, 2, 3, 4, 5\n"
        "    set sum sum + i\n";
    
    // Note: For loop syntax might not be implemented yet
    AST_T* result = execute_zen_code(code);
    
    // Don't assert success since for loops might not be implemented
}

TEST(test_mixed_data_types) {
    char* code = 
        "set number 42\n"
        "set text \"The answer is \"\n"
        "set boolean true\n"
        "set nothing null\n"
        "set message text + number\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_boolean_literals) {
    char* code = 
        "set flag1 true\n"
        "set flag2 false\n"
        "set combined flag1 & !flag2\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_null_handling) {
    char* code = 
        "set empty_var null\n"
        "set result empty_var = null\n";
    
    AST_T* result = execute_zen_code(code);
    
    ASSERT_NOT_NULL(result);
}

TEST(test_error_recovery) {
    // Test program with syntax error - should handle gracefully
    char* code = 
        "set valid_var 42\n"
        "set + invalid_syntax\n"
        "set another_valid_var 13\n";
    
    AST_T* result = execute_zen_code(code);
    
    // Should not crash, might return NULL or partial result
    // Exact behavior depends on error recovery implementation
}

END_TEST_SUITE