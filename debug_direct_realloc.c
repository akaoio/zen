#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Test realloc directly without the memory wrapper
int main() {
    // Simulate the buffer expansion that's failing
    size_t buffer_size = 64;
    char* value = malloc(buffer_size);
    value[0] = '\0';
    size_t value_len = 0;
    
    printf("Testing direct realloc behavior\n");
    printf("Initial: ptr=%p, size=%zu\n", (void*)value, buffer_size);
    
    // Add 50 chars
    for (int i = 0; i < 50; i++) {
        value[value_len++] = 'a' + (i % 26);
        value[value_len] = '\0';
    }
    
    printf("After 50 chars: len=%zu, content='%.10s...'\n", strlen(value), value);
    
    // Now try realloc when we need to expand (at 64 chars)
    for (int i = 50; i < 70; i++) {
        if (value_len + 1 >= buffer_size) {
            printf("EXPANDING at char %d: old_ptr=%p, old_size=%zu\n", 
                   i, (void*)value, buffer_size);
            printf("Content before realloc (len=%zu): '%.20s...'\n", strlen(value), value);
            
            buffer_size *= 2;
            char* new_value = realloc(value, buffer_size);
            
            printf("REALLOC: new_ptr=%p, new_size=%zu, %s\n", 
                   (void*)new_value, buffer_size, new_value ? "SUCCESS" : "FAILED");
            
            if (new_value) {
                value = new_value;
                printf("Content after realloc (len=%zu): '%.20s...'\n", strlen(value), value);
            } else {
                printf("REALLOC FAILED!\n");
                free(value);
                return 1;
            }
        }
        
        value[value_len++] = 'a' + (i % 26);
        value[value_len] = '\0';
    }
    
    printf("Final result: len=%zu, content='%.20s...'\n", strlen(value), value);
    
    free(value);
    return 0;
}