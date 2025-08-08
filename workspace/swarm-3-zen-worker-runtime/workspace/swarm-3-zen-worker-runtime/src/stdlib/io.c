#define _GNU_SOURCE  // For getline and ssize_t
#include "zen/stdlib/io.h"
#include "zen/stdlib/json.h"
#include "zen/types/value.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/**
 * @brief Read entire file to string
 * @param filepath Path to the file to read
 * @return Newly allocated string containing file contents, or NULL on error
 */
char* io_read_file_internal(const char* filepath)
{
    if (!filepath) {
        return NULL;
    }
    
    char* buffer = NULL;
    long length;

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        return NULL; // Don't exit, return NULL for error
    }
    
    // Get file size
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    
    length = ftell(f);
    if (length < 0) {
        fclose(f);
        return NULL;
    }
    
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    // Allocate buffer
    buffer = calloc(length + 1, sizeof(char));
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    // Read file contents
    size_t bytes_read = fread(buffer, 1, length, f);
    fclose(f);
    
    if (bytes_read != (size_t)length) {
        free(buffer);
        return NULL;
    }
    
    buffer[length] = '\0'; // Ensure null termination
    return buffer;
}

/**
 * @brief Print Value to stdout with newline
 * @param value Value to print
 */
void io_print_internal(const Value* value) {
    if (!value) {
        printf("null\n");
        return;
    }
    
    char* str = value_to_string(value);
    if (str) {
        printf("%s\n", str);
        free(str);
    } else {
        printf("null\n");
    }
}

/**
 * @brief Print Value to stdout without newline
 * @param value Value to print
 */
void io_print_no_newline_internal(const Value* value) {
    if (!value) {
        printf("null");
        return;
    }
    
    char* str = value_to_string(value);
    if (str) {
        printf("%s", str);
        free(str);
    } else {
        printf("null");
    }
}

/**
 * @brief Read a line from stdin
 * @return Newly allocated string containing user input, or NULL on error
 */
char* io_input_internal(void) {
    char* buffer = NULL;
    size_t buffer_size = 0;
    ssize_t chars_read = getline(&buffer, &buffer_size, stdin);
    
    if (chars_read == -1) {
        if (buffer) {
            free(buffer);
        }
        return NULL;
    }
    
    // Remove trailing newline if present
    if (chars_read > 0 && buffer[chars_read - 1] == '\n') {
        buffer[chars_read - 1] = '\0';
    }
    
    return buffer;
}

/**
 * @brief Read a line from stdin with prompt
 * @param prompt Prompt string to display
 * @return Newly allocated string containing user input, or NULL on error
 */
char* io_input_prompt_internal(const char* prompt) {
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    return io_input_internal();
}

/**
 * @brief Write string to file
 * @param filepath Path to file to write
 * @param content Content to write
 * @return true on success, false on failure
 */
bool io_write_file_internal(const char* filepath, const char* content) {
    if (!filepath || !content) {
        return false;
    }
    
    FILE* f = fopen(filepath, "w");
    if (!f) {
        return false;
    }
    
    size_t content_len = strlen(content);
    size_t written = fwrite(content, 1, content_len, f);
    fclose(f);
    
    return written == content_len;
}

/**
 * @brief Append string to file
 * @param filepath Path to file to append to
 * @param content Content to append
 * @return true on success, false on failure
 */
bool io_append_file_internal(const char* filepath, const char* content) {
    if (!filepath || !content) {
        return false;
    }
    
    FILE* f = fopen(filepath, "a");
    if (!f) {
        return false;
    }
    
    size_t content_len = strlen(content);
    size_t written = fwrite(content, 1, content_len, f);
    fclose(f);
    
    return written == content_len;
}

/**
 * @brief Check if file exists
 * @param filepath Path to check
 * @return true if file exists, false otherwise
 */
