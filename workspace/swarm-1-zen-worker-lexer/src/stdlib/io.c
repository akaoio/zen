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
char* zen_read_file(const char* filepath)
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
void zen_print(const Value* value) {
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
void zen_print_no_newline(const Value* value) {
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
char* zen_input(void) {
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
char* zen_input_prompt(const char* prompt) {
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    return zen_input();
}

/**
 * @brief Write string to file
 * @param filepath Path to file to write
 * @param content Content to write
 * @return true on success, false on failure
 */
bool zen_write_file(const char* filepath, const char* content) {
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
bool zen_append_file(const char* filepath, const char* content) {
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
bool zen_file_exists(const char* filepath) {
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
Value* zen_load_json_file(const char* filepath) {
    if (!filepath || !zen_file_exists(filepath)) {
        return NULL;
    }
    
    char* content = zen_read_file(filepath);
    if (!content) {
        return NULL;
    }
    
    Value* result = json_parse(content);
    free(content);
    
    return result;
}

/**
 * @brief Load YAML file as Value object (using JSON parser for basic compatibility)
 * @param filepath Path to YAML file
 * @return Value object representing the YAML data, or NULL on error
 */
Value* zen_load_yaml_file(const char* filepath) {
    // For now, we'll implement basic YAML support using our JSON parser
    // This is a simplified implementation that handles basic YAML syntax
    if (!filepath || !zen_file_exists(filepath)) {
        return NULL;
    }
    
    char* content = zen_read_file(filepath);
    if (!content) {
        return NULL;
    }
    
    // Basic YAML to JSON conversion (simplified)
    // This handles basic key-value pairs and converts them to JSON format
    size_t content_len = strlen(content);
    char* json_content = calloc(content_len * 2, sizeof(char)); // Extra space for conversion
    if (!json_content) {
        free(content);
        return NULL;
    }
    
    // Simple YAML to JSON conversion
    strcat(json_content, "{");
    
    char* line = strtok(content, "\n");
    bool first_item = true;
    
    while (line) {
        // Skip comments and empty lines
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (*trimmed != '#' && *trimmed != '\0') {
            char* colon = strchr(trimmed, ':');
            if (colon) {
                if (!first_item) {
                    strcat(json_content, ",");
                }
                first_item = false;
                
                *colon = '\0';
                char* key = trimmed;
                char* value = colon + 1;
                
                // Trim value
                while (*value == ' ' || *value == '\t') value++;
                
                // Add key-value pair in JSON format
                strcat(json_content, "\"");
                strcat(json_content, key);
                strcat(json_content, "\":\"");
                strcat(json_content, value);
                strcat(json_content, "\"");
            }
        }
        line = strtok(NULL, "\n");
    }
    
    strcat(json_content, "}");
    
    Value* result = json_parse(json_content);
    free(content);
    free(json_content);
    
    return result;
}

/**
 * @brief Resolve module path with extensions
 * @param module_path Base path for the module
 * @return Newly allocated string with resolved path, or NULL if not found
 */
char* zen_resolve_module_path(const char* module_path) {
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
        
        if (zen_file_exists(full_path)) {
            return full_path;
        }
        
        free(full_path);
    }
    
    // Check if the path exists as-is
    if (zen_file_exists(module_path)) {
        return strdup(module_path);
    }
    
    return NULL;
}
