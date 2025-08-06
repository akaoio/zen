#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Just copy minimal definitions to test
typedef struct {
    int type;
    char* value;
} token_T;

typedef struct {
    char* contents;
    size_t i;
    char c;
} lexer_T;

void* memory_alloc(size_t size) { return calloc(1, size); }
void* memory_realloc(void* ptr, size_t size) { return realloc(ptr, size); }
void memory_free(void* ptr) { free(ptr); }

token_T* init_token(int type, char* value) {
    token_T* token = memory_alloc(sizeof(token_T));
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    return token;
}

char* lexer_get_current_char_as_string(lexer_T* lexer) {
    char* str = memory_alloc(2);
    str[0] = lexer->c;
    str[1] = '\0';
    return str;
}

void lexer_advance(lexer_T* lexer) {
    if (lexer->c \!= '\0' && lexer->i < strlen(lexer->contents)) {
        lexer->i += 1;
        lexer->c = lexer->contents[lexer->i];
    }
}

token_T* lexer_collect_number(lexer_T* lexer) {
    printf("DEBUG: lexer_collect_number started, current char='%c', position=%lu\n", 
           lexer->c, lexer->i);
    
    char* value = memory_alloc(1);
    value[0] = '\0';
    int has_dot = 0;

    // Collect digits and at most one decimal point
    while (isdigit(lexer->c) || (lexer->c == '.' && \!has_dot)) {
        printf("DEBUG: In number loop, char='%c', current value='%s'\n", lexer->c, value);
        
        if (lexer->c == '.') {
            // Check if next character is a digit (to avoid treating "42.foo" as a number)
            if (lexer->i + 1 >= strlen(lexer->contents) || \!isdigit(lexer->contents[lexer->i + 1])) {
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
        printf("DEBUG: Found 'e/E', checking scientific notation...\n");
        // Check if this is actually scientific notation (followed by digit or +/-)
        if (lexer->i + 1 < strlen(lexer->contents)) {
            char next_char = lexer->contents[lexer->i + 1];
            printf("DEBUG: Next char after e/E: '%c'\n", next_char);
            if (isdigit(next_char) || next_char == '+' || next_char == '-') {
                printf("DEBUG: Valid scientific notation, processing...\n");
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

    printf("DEBUG: lexer_collect_number collected: '%s'\n", value);
    return init_token(3, value);  // TOKEN_NUMBER = 3
}

lexer_T* init_lexer(char* contents) {
    lexer_T* lexer = memory_alloc(sizeof(lexer_T));
    lexer->contents = contents;
    lexer->i = 0;
    lexer->c = contents[lexer->i];
    return lexer;
}

int main() {
    printf("=== Scientific notation test ===\n");
    char* input = "1e5";
    printf("Input: %s\n", input);
    
    lexer_T* lexer = init_lexer(input);
    
    token_T* token = lexer_collect_number(lexer);
    printf("Result: type=%d, value='%s' (expected: '1e5')\n", token->type, token->value);
    
    return 0;
}
EOF < /dev/null
