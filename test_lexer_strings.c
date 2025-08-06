#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Minimal token structure for testing
typedef struct TOKEN_STRUCT {
    enum {
        TOKEN_STRING = 2
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

void* memory_realloc(void* ptr, size_t size) { 
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "Memory reallocation failed\n");
        exit(1);
    }
    return new_ptr;
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

// Advance lexer position
void lexer_advance(lexer_T* lexer) {
    if (lexer->c != '\0' && lexer->i < strlen(lexer->contents)) {
        lexer->i += 1;
        lexer->c = lexer->contents[lexer->i];
    }
}

// Fixed string collection function (copied from current implementation)
token_T* lexer_collect_string(lexer_T* lexer) {
    lexer_advance(lexer); // Skip opening quote

    char* value = memory_alloc(1);
    value[0] = '\0';
    size_t value_len = 0;

    while (lexer->c != '"' && lexer->c != '\0') {
        char ch_to_add;
        
        if (lexer->c == '\\' && lexer->i + 1 < strlen(lexer->contents)) {
            // Handle escape sequences
            lexer_advance(lexer); // Skip backslash
            
            switch (lexer->c) {
                case 'n': ch_to_add = '\n'; break;
                case 't': ch_to_add = '\t'; break;
                case 'r': ch_to_add = '\r'; break;
                case '\\': ch_to_add = '\\'; break;
                case '"': ch_to_add = '"'; break;
                case '0': ch_to_add = '\0'; break;
                default: 
                    // For unrecognized escapes, include the backslash and character
                    value = memory_realloc(value, value_len + 3);
                    value[value_len] = '\\';
                    value[value_len + 1] = lexer->c;
                    value[value_len + 2] = '\0';
                    value_len += 2;
                    lexer_advance(lexer);
                    continue;
            }
        } else {
            ch_to_add = lexer->c;
        }
        
        // Add character to value
        value = memory_realloc(value, value_len + 2);
        value[value_len] = ch_to_add;
        value[value_len + 1] = '\0';
        value_len++;
        
        lexer_advance(lexer);
    }

    if (lexer->c == '"') {
        lexer_advance(lexer); // Skip closing quote
    }

    return init_token(TOKEN_STRING, value);
}

// Initialize lexer
lexer_T* init_lexer(char* contents) {
    lexer_T* lexer = memory_alloc(sizeof(lexer_T));
    lexer->contents = contents;
    lexer->i = 0;
    lexer->c = contents[lexer->i];
    return lexer;
}

void print_string_representation(const char* str) {
    printf("\"");
    for (int i = 0; str[i] != '\0'; i++) {
        switch (str[i]) {
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            case '\r': printf("\\r"); break;
            case '\\': printf("\\\\"); break;
            case '"': printf("\\\""); break;
            case '\0': printf("\\0"); break;
            default: printf("%c", str[i]); break;
        }
    }
    printf("\"");
}

// Test string escape sequences
void test_string_escapes() {
    printf("=== String escape sequence tests ===\n");
    
    struct {
        char* input;
        char* expected_literal;
        char* description;
    } tests[] = {
        {"\"hello\"", "hello", "simple string"},
        {"\"\\n\"", "\n", "newline escape"},
        {"\"\\t\"", "\t", "tab escape"},
        {"\"\\r\"", "\r", "carriage return escape"},
        {"\"\\\\\"", "\\", "backslash escape"},
        {"\"\\\"\"", "\"", "quote escape"},
        {"\"\\n\\t\\r\\\\\\\"\"", "\n\t\r\\\"", "combined escapes"},
        {NULL, NULL, NULL}
    };
    
    for (int i = 0; tests[i].input != NULL; i++) {
        printf("Testing: %s (%s)\n", tests[i].input, tests[i].description);
        lexer_T* lexer = init_lexer(tests[i].input);
        
        token_T* token = lexer_collect_string(lexer);
        
        printf("  Expected: ");
        print_string_representation(tests[i].expected_literal);
        printf("\n  Got: ");
        print_string_representation(token->value);
        printf("\n");
        
        if (strcmp(token->value, tests[i].expected_literal) == 0) {
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

int main() {
    printf("ZEN Lexer String Tests\n");
    printf("=====================\n\n");
    
    test_string_escapes();
    
    return 0;
}