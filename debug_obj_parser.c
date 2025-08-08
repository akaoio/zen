#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"
#include <stdio.h>

int main(void) {
    printf("Testing parser_peek_for_object_literal...\n");
    
    char* input = "name \"Alice\", age 30";
    lexer_T* lexer = lexer_new(input);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    
    printf("Created parser for input: %s\n", input);
    
    // Advance to first token
    parser->current_token = lexer_get_next_token(parser->lexer);
    
    printf("Current token: type=%d, value='%s'\n", 
           parser->current_token->type, parser->current_token->value);
    
    int is_object = parser_peek_for_object_literal(parser);
    printf("Object literal detection result: %d\n", is_object);
    
    printf("Testing completed successfully\n");
    
    parser_free(parser);
    scope_free(scope);
    return 0;
}