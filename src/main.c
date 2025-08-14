#include "zen/core/lexer.h"
#include "zen/core/logger.h"
#include "zen/core/memory.h"
#include "zen/core/parser.h"
#include "zen/core/runtime_value.h"
#include "zen/core/scope.h"
#include "zen/core/visitor.h"
#include "zen/stdlib/io.h"
#include "zen/stdlib/database.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_SIZE 1024

static bool repl_running = true;

/**
 * @brief Print help information
 * @param void Function takes no parameters
 */
void print_help()
{
    printf("ZEN Language Interpreter v0.0.1\n");
    printf("Usage:\n");
    printf("  zen                    - Start interactive REPL\n");
    printf("  zen <filename>         - Execute ZEN file\n");
    printf("  zen --help             - Show this help\n");
    printf("  zen --debug            - Enable debug logging\n");
    printf("  zen --verbose          - Enable verbose logging (INFO level)\n");
    printf("  zen --silent           - Disable all logging\n");
    printf("  zen --log-file <file>  - Log to file instead of stdout/stderr\n");
    printf("\nSupported file extensions: .zen, .zn\n");
    printf("\nLogging categories (use ZEN_LOG_CATEGORIES env var):\n");
    printf("  GENERAL, LEXER, PARSER, AST, VISITOR, MEMORY, VALUES, STDLIB, ALL\n");
    exit(0);
}

/**
 * @brief Execute a single line of ZEN code
 * @param line The line to execute
 * @param global_scope Global scope for variables
 * @return true if execution should continue, false to exit
 */
