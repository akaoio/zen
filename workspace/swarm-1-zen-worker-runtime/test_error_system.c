/*
 * test_error_system.c
 * Test program for the comprehensive ZEN error handling system
 * 
 * This demonstrates:
 * - Enhanced error creation with location information
 * - Error chaining and cause tracking
 * - Conversion between detailed and simple errors
 * - Integration with Value system
 * - Source context display
 */

#include "src/include/zen/core/error.h"
#include <stdio.h>
#include <stdlib.h>

void test_basic_error_creation(void) {
    printf("=== Testing Basic Error Creation ===\n");
    
    // Create a detailed error
    ZenDetailedError* error = error_create(ZEN_ERROR_PARSER_UNEXPECTED_TOKEN, 
                                          "Expected ';' but found '{'");
    
    if (error) {
        printf("Created error: ");
        error_print(error);
        printf("Error code name: %s\n", error_code_name(error->code));
        printf("Error description: %s\n", error_code_description(error->code));
        error_detailed_unref(error);
    }
    
    printf("\n");
}

void test_error_with_location(void) {
    printf("=== Testing Error with Location ===\n");
    
    // Create error with location information
    ZenDetailedError* error = error_create_with_location(
        ZEN_ERROR_LEXER_UNTERMINATED_STRING,
        "String literal not terminated",
        "test.zen",
        42,
        15
    );
    
    if (error) {
        // Add source context
        error_set_source_context(error, "    set name \"Hello World");
        
        printf("Error with location and context:\n");
        error_print_with_context(error);
        error_detailed_unref(error);
    }
    
    printf("\n");
}

void test_error_chaining(void) {
    printf("=== Testing Error Chaining ===\n");
    
    // Create a root cause error
    ZenDetailedError* root_cause = error_create_with_location(
        ZEN_ERROR_SYSTEM_FILE_NOT_FOUND,
        "Could not open 'config.zen'",
        "config.zen",
        0,
        0
    );
    
    // Create a higher-level error that chains to the root cause
    ZenDetailedError* higher_error = error_create_with_cause(
        ZEN_ERROR_PARSER_INVALID_STATEMENT,
        "Failed to parse configuration",
        root_cause
    );
    
    if (higher_error) {
        printf("Error chain:\n");
        error_print(higher_error);
        error_detailed_unref(higher_error);
    }
    
    // root_cause is automatically unreferenced when higher_error is freed
    
    printf("\n");
}

void test_error_conversion(void) {
    printf("=== Testing Error Conversion ===\n");
    
    // Create detailed error
    ZenDetailedError* detailed = error_create_with_location(
        ZEN_ERROR_RUNTIME_TYPE_ERROR,
        "Cannot add number and string",
        "test.zen",
        10,
        5
    );
    
    if (detailed) {
        // Convert to simple error for compatibility
        ZenError* simple = error_detailed_to_simple(detailed);
        
        if (simple) {
            printf("Simple error: ");
            error_simple_print(simple);
            
            // Convert back to detailed
            ZenDetailedError* back_to_detailed = error_simple_to_detailed(simple);
            
            if (back_to_detailed) {
                printf("Converted back to detailed: ");
                error_print(back_to_detailed);
                error_detailed_unref(back_to_detailed);
            }
            
            error_simple_free(simple);
        }
        
        error_detailed_unref(detailed);
    }
    
    printf("\n");
}

void test_error_categories(void) {
    printf("=== Testing Error Categories ===\n");
    
    ZenDetailedError* lexer_error = error_create(ZEN_ERROR_LEXER_INVALID_CHAR, "Invalid character");
    ZenDetailedError* parser_error = error_create(ZEN_ERROR_PARSER_UNEXPECTED_TOKEN, "Unexpected token");
    ZenDetailedError* runtime_error = error_create(ZEN_ERROR_RUNTIME_DIVISION_BY_ZERO, "Division by zero");
    
    if (lexer_error && parser_error && runtime_error) {
        printf("Lexer error is lexer category: %s\n", 
               error_is_category(lexer_error, ZEN_ERROR_LEXER_INVALID_CHAR) ? "true" : "false");
        printf("Parser error is parser category: %s\n",
               error_is_category(parser_error, ZEN_ERROR_PARSER_UNEXPECTED_TOKEN) ? "true" : "false");
        printf("Runtime error is runtime category: %s\n",
               error_is_category(runtime_error, ZEN_ERROR_RUNTIME_TYPE_ERROR) ? "true" : "false");
        
        // Cross-category tests (should be false)
        printf("Lexer error is parser category: %s\n",
               error_is_category(lexer_error, ZEN_ERROR_PARSER_UNEXPECTED_TOKEN) ? "true" : "false");
        
        error_detailed_unref(lexer_error);
        error_detailed_unref(parser_error);
        error_detailed_unref(runtime_error);
    }
    
    printf("\n");
}

void test_value_integration(void) {
    printf("=== Testing Value System Integration ===\n");
    
    // Create detailed error
    ZenDetailedError* detailed = error_create(ZEN_ERROR_VALUE_CONVERSION, 
                                             "Cannot convert string to number");
    
    if (detailed) {
        // Convert to Value
        Value* error_value = error_detailed_to_value(detailed);
        
        if (error_value) {
            printf("Created VALUE_ERROR from detailed error\n");
            printf("Value type: %s\n", value_type_name(error_value->type));
            
            if (error_value->as.error) {
                printf("Error in value - Code: %d, Message: %s\n",
                       error_value->as.error->code,
                       error_value->as.error->message ? error_value->as.error->message : "No message");
            }
            
            // Convert back to detailed error
            ZenDetailedError* back_detailed = error_detailed_from_value(error_value);
            if (back_detailed) {
                printf("Converted back from Value: ");
                error_print(back_detailed);
                error_detailed_unref(back_detailed);
            }
            
            value_unref(error_value);
        }
        
        error_detailed_unref(detailed);
    }
    
    printf("\n");
}

void global_error_handler(const ZenDetailedError* error) {
    printf("[GLOBAL HANDLER] ");
    error_print(error);
}

void test_global_error_handler(void) {
    printf("=== Testing Global Error Handler ===\n");
    
    // Set global error handler
    error_set_global_handler(global_error_handler);
    
    // Create and report errors
    ZenDetailedError* error1 = error_create(ZEN_ERROR_SYSTEM_OUT_OF_MEMORY, "Memory allocation failed");
    ZenDetailedError* error2 = error_create_with_location(ZEN_ERROR_LEXER_INVALID_NUMBER, 
                                                         "Invalid number format", "test.zen", 5, 10);
    
    if (error1 && error2) {
        printf("Reporting errors to global handler:\n");
        error_report_global(error1);
        error_report_global(error2);
        
        error_detailed_unref(error1);
        error_detailed_unref(error2);
    }
    
    // Clear global handler
    error_set_global_handler(NULL);
    
    printf("\n");
}

int main(void) {
    printf("ZEN Error System Test Suite\n");
    printf("===========================\n\n");
    
    test_basic_error_creation();
    test_error_with_location();
    test_error_chaining();
    test_error_conversion();
    test_error_categories();
    test_value_integration();
    test_global_error_handler();
    
    printf("All tests completed successfully!\n");
    return 0;
}