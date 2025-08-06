#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Minimal token structure for testing
typedef struct TOKEN_STRUCT {
    enum {
        TOKEN_NUMBER = 3,
        TOKEN_EOF = 5
    } type;
    char* value;
} token_T;

// Minimal lexer structure for testing
typedef struct LEXER_STRUCT {
    char* contents;
    size_t i;
    char c;
} lexer_T;

// Simple memory functions
void* memory_alloc(size_t size) { 
    void* ptr = calloc(1, size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

void memory_free(void* ptr) { 
    if (ptr) free(ptr); 
}

// Token creation
token_T* init_token(int type, char* value) {
    token_T* token = memory_alloc(sizeof(token_T));
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    return token;
}

// Helper function to get current char as string
char* lexer_get_current_char_as_string(lexer_T* lexer) {
    char* str = memory_alloc(2);
    str[0] = lexer->c;
    str[1] = '\0';
    return str;
}

// Advance lexer position
void lexer_advance(lexer_T* lexer) {
    if (lexer->c != '\0' && lexer->i < strlen(lexer->contents)) {
        lexer->i += 1;
        lexer->c = lexer->contents[lexer->i];
    }
}

// Fixed number collection function
token_T* lexer_collect_number(lexer_T* lexer) {
    char* value = memory_alloc(1);
    value[0] = '\0';
    int has_dot = 0;

    // Collect digits and at most one decimal point
    while (isdigit(lexer->c) || (lexer->c == '.' && !has_dot)) {
        if (lexer->c == '.') {
            // Check if next character is a digit (to avoid treating "42.foo" as a number)
            if (lexer->i + 1 >= strlen(lexer->contents) || !isdigit(lexer->contents[lexer->i + 1])) {
                break;
            }
            has_dot = 1;
        }
        
        char* s = lexer_get_current_char_as_string(lexer);
        
        // Safe string concatenation
        size_t old_len = strlen(value);
        size_t new_len = old_len + strlen(s) + 1;
        char* new_value = memory_alloc(new_len);
        strcpy(new_value, value);
        strcat(new_value, s);
        memory_free(value);
        value = new_value;
        
        memory_free(s);
        lexer_advance(lexer);
    }

    // Handle scientific notation (e/E)
    if (lexer->c == 'e' || lexer->c == 'E') {
        // Check if this is actually scientific notation (followed by digit or +/-)
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                char* s = lexer_get_current_char_as_string(lexer);
                
                // Safe string concatenation
                size_t old_len = strlen(value);
                size_t new_len = old_len + strlen(s) + 1;
                char* new_value = memory_alloc(new_len);
                strcpy(new_value, value);
                strcat(new_value, s);
                memory_free(value);
                value = new_value;
                
                memory_free(s);
                lexer_advance(lexer);
                
                // Handle optional +/- after e/E
                if (lexer->c == '+' || lexer->c == '-') {
                    s = lexer_get_current_char_as_string(lexer);
                    
                    // Safe string concatenation
                    size_t old_len = strlen(value);
                    size_t new_len = old_len + strlen(s) + 1;
                    char* new_value = memory_alloc(new_len);
                    strcpy(new_value, value);
                    strcat(new_value, s);
                    memory_free(value);
                    value = new_value;
                    
                    memory_free(s);
                    lexer_advance(lexer);
                }
                
                // Collect exponent digits
                while (isdigit(lexer->c)) {
                    s = lexer_get_current_char_as_string(lexer);
                    
                    // Safe string concatenation
                    size_t old_len = strlen(value);
                    size_t new_len = old_len + strlen(s) + 1;
                    char* new_value = memory_alloc(new_len);
                    strcpy(new_value, value);
                    strcat(new_value, s);
                    memory_free(value);
                    value = new_value;
                    
                    memory_free(s);
                    lexer_advance(lexer);
                }
            }
        }
    }

    return init_token(TOKEN_NUMBER, value);
}

// Initialize lexer
lexer_T* init_lexer(char* contents) {
    lexer_T* lexer = memory_alloc(sizeof(lexer_T));
    lexer->contents = contents;
    lexer->i = 0;
    lexer->c = contents[lexer->i];
    return lexer;
}

// Test scientific notation parsing
void test_scientific_notation() {
    printf("=== Scientific notation tests ===\n");
    
    struct {
        char* input;
        char* expected;
    } tests[] = {
        {"1e5", "1e5"},
        {"2.5e-3", "2.5e-3"},
        {"1.23E+10", "1.23E+10"},
        {"5e0", "5e0"},
        {"42", "42"},
        {"3.14", "3.14"},
        {NULL, NULL}
    };
    
    for (int i = 0; tests[i].input != NULL; i++) {
        printf("Testing: %s\n", tests[i].input);
        lexer_T* lexer = init_lexer(tests[i].input);
        
        token_T* token = lexer_collect_number(lexer);
        
        printf("  Expected: %s\n", tests[i].expected);
        printf("  Got: %s\n", token->value);
        
        if (strcmp(token->value, tests[i].expected) == 0) {
            printf("  ✓ PASS\n");
        } else {
            printf("  ✗ FAIL\n");
        }
        
        memory_free(token->value);
        memory_free(token);
        memory_free(lexer);
        printf("\n");
    }
}

// Test escape sequences
void test_string_escapes() {
    printf("=== String escape sequence tests ===\n");
    
    // Simple escape sequences that should work
    char* expected_escapes = "\n\t\r\\\"";
    printf("Expected escape sequences: newline, tab, carriage return, backslash, quote\n");
    printf("We expect lexer_collect_string to handle: \\n \\t \\r \\\\ \\\"\n");
}

int main() {
    printf("ZEN Lexer Unit Tests\n");
    printf("===================\n\n");
    
    test_scientific_notation();
    test_string_escapes();
    
    return 0;
}