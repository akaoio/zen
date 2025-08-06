#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Mock basic functions we need
void* memory_alloc(size_t size) {
    void* ptr = calloc(1, size);
    return ptr;
}

void* memory_realloc(void* ptr, size_t new_size) {
    return realloc(ptr, new_size);
}

void memory_free(void* ptr) {
    free(ptr);
}

char* lexer_get_current_char_as_string_test(char c) {
    char* str = memory_alloc(2);
    str[0] = c;
    str[1] = '\0';
    return str;
}

// Test the memory realloc pattern used in lexer
int main() {
    printf("Testing memory realloc pattern...\n");
    
    char* value = memory_alloc(1);
    value[0] = '\0';
    
    // Simulate adding '1'
    char* s = lexer_get_current_char_as_string_test('1');
    printf("Adding '1', current value: '%s', s: '%s'\n", value, s);
    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
    strcat(value, s);
    printf("After adding '1', value: '%s'\n", value);
    memory_free(s);
    
    // Simulate adding 'e'
    s = lexer_get_current_char_as_string_test('e');
    printf("Adding 'e', current value: '%s', s: '%s'\n", value, s);
    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
    strcat(value, s);
    printf("After adding 'e', value: '%s'\n", value);
    memory_free(s);
    
    // Simulate adding '5'
    s = lexer_get_current_char_as_string_test('5');
    printf("Adding '5', current value: '%s', s: '%s'\n", value, s);
    value = memory_realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
    strcat(value, s);
    printf("After adding '5', value: '%s'\n", value);
    memory_free(s);
    
    printf("Final value: '%s' (length: %zu)\n", value, strlen(value));
    memory_free(value);
    
    return 0;
}
EOF < /dev/null