static bool execute_line(const char *line, scope_T *global_scope)
{
    if (!line || strlen(line) == 0) {
        return true;  // Empty line, continue
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
    lexer_T *lexer = lexer_new((char *)line);
    if (!lexer) {
        printf("Error: Failed to create lexer\n");
        return true;
    }

    parser_T *parser = parser_new(lexer);
    if (!parser) {
        printf("Error: Failed to create parser\n");
        lexer_free(lexer);
        return true;
    }

    // Use the global scope - free the original parser scope first to prevent leak
    if (parser->scope) {
        scope_free(parser->scope);
    }
    parser->scope = global_scope;

    AST_T *root = parser_parse_statements(parser, global_scope);
    if (!root) {
        if (parser_has_errors(parser)) {
            printf("Parse Error: Input contains %zu syntax errors\n",
                   parser_get_error_count(parser));
        } else {
            printf("Parse Error: Invalid syntax or empty input\n");
        }
        parser_free(parser);
        lexer_free(lexer);
        return true;
    }

    visitor_T *visitor = visitor_new();
    if (!visitor) {
        printf("Error: Failed to create visitor\n");
        ast_free(root);
        parser_free(parser);
        lexer_free(lexer);
        return true;
    }

    // Set the visitor's current scope to the global scope
    // This is critical for function lookup during recursive calls
    visitor->current_scope = global_scope;

    // Execute the parsed code
    RuntimeValue *result = visitor_visit(visitor, root);

    // In REPL mode, display results intelligently
    // Check if the last statement was a print function call
    bool was_print_call = false;
    if (root && root->type == AST_COMPOUND && root->compound_size > 0) {
        AST_T *last_stmt = root->compound_statements[root->compound_size - 1];
        if (last_stmt && last_stmt->type == AST_FUNCTION_CALL && last_stmt->function_call_name &&
            strcmp(last_stmt->function_call_name, "print") == 0) {
            was_print_call = true;
        }
    }

    // Only display the result if it's meaningful
    if (result) {
        // Special handling for control flow markers
        if (result->type == RV_STRING) {
            const char *str_val = rv_get_string(result);
            if (str_val &&
                (strcmp(str_val, "__BREAK__") == 0 || strcmp(str_val, "__CONTINUE__") == 0)) {
                // Don't print control flow markers
            } else {
                char *str = rv_to_string(result);
                printf("%s\n", str);
                memory_free(str);
            }
        } else if (result->type == RV_OBJECT) {
            // Check for internal markers like __RETURN__
            RuntimeValue *return_marker = rv_object_get(result, "__RETURN__");
            if (return_marker) {
                // This is a return marker, display the return value
                char *str = rv_to_string(return_marker);
                printf("%s\n", str);
                memory_free(str);
            } else {
                // Normal object, display it
                char *str = rv_to_string(result);
                printf("%s\n", str);
                memory_free(str);
            }
        } else if (result->type == RV_NULL && was_print_call) {
            // Don't display null returned by print function
        } else {
            // All other types including RV_NULL (when not from print), RV_FUNCTION, RV_ERROR
            char *str = rv_to_string(result);
            printf("%s\n", str);
            memory_free(str);
        }
    }

    // Clean up RuntimeValue
    if (result) {
        rv_unref(result);
    }

    // CRITICAL: Free all resources to prevent memory leaks
    // CRITICAL MEMORY LEAK FIX: Free visitor result AST nodes
    // Note: Do not free visitor result as it may share nodes with root AST
    // The root AST cleanup will handle all shared nodes properly
    visitor_free(visitor);
    // PROPER SOLUTION: Now that variables are stored as RuntimeValues in scope,
    // we can safely free the AST after each REPL command
    ast_free(root);  // Safe now - variables are stored separately from AST!

    // CRITICAL: Prevent double-free of global_scope by clearing parser's scope reference
    // The global_scope will be freed by main() at program exit
    parser->scope = NULL;
    parser_free(parser);
    lexer_free(lexer);

    return true;
}

/**
 * @brief Main entry point
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char *argv[])
{
    // Initialize logging system first
    logger_init();

    // Process command line arguments
    int file_arg_start = 1;
    const char *log_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_help();
        } else if (strcmp(argv[i], "--debug") == 0) {
            logger_set_level(LOG_LEVEL_DEBUG);
            file_arg_start = i + 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            logger_set_level(LOG_LEVEL_INFO);
            file_arg_start = i + 1;
        } else if (strcmp(argv[i], "--silent") == 0) {
            logger_set_level(LOG_LEVEL_SILENT);
            file_arg_start = i + 1;
        } else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            log_file = argv[i + 1];
            logger_set_file(log_file);
            file_arg_start = i + 2;
            i++;  // Skip the filename argument
        } else {
            break;  // Found a non-flag argument (likely a file)
        }
    }

    // Enable memory debugging for leak detection
    memory_debug_enable(true);

    // Create global scope for persistent variables
    scope_T *global_scope = scope_new();
    if (!global_scope) {
        fprintf(stderr, "Error: Failed to create global scope\n");
        return 1;
    }

    if (file_arg_start < argc) {
        // File execution mode
        for (int i = file_arg_start; i < argc; i++) {
            int len = strlen(argv[i]);
            if ((len >= 4 && strcmp(&argv[i][len - 4], ".zen") == 0) ||
                (len >= 3 && strcmp(&argv[i][len - 3], ".zn") == 0)) {
                char *file_contents = io_read_file_internal(argv[i]);
                if (!file_contents) {
                    fprintf(stderr, "Error: Could not read file '%s'\n", argv[i]);
                    return 1;
                }

                lexer_T *lexer = lexer_new(file_contents);
                if (!lexer) {
                    fprintf(stderr, "Error: Failed to create lexer for file '%s'\n", argv[i]);
                    return 1;
                }

                parser_T *parser = parser_new(lexer);
                if (!parser) {
                    fprintf(stderr, "Error: Failed to create parser for file '%s'\n", argv[i]);
                    return 1;
                }

                AST_T *root = parser_parse_statements(parser, global_scope);
                if (!root) {
                    if (parser_has_errors(parser)) {
                        fprintf(stderr,
                                "Parse Error in '%s': %zu syntax errors found\n",
                                argv[i],
                                parser_get_error_count(parser));
                    } else {
                        fprintf(
                            stderr, "Parse Error in '%s': Invalid syntax or empty file\n", argv[i]);
                    }
                    parser_free(parser);
                    lexer_free(lexer);
                    memory_free(file_contents);
                    return 1;
                }

                visitor_T *visitor = visitor_new();
                if (!visitor) {
                    fprintf(stderr, "Error: Failed to create visitor for file '%s'\n", argv[i]);
                    return 1;
                }

                // Set the visitor's current scope to the global scope
                // This is critical for function lookup during recursive calls
                visitor->current_scope = global_scope;

                // Execute the parsed AST

                // Execute the parsed AST - this performs all side effects (print, variable
                // assignments, etc.)
                RuntimeValue *result = visitor_visit(visitor, root);

                // In file/script mode, we do NOT print expression results
                // Only explicit print statements should produce output
                // This is different from interactive REPL mode where expressions are displayed

                // Clean up RuntimeValue
                if (result) {
                    rv_unref(result);
                }

                // CRITICAL FIX: Do NOT free visitor result!
                // The visitor result is typically part of the AST tree and will be freed by
                // ast_free(root) Freeing it separately causes double-free crashes Only the root AST
                // should be freed - it will recursively free all its children

                visitor_free(visitor);
                ast_free(root);  // This will free the entire parse tree
                parser_free(parser);
                lexer_free(lexer);
                memory_free(file_contents);

            } else {
                fprintf(stderr, "Error: File '%s' must have .zen or .zn extension\n", argv[i]);
                return 1;
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

    // CRITICAL: Clear database file cache
    database_clear_cache();

    // CRITICAL: Cleanup logging system
    logger_cleanup();

    // CRITICAL: Cleanup memory debugging system to prevent false leak reports
    memory_debug_cleanup();

    return 0;
}