bool io_file_exists_internal(const char* filepath) {
    if (!filepath) {
        return false;
    }
    
    FILE* f = fopen(filepath, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/**
 * @brief Load JSON file as Value object
 * @param filepath Path to JSON file
 * @return Value object representing the JSON data, or NULL on error
 */
Value* io_load_json_file_internal(const char* filepath) {
    if (!filepath || !io_file_exists_internal(filepath)) {
        return NULL;
    }
    
    char* content = io_read_file_internal(filepath);
    if (!content) {
        return NULL;
    }
    
    Value* result = json_parse(content);
    free(content);
    
    return result;
}


/**
 * @brief Resolve module path with extensions
 * @param module_path Base path for the module
 * @return Newly allocated string with resolved path, or NULL if not found
 */
char* io_resolve_module_path_internal(const char* module_path) {
    if (!module_path) {
        return NULL;
    }
    
    // Try different extensions in order of preference
    const char* extensions[] = {".zen", ".json", ".yaml", NULL};
    
    for (int i = 0; extensions[i]; i++) {
        size_t path_len = strlen(module_path) + strlen(extensions[i]) + 1;
        char* full_path = malloc(path_len);
        if (!full_path) continue;
        
        strcpy(full_path, module_path);
        strcat(full_path, extensions[i]);
        
        if (io_file_exists_internal(full_path)) {
            return full_path;
        }
        
        free(full_path);
    }
    
    // Check if the path exists as-is
    if (io_file_exists_internal(module_path)) {
        return strdup(module_path);
    }
    
    return NULL;
}

// STDLIB WRAPPER FUNCTIONS - Match MANIFEST.json signatures

/**
 * @brief Print function for stdlib integration
 * @param args Array of Value arguments 
 * @param argc Number of arguments
 * @return Always returns null Value
 */
Value* io_print(Value** args, size_t argc) {
    for (size_t i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        io_print_no_newline_internal(args[i]);
    }
    printf("\n");
    return value_new_null();
}

/**
 * @brief Input function for stdlib integration
 * @param args Array of Value arguments (optional prompt)
 * @param argc Number of arguments
 * @return String Value containing user input
 */
Value* io_input(Value** args, size_t argc) {
    char* input_str = NULL;
    
    if (argc > 0 && args[0] && args[0]->type == VALUE_STRING) {
        input_str = io_input_prompt_internal(args[0]->as.string->data);
    } else {
        input_str = io_input_internal();
    }
    
    if (!input_str) {
        return value_new_string("");
    }
    
    Value* result = value_new_string(input_str);
    free(input_str);
    return result;
}

/**
 * @brief Read file function for stdlib integration
 * @param args Array of Value arguments (filepath)
 * @param argc Number of arguments
 * @return String Value containing file contents or error
 */
Value* io_read_file(Value** args, size_t argc) {
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("readFile requires a string filepath argument");
            error->as.error->code = -1;
        }
        return error;
    }
    
    char* content = io_read_file_internal(args[0]->as.string->data);
    if (!content) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Failed to read file");
            error->as.error->code = -1;
        }
        return error;
    }
    
    Value* result = value_new_string(content);
    free(content);
    return result;
}

/**
 * @brief Write file function for stdlib integration
 * @param args Array of Value arguments (filepath, content)
 * @param argc Number of arguments
 * @return Boolean Value indicating success
 */
Value* io_write_file(Value** args, size_t argc) {
    if (argc < 2 || !args[0] || !args[1] || 
        args[0]->type != VALUE_STRING || args[1]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool success = io_write_file_internal(args[0]->as.string->data, args[1]->as.string->data);
    return value_new_boolean(success);
}

/**
 * @brief Append to file function for stdlib integration
 * @param args Array of Value arguments (filepath, content)
 * @param argc Number of arguments
 * @return Boolean Value indicating success
 */
Value* io_append_file(Value** args, size_t argc) {
    if (argc < 2 || !args[0] || !args[1] || 
        args[0]->type != VALUE_STRING || args[1]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool success = io_append_file_internal(args[0]->as.string->data, args[1]->as.string->data);
    return value_new_boolean(success);
}

/**
 * @brief Check file exists function for stdlib integration
 * @param args Array of Value arguments (filepath)
 * @param argc Number of arguments
 * @return Boolean Value indicating if file exists
 */
Value* io_file_exists(Value** args, size_t argc) {
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    bool exists = io_file_exists_internal(args[0]->as.string->data);
    return value_new_boolean(exists);
}
