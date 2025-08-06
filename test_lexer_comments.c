#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Minimal token structure for testing
typedef struct TOKEN_STRUCT {
    enum {
        TOKEN_ID = 0,
        TOKEN_EQUALS = 1,
        TOKEN_STRING = 2,
        TOKEN_NUMBER = 3,
        TOKEN_SET = 4,
        TOKEN_EOF = 5,
        TOKEN_NEWLINE = 8
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

// Advance lexer position
void lexer_advance(lexer_T* lexer) {
    if (lexer->c != '\0' && lexer->i < strlen(lexer->contents)) {
        lexer->i += 1;
        lexer->c = lexer->contents[lexer->i];
    }
}

// Simplified identifier collection
token_T* lexer_collect_id(lexer_T* lexer) {
    char* value = memory_alloc(100);  // Simple fixed buffer for test
    int pos = 0;
    
    while (isalnum(lexer->c) || lexer->c == '_') {
        value[pos++] = lexer->c;
        lexer_advance(lexer);
    }
    value[pos] = '\0';
    
    // Check if it's "set" keyword
    if (strcmp(value, "set") == 0) {
        return init_token(TOKEN_SET, value);
    }
    return init_token(TOKEN_ID, value);
}

// Simplified number collection
token_T* lexer_collect_number(lexer_T* lexer) {
    char* value = memory_alloc(100);  // Simple fixed buffer for test
    int pos = 0;
    
    while (isdigit(lexer->c)) {
        value[pos++] = lexer->c;
        lexer_advance(lexer);
    }
    value[pos] = '\0';
    
    return init_token(TOKEN_NUMBER, value);
}

// Simplified tokenizer that handles comments
token_T* lexer_get_next_token(lexer_T* lexer) {
    while (lexer->c != '\0' && lexer->i < strlen(lexer->contents)) {
        // Skip whitespace
        if (lexer->c == ' ' || lexer->c == '\t') {
            lexer_advance(lexer);
            continue;
        }
        
        // Handle newlines
        if (lexer->c == '\n') {
            lexer_advance(lexer);
            return init_token(TOKEN_NEWLINE, "\n");
        }
        
        // Handle identifiers and keywords
        if (isalpha(lexer->c) || lexer->c == '_') {
            return lexer_collect_id(lexer);
        }
        
        // Handle numbers
        if (isdigit(lexer->c)) {
            return lexer_collect_number(lexer);
        }
        
        // Single-line comments (//)
        if (lexer->c == '/' && lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '/') {
            // Skip the rest of the line
            while (lexer->c != '\n' && lexer->c != '\0') {
                lexer_advance(lexer);
            }
            continue;
        }
        
        // Multi-line comments (/* */)
        if (lexer->c == '/' && lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '*') {
            lexer_advance(lexer); // Skip '/'
            lexer_advance(lexer); // Skip '*'
            
            // Look for closing */
            while (lexer->c != '\0') {
                if (lexer->c == '*' && lexer->i + 1 < strlen(lexer->contents) && lexer->contents[lexer->i + 1] == '/') {
                    lexer_advance(lexer); // Skip '*'
                    lexer_advance(lexer); // Skip '/'
                    break;
                }
                lexer_advance(lexer);
            }
            continue;
        }
        
        // Handle other operators
        if (lexer->c == '=') {
            lexer_advance(lexer);
            return init_token(TOKEN_EQUALS, "=");
        }
        
        // Skip unknown characters for this test
        lexer_advance(lexer);
    }
    
    return init_token(TOKEN_EOF, "\0");
}

// Initialize lexer
lexer_T* init_lexer(char* contents) {
    lexer_T* lexer = memory_alloc(sizeof(lexer_T));
    lexer->contents = contents;
    lexer->i = 0;
    lexer->c = contents[lexer->i];
    return lexer;
}

const char* token_type_name(int type) {
    switch (type) {
        case TOKEN_ID: return "ID";
        case TOKEN_EQUALS: return "EQUALS";
        case TOKEN_STRING: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_SET: return "SET";
        case TOKEN_EOF: return "EOF";
        case TOKEN_NEWLINE: return "NEWLINE";
        default: return "UNKNOWN";
    }
}

// Test comment handling
void test_comments() {
    printf("=== Comment handling tests ===\n");
    
    char* input = 
        "set x 42  // This is a comment\n"
        "/* Multi-line\n"
        "   comment */\n"
        "set y 13";
    
    printf("Input:\n%s\n\n", input);
    
    lexer_T* lexer = init_lexer(input);
    token_T* token;
    
    printf("Expected tokens: SET x NUMBER NEWLINE SET y NUMBER EOF\n");
    printf("Actual tokens: ");
    
    while ((token = lexer_get_next_token(lexer))->type != TOKEN_EOF) {
        if (token->type != TOKEN_NEWLINE) {  // Skip newlines for cleaner output
            printf("%s(%s) ", token_type_name(token->type), token->value);
        }
        memory_free(token->value);
        memory_free(token);
    }
    printf("%s\n\n", token_type_name(token->type));
    
    memory_free(token);
    memory_free(lexer);
}

int main() {
    printf("ZEN Lexer Comment Tests\n");
    printf("======================\n\n");
    
    test_comments();
    
    return 0;
}