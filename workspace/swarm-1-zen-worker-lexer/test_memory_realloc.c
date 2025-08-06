#include "zen/core/memory.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main() {
    printf("Testing memory_realloc functionality...\n");
    
    // Test 1: Basic realloc
    void* ptr = memory_alloc(10);
    assert(ptr != NULL);
    strcpy((char*)ptr, "hello");
    
    ptr = memory_realloc(ptr, 20);
    assert(ptr != NULL);
    assert(strcmp((char*)ptr, "hello") == 0);
    printf("✓ Basic realloc test passed\n");
    
    // Test 2: Expanding realloc multiple times
    for (size_t size = 40; size <= 1000; size *= 2) {
        ptr = memory_realloc(ptr, size);
        assert(ptr != NULL);
        assert(strcmp((char*)ptr, "hello") == 0);
    }
    printf("✓ Multiple expansion test passed\n");
    
    // Test 3: String building with realloc (similar to lexer usage)
    memory_free(ptr);
    
    char* str = memory_alloc(1);
    str[0] = '\0';
    size_t len = 0;
    size_t capacity = 1;
    
    // Append characters like the lexer does
    const char* test_data = "This is a test string that will grow dynamically using memory_realloc";
    for (size_t i = 0; test_data[i]; i++) {
        if (len + 1 >= capacity) {
            capacity *= 2;
            str = memory_realloc(str, capacity);
            assert(str != NULL);
        }
        str[len++] = test_data[i];
        str[len] = '\0';
    }
    
    assert(strcmp(str, test_data) == 0);
    printf("✓ String building test passed\n");
    
    memory_free(str);
    printf("=== memory_realloc: ALL TESTS PASSED ===\n");
    return 0;
}