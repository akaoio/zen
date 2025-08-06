#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Include the lexer structures directly
typedef struct TOKEN_STRUCT
{
    enum
    {
        TOKEN_ID,
        TOKEN_EQUALS,
        TOKEN_STRING,
        TOKEN_NUMBER,
        TOKEN_RPAREN,
        TOKEN_LPAREN,
        TOKEN_LBRACE,
        TOKEN_RBRACE,
        TOKEN_LSQUARE,
        TOKEN_RSQUARE,
        TOKEN_COMMA,
        TOKEN_DOT,
        TOKEN_SEMICOLON,
        TOKEN_COLON,
        TOKEN_NEWLINE,
        TOKEN_EOF,
        TOKEN_SET,
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_MULTIPLY,
        TOKEN_DIVIDE,
        TOKEN_MODULO,
        TOKEN_LESS_THAN,
        TOKEN_GREATER_THAN,
        TOKEN_LESS_EQUALS,
        TOKEN_GREATER_EQUALS,
        TOKEN_AND,
        TOKEN_OR,
        TOKEN_NOT,
        TOKEN_NOT_EQUALS,
        TOKEN_TRUE,
        TOKEN_FALSE,
        TOKEN_NULL,
        TOKEN_FUNCTION,
        TOKEN_RETURN,
        TOKEN_IF,
        TOKEN_ELIF,
        TOKEN_ELSE,
        TOKEN_WHILE,
        TOKEN_FOR,
        TOKEN_IN,
        TOKEN_BREAK,
        TOKEN_CONTINUE,
        TOKEN_INDENT,
        TOKEN_DEDENT,
        TOKEN_DASH,
        TOKEN_STAR,
        TOKEN_SLASH
    } type;

    char* value;
} token_T;

typedef struct LEXER_STRUCT
{
    char* contents;
    size_t i;
    char c;
    int current_indent;
    int indent_stack[32];
    int indent_level;
    int at_line_start;
} lexer_T;

// Function declarations
lexer_T* init_lexer(const char* contents);
token_T* lexer_get_next_token(lexer_T* lexer);
token_T* init_token(int type, char* value);

// Memory management
void* memory_alloc(size_t size) { return calloc(1, size); }
void* memory_realloc(void* ptr, size_t size) { return realloc(ptr, size); }
void memory_free(void* ptr) { free(ptr); }

int main() {
    printf("Testing scientific notation with '1e5 2.5e-3':\n");
    
    char* input = "1e5 2.5e-3";
    lexer_T* lexer = init_lexer(input);
    
    printf("Input: '%s'\n", input);
    
    // First token should be 1e5
    token_T* token1 = lexer_get_next_token(lexer);
    printf("Token 1: type=%d, value='%s'\n", token1->type, token1->value);
    
    // Skip whitespace
    token_T* token2 = lexer_get_next_token(lexer);
    if (token2->type == TOKEN_ID && strlen(token2->value) == 0) {
        token2 = lexer_get_next_token(lexer); // Get next real token
    }
    printf("Token 2: type=%d, value='%s'\n", token2->type, token2->value);
    
    printf("\nExpected: Token 1='1e5', Token 2='2.5e-3'\n");
    printf("Results:  Token 1='%s', Token 2='%s'\n", token1->value, token2->value);
    
    if (strcmp(token1->value, "1e5") == 0 && strcmp(token2->value, "2.5e-3") == 0) {
        printf("✅ SUCCESS: Scientific notation parsing fixed!\n");
        return 0;
    } else {
        printf("❌ FAILED: Scientific notation still broken\n");
        return 1;
    }
}