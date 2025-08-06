#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Copy the exact lexer_keyword_type function 
int lexer_keyword_type(const char* value) {
    printf("DEBUG: lexer_keyword_type called with value='%s'\n", value);
    
    // Literals
    if (strcmp(value, "true") == 0) return 17; // TOKEN_TRUE;
    if (strcmp(value, "false") == 0) return 18; // TOKEN_FALSE;
    if (strcmp(value, "null") == 0) return 19; // TOKEN_NULL;
    
    // Control flow keywords
    if (strcmp(value, "set") == 0) { 
        printf("DEBUG: Found 'set' keyword, returning TOKEN_SET\n");
        return 21; // TOKEN_SET;
    }
    if (strcmp(value, "function") == 0) return 22; // TOKEN_FUNCTION;
    if (strcmp(value, "return") == 0) return 23; // TOKEN_RETURN;
    if (strcmp(value, "if") == 0) return 24; // TOKEN_IF;
    if (strcmp(value, "elif") == 0) return 25; // TOKEN_ELIF;
    if (strcmp(value, "else") == 0) return 26; // TOKEN_ELSE;
    if (strcmp(value, "then") == 0) return 27; // TOKEN_THEN;
    if (strcmp(value, "while") == 0) return 28; // TOKEN_WHILE;
    if (strcmp(value, "for") == 0) return 29; // TOKEN_FOR;
    if (strcmp(value, "in") == 0) return 30; // TOKEN_IN;
    if (strcmp(value, "break") == 0) return 31; // TOKEN_BREAK;
    if (strcmp(value, "continue") == 0) return 32; // TOKEN_CONTINUE;
    
    printf("DEBUG: No keyword match, returning TOKEN_ID\n");
    // Not a keyword, return as identifier
    return 4; // TOKEN_ID;
}

int main() {
    printf("Testing keyword recognition:\n");
    
    char* test_words[] = {"set", "function", "if", "variable_name", NULL};
    
    for (int i = 0; test_words[i] != NULL; i++) {
        int result = lexer_keyword_type(test_words[i]);
        printf("Word: '%s' -> Token type: %d\n", test_words[i], result);
    }
    
    return 0;
}