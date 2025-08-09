#include <stdio.h>
#include "zen/core/lexer.h"
#include "zen/core/token.h"

int main(void) {
    const char *code = 
        "print \"Before\"\n"
        "class Foo\n"
        "    function bar\n"
        "        print \"bar\"\n"
        "    function baz\n"
        "        print \"baz\"\n"
        "print \"After\"\n";
    
    lexer_T *lexer = lexer_new((char *)code);
    token_T *token;
    int count = 0;
    
    while ((token = lexer_get_next_token(lexer)) && token->type != TOKEN_EOF) {
        printf("%3d: type=%2d value='%s'\n", ++count, token->type, 
               token->value ? token->value : (token->type == TOKEN_NEWLINE ? "NEWLINE" : 
                                              token->type == TOKEN_INDENT ? "INDENT" :
                                              token->type == TOKEN_DEDENT ? "DEDENT" : ""));
        token_free(token);
    }
    
    if (token) token_free(token);
    lexer_free(lexer);
    return 0;
}