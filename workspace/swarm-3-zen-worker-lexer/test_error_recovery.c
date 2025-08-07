#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "zen/core/lexer.h"

/**
 * Test error recovery functions in lexer
 */
int main() {
    printf("Testing lexer error recovery functions...\n");
    
    // Test basic lexer creation
    char* test_input = "set x 42\nset y \"invalid string\nset z 10";
    lexer_T* lexer = init_lexer(test_input);
    assert(lexer != NULL);
    
    // Test 1: Initially not in error recovery mode
    printf("Test 1: Initial state - ");
    assert(!lexer_in_error_recovery(lexer));
    printf("PASS\n");
    
    // Test 2: Enter error recovery mode
    printf("Test 2: Enter error recovery - ");
    lexer_enter_error_recovery(lexer, "Test error message");
    assert(lexer_in_error_recovery(lexer));
    assert(lexer->error_recovery_mode == true);
    assert(lexer->error_message != NULL);
    assert(strcmp(lexer->error_message, "Test error message") == 0);
    printf("PASS\n");
    
    // Test 3: Error location is recorded
    printf("Test 3: Error location recording - ");
    assert(lexer->last_error.line == lexer->line_number);
    assert(lexer->last_error.column == lexer->column_number);
    assert(lexer->last_error.position == lexer->i);
    printf("PASS\n");
    
    // Test 4: Exit error recovery mode
    printf("Test 4: Exit error recovery - ");
    lexer_exit_error_recovery(lexer);
    assert(!lexer_in_error_recovery(lexer));
    assert(lexer->error_recovery_mode == false);
    assert(lexer->error_message == NULL);
    assert(lexer->last_error.line == 0);
    assert(lexer->last_error.column == 0);
    assert(lexer->last_error.position == 0);
    printf("PASS\n");
    
    // Test 5: Null pointer safety
    printf("Test 5: Null pointer safety - ");
    lexer_enter_error_recovery(NULL, "test");
    lexer_exit_error_recovery(NULL);
    assert(!lexer_in_error_recovery(NULL));
    printf("PASS\n");
    
    // Test 6: Error message replacement
    printf("Test 6: Error message replacement - ");
    lexer_enter_error_recovery(lexer, "First error");
    assert(strcmp(lexer->error_message, "First error") == 0);
    lexer_enter_error_recovery(lexer, "Second error");
    assert(strcmp(lexer->error_message, "Second error") == 0);
    printf("PASS\n");
    
    // Test 7: NULL error message handling
    printf("Test 7: NULL error message handling - ");
    lexer_enter_error_recovery(lexer, NULL);
    assert(lexer->error_message == NULL);
    assert(lexer_in_error_recovery(lexer)); // Still in error recovery mode
    printf("PASS\n");
    
    lexer_free(lexer);
    
    printf("\nAll error recovery tests PASSED!\n");
    printf("✓ lexer_enter_error_recovery() properly sets state and stores error info\n");
    printf("✓ lexer_exit_error_recovery() properly cleans up and resets state\n");
    printf("✓ lexer_in_error_recovery() correctly reports recovery state\n");
    printf("✓ Functions handle NULL pointers safely\n");
    printf("✓ Error messages are properly managed with memory allocation\n");
    
    return 0;
}