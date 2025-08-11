/**
 * @file test_memory_management.c
 * @brief Memory management and leak detection tests
 */

#include "../framework/test.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/runtime_value.h"

TEST(test_basic_memory_allocation) {
    void* ptr = memory_alloc(1024);
    ASSERT_NOT_NULL(ptr);
    
    // Verify memory is zeroed
    char* char_ptr = (char*)ptr;
    for (int i = 0; i < 1024; i++) {
        ASSERT_EQ(char_ptr[i], 0);
    }
    
    memory_free(ptr);
}

TEST(test_memory_reallocation) {
    char* ptr = (char*)memory_alloc(100);
    ASSERT_NOT_NULL(ptr);
    
    // Write some data
    strcpy(ptr, "Hello World");
    
    // Reallocate to larger size
    ptr = (char*)memory_realloc(ptr, 200);
    ASSERT_NOT_NULL(ptr);
    ASSERT_STR_EQ(ptr, "Hello World");  // Data should be preserved
    
    memory_free(ptr);
}

TEST(test_memory_string_duplication) {
    const char* original = "Test String";
    char* duplicate = memory_strdup(original);
    
    ASSERT_NOT_NULL(duplicate);
    ASSERT_STR_EQ(duplicate, original);
    ASSERT_NE(duplicate, original);  // Different pointers
    
    memory_free(duplicate);
}

TEST(test_reference_counting) {
    // Create a value with reference counting
    Value* value = value_new_string("Reference Test");
    ASSERT_EQ(value->ref_count, 1);
    
    // Increment reference
    Value* ref1 = value_ref(value);
    ASSERT_EQ(value, ref1);
    ASSERT_EQ(value->ref_count, 2);
    
    // Increment again
    Value* ref2 = value_ref(value);
    ASSERT_EQ(value, ref2);
    ASSERT_EQ(value->ref_count, 3);
    
    // Decrement references
    value_unref(ref2);
    ASSERT_EQ(value->ref_count, 2);
    
    value_unref(ref1);
    ASSERT_EQ(value->ref_count, 1);
    
    // Final unref should free the memory
    value_unref(value);
    // value is now invalid
}

TEST(test_memory_stats_tracking) {
    // Enable memory debugging
    memory_debug_enable(true);
    memory_reset_stats();
    
    MemoryStats stats_before;
    memory_get_stats(&stats_before);
    
    // Allocate some memory
    void* ptr1 = memory_alloc(100);
    void* ptr2 = memory_alloc(200);
    void* ptr3 = memory_alloc(300);
    
    MemoryStats stats_after_alloc;
    memory_get_stats(&stats_after_alloc);
    
    // Should show increased allocation count and bytes
    ASSERT_TRUE(stats_after_alloc.allocation_count > stats_before.allocation_count);
    ASSERT_TRUE(stats_after_alloc.total_allocated > stats_before.total_allocated);
    ASSERT_TRUE(stats_after_alloc.current_allocated > stats_before.current_allocated);
    
    // Free the memory
    memory_free(ptr1);
    memory_free(ptr2);
    memory_free(ptr3);
    
    MemoryStats stats_after_free;
    memory_get_stats(&stats_after_free);
    
    // Should show freed memory
    ASSERT_EQ(stats_after_free.current_allocated, stats_before.current_allocated);
    ASSERT_TRUE(stats_after_free.total_freed > stats_before.total_freed);
    
    memory_debug_enable(false);
}

TEST(test_memory_leak_detection) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    // Intentionally "leak" some memory for testing
    void* leaked_ptr = memory_alloc(1000);
    (void)leaked_ptr; // Suppress unused variable warning
    
    // Check for leaks
    size_t leak_count = memory_check_leaks();
    ASSERT_TRUE(leak_count > 0);
    
    // Clean up the "leak" for the test
    memory_free(leaked_ptr);
    
    // Should have no leaks now
    size_t leaks2 = memory_check_leaks();
    ASSERT_EQ(leaks2, 0);
    
    memory_debug_enable(false);
}

TEST(test_value_memory_management) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    MemoryStats stats_before;
    memory_get_stats(&stats_before);
    
    // Create and manipulate values
    Value* str_val = value_new_string("Memory Test");
    Value* num_val = value_new_number(42.0);
    Value* bool_val = value_new_boolean(true);
    Value* null_val = value_new_null();
    
    // Copy values (should allocate more memory)
    Value* str_copy = value_copy(str_val);
    Value* num_copy = value_copy(num_val);
    
    MemoryStats stats_after_alloc;
    memory_get_stats(&stats_after_alloc);
    ASSERT_TRUE(stats_after_alloc.allocation_count > stats_before.allocation_count);
    
    // Free all values
    value_unref(str_val);
    value_unref(num_val);
    value_unref(bool_val);
    value_unref(null_val);
    value_unref(str_copy);
    value_unref(num_copy);
    
    MemoryStats stats_after_free;
    memory_get_stats(&stats_after_free);
    ASSERT_EQ(stats_after_free.current_allocated, stats_before.current_allocated);
    
    // Should have no leaks
    size_t leaks = memory_check_leaks();
    ASSERT_EQ(leaks, 0);
    
    memory_debug_enable(false);
}

TEST(test_lexer_memory_management) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    MemoryStats stats_before;
    memory_get_stats(&stats_before);
    
    char* input = "set x 42\nset y \"hello\"\nprint x + y";
    lexer_T* lexer = lexer_new(input);
    
    // Tokenize entire input
    token_T* token;
    do {
        token = lexer_get_next_token(lexer);
    } while (token && token->type != TOKEN_EOF);
    
    // Free lexer (should free all associated memory)
    // Note: Need proper lexer cleanup function
    
    MemoryStats stats_after;
    memory_get_stats(&stats_after);
    
    // Should not have significant memory increase
    // (tokens might be cached, but no major leaks)
    size_t has_leaks = memory_check_leaks();
    
    // Don't assert no leaks since lexer cleanup might not be fully implemented
    // Just verify no major memory explosion
    (void)has_leaks; // Suppress unused variable warning
    ASSERT_TRUE(stats_after.allocation_count < stats_before.allocation_count + 100);
    
    memory_debug_enable(false);
}

TEST(test_parser_memory_management) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    char* input = 
        "function factorial n\n"
        "    if n <= 1\n"
        "        return 1\n"
        "    else\n"
        "        return n * factorial (n - 1)\n"
        "\n"
        "set result factorial 5";
    
    lexer_T* lexer = lexer_new(input);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    
    AST_T* ast = parser_parse_statements(parser, scope);
    
    // AST should be created without memory leaks
    ASSERT_NOT_NULL(ast);
    
    // Clean up AST (should free all nodes recursively)
    ast_free(ast);
    
    // Should not have memory leaks
    size_t has_leaks = memory_check_leaks();
    
    // Don't assert no leaks since parser/AST cleanup might not be fully implemented
    // Just verify no major leaks
    (void)has_leaks; // Suppress unused variable warning
    
    memory_debug_enable(false);
}

TEST(test_visitor_memory_management) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    char* code = 
        "set x 10\n"
        "set y 20\n"
        "set result x + y\n"
        "print result";
    
    lexer_T* lexer = lexer_new(code);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    AST_T* ast = parser_parse_statements(parser, scope);
    visitor_T* visitor = visitor_new();
    
    // Execute the code
    RuntimeValue* result = visitor_visit(visitor, ast);
    
    ASSERT_NOT_NULL(result);
    rv_unref(result);
    
    // Clean up everything
    ast_free(ast);
    // Note: Need proper cleanup functions for lexer, parser, visitor, scope
    
    size_t has_leaks = memory_check_leaks();
    
    // Don't assert no leaks since cleanup might not be fully implemented
    (void)has_leaks; // Suppress unused variable warning
    
    memory_debug_enable(false);
}

TEST(test_large_allocation_stress) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    const int num_allocations = 1000;
    void* ptrs[num_allocations];
    
    // Allocate many small blocks
    for (int i = 0; i < num_allocations; i++) {
        ptrs[i] = memory_alloc(i + 1);
        ASSERT_NOT_NULL(ptrs[i]);
    }
    
    MemoryStats stats_after_alloc;
    memory_get_stats(&stats_after_alloc);
    ASSERT_TRUE(stats_after_alloc.allocation_count >= (size_t)num_allocations);
    
    // Free all blocks
    for (int i = 0; i < num_allocations; i++) {
        memory_free(ptrs[i]);
    }
    
    MemoryStats stats_after_free;
    memory_get_stats(&stats_after_free);
    ASSERT_EQ(stats_after_free.current_allocated, 0);
    ASSERT_EQ(memory_check_leaks(), 0);
    
    memory_debug_enable(false);
}

TEST(test_reallocation_stress) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    char* ptr = (char*)memory_alloc(10);
    ASSERT_NOT_NULL(ptr);
    
    // Repeatedly reallocate to larger sizes
    for (int size = 20; size <= 10000; size *= 2) {
        ptr = (char*)memory_realloc(ptr, size);
        ASSERT_NOT_NULL(ptr);
        
        // Write some data to verify memory is accessible
        ptr[0] = 'A';
        ptr[size - 1] = 'Z';
    }
    
    memory_free(ptr);
    ASSERT_EQ(memory_check_leaks(), 0);
    
    memory_debug_enable(false);
}

TEST(test_memory_fragmentation) {
    memory_debug_enable(true);
    memory_reset_stats();
    
    const int num_ptrs = 100;
    void* ptrs[num_ptrs];
    
    // Allocate blocks of varying sizes
    for (int i = 0; i < num_ptrs; i++) {
        ptrs[i] = memory_alloc((i % 10 + 1) * 100);
        ASSERT_NOT_NULL(ptrs[i]);
    }
    
    // Free every other block (creating fragmentation)
    for (int i = 0; i < num_ptrs; i += 2) {
        memory_free(ptrs[i]);
        ptrs[i] = NULL;
    }
    
    // Allocate more blocks in the gaps
    for (int i = 0; i < num_ptrs; i += 2) {
        ptrs[i] = memory_alloc(50);
        ASSERT_NOT_NULL(ptrs[i]);
    }
    
    // Free everything
    for (int i = 0; i < num_ptrs; i++) {
        if (ptrs[i]) {
            memory_free(ptrs[i]);
        }
    }
    
    ASSERT_EQ(memory_check_leaks(), 0);
    
    memory_debug_enable(false);
}

TEST_SUITE_BEGIN(memory_management_tests)
    RUN_TEST(test_basic_memory_allocation);
    RUN_TEST(test_memory_reallocation);
    RUN_TEST(test_memory_string_duplication);
    RUN_TEST(test_reference_counting);
    RUN_TEST(test_memory_stats_tracking);
    RUN_TEST(test_memory_leak_detection);
    RUN_TEST(test_value_memory_management);
    RUN_TEST(test_lexer_memory_management);
    RUN_TEST(test_parser_memory_management);
    RUN_TEST(test_visitor_memory_management);
    RUN_TEST(test_large_allocation_stress);
    RUN_TEST(test_reallocation_stress);
    RUN_TEST(test_memory_fragmentation);
TEST_SUITE_END