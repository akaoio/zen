#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/scope.h"
#include "zen/core/memory.h"
#include "zen/stdlib/io.h"

#define MAX_INPUT_SIZE 1024

static bool repl_running = true;

/**
 * @brief Print help information
 */
void print_help()
{
    printf("ZEN Language Interpreter v0.0.1\n");
    printf("Usage:\n");
    printf("  zen                    - Start interactive REPL\n");
    printf("  zen <filename>         - Execute ZEN file\n");
    printf("  zen --help             - Show this help\n");
    printf("\nSupported file extensions: .zen, .zn\n");
    exit(0);
}

/**
 * @brief Execute a single line of ZEN code
 * @param line The line to execute
 * @param global_scope Global scope for variables
 * @return true if execution should continue, false to exit
 */
static bool execute_line(const char* line, scope_T* global_scope) {
    if (!line || strlen(line) == 0) {
        return true; // Empty line, continue
    }
    
    // Handle REPL commands
    if (strcmp(line, "exit\n") == 0 || strcmp(line, "quit\n") == 0) {
        printf("Goodbye!\n");
        return false;
    }
    
    if (strcmp(line, "help\n") == 0) {
        printf("ZEN REPL Commands:\n");
        printf("  help     - Show this help\n");
        printf("  exit     - Exit REPL\n");
        printf("  quit     - Exit REPL\n");
        printf("  clear    - Clear screen\n");
        return true;
    }
    
    if (strcmp(line, "clear\n") == 0) {
        system("clear");
        return true;
    }
    
    // Parse and execute ZEN code
    lexer_T* lexer = init_lexer((char*)line);
    if (!lexer) {
        printf("Error: Failed to create lexer\n");
        return true;
    }
    
    parser_T* parser = init_parser(lexer);
    if (!parser) {
        printf("Error: Failed to create parser\n");
        return true;
    }
    
    // Use the global scope
    parser->scope = global_scope;
    
    AST_T* root = parser_parse_statements(parser, global_scope);
    if (!root) {
        printf("Error: Failed to parse input\n");
        return true;
    }
    
    visitor_T* visitor = init_visitor();
    if (!visitor) {
        printf("Error: Failed to create visitor\n");
        return true;
    }
    
    // Execute the parsed code
    AST_T* result = visitor_visit(visitor, root);
    
    // Print result if it's not NOOP
    if (result && result->type != AST_NOOP) {
        switch (result->type) {
            case AST_STRING:
                if (result->string_value) {
                    printf("%s\n", result->string_value);
                }
                break;
            case AST_NUMBER:
                printf("%.15g\n", result->number_value);
                break;
            case AST_BOOLEAN:
                printf("%s\n", result->boolean_value ? "true" : "false");
                break;
            case AST_NULL:
                printf("null\n");
                break;
            default:
                // Don't print other types
                break;
        }
    }
    
    return true;
}

/**
 * @brief Main entry point
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char* argv[])
{
    // Handle help flag
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_help();
    }
    
    // Create global scope for persistent variables
    scope_T* global_scope = init_scope();
    if (!global_scope) {
        fprintf(stderr, "Error: Failed to create global scope\n");
        return 1;
    }
    
    if (argc >= 2) {
        // File execution mode
        for (int i = 1; i < argc; i++) {
            int len = strlen(argv[i]);
            if ((len >= 4 && strcmp(&argv[i][len-4], ".zen") == 0) || 
                (len >= 3 && strcmp(&argv[i][len-3], ".zn") == 0)) {
                
                char* file_contents = get_file_contents(argv[i]);
                if (!file_contents) {
                    fprintf(stderr, "Error: Could not read file '%s'\n", argv[i]);
                    return 1;
                }
                
                lexer_T* lexer = init_lexer(file_contents);
                if (!lexer) {
                    fprintf(stderr, "Error: Failed to create lexer for file '%s'\n", argv[i]);
                    return 1;
                }
                
                parser_T* parser = init_parser(lexer);
                if (!parser) {
                    fprintf(stderr, "Error: Failed to create parser for file '%s'\n", argv[i]);
                    return 1;
                }
                
                AST_T* root = parser_parse_statements(parser, global_scope);
                if (!root) {
                    fprintf(stderr, "Error: Failed to parse file '%s'\n", argv[i]);
                    return 1;
                }
                
                visitor_T* visitor = init_visitor();
                if (!visitor) {
                    fprintf(stderr, "Error: Failed to create visitor for file '%s'\n", argv[i]);
                    return 1;
                }
                
                // Execute the parsed AST - this performs all side effects (print, variable assignments, etc.)
                AST_T* result = visitor_visit(visitor, root);
                
                // Handle any meaningful return value from execution
                if (result && result->type != AST_NOOP) {
                    switch (result->type) {
                        case AST_STRING:
                            if (result->string_value) {
                                printf("%s\n", result->string_value);
                            }
                            break;
                        case AST_NUMBER:
                            printf("%.15g\n", result->number_value);
                            break;
                        case AST_BOOLEAN:
                            printf("%s\n", result->boolean_value ? "true" : "false");
                            break;
                        case AST_NULL:
                            printf("null\n");
                            break;
                        default:
                            // Don't print other types like compound results
                            break;
                    }
                }
                
                // CRITICAL: Free all allocated resources to prevent memory leaks
                // CRITICAL DOUBLE-FREE FIX: Do NOT free visitor result 
                // The visitor result can be:
                // 1. A reference to a node in the original parse tree (freed by ast_free(root))
                // 2. A new temporary node created by visitor (should be handled by visitor_free)
                // 3. A shared node stored in scope (freed by scope_free)
                // In all cases, freeing the result separately causes double-free crashes
                
                visitor_free(visitor);
                ast_free(root);  // This will free the entire parse tree
                parser_free(parser);
                lexer_free(lexer);
                free(file_contents);
                
            } else {
                print_help();
            }
        }
    } else {
        // REPL mode
        printf("ZEN Language Interpreter v0.0.1\n");
        printf("Type 'help' for commands, 'exit' to quit.\n\n");
        
        char input[MAX_INPUT_SIZE];
        repl_running = true;
        
        while (repl_running) {
            printf("zen> ");
            fflush(stdout);
            
            if (!fgets(input, MAX_INPUT_SIZE, stdin)) {
                // EOF or error - exit gracefully
                printf("\nGoodbye!\n");
                break;
            }
            
            // Execute the line
            if (!execute_line(input, global_scope)) {
                break;
            }
        }
    }
    
    // CRITICAL: Free global scope before exit
    scope_free(global_scope);
    
    // CRITICAL: Cleanup memory debugging system to prevent false leak reports
    memory_debug_cleanup();
    
    return 0;
}
