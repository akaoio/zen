#define _GNU_SOURCE  // For getline and ssize_t
#include "zen/stdlib/io.h"

#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/json.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Read entire file to string
 * @param filepath Path to the file to read
 * @return Newly allocated string containing file contents, or NULL on error
 */
char *io_read_file_internal(const char *filepath)
{
    if (!filepath) {
        return NULL;
    }

    char *buffer = NULL;
    long length;

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return NULL;  // Don't exit, return NULL for error
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
    buffer = memory_alloc(length + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    // Initialize to zero like calloc
    memset(buffer, 0, length + 1);

    // Read file contents
    size_t bytes_read = fread(buffer, 1, length, f);
    fclose(f);

    if (bytes_read != (size_t)length) {
        memory_free(buffer);
        return NULL;
    }

    buffer[length] = '\0';  // Ensure null termination
    return buffer;
}

/**
 * @brief Print RuntimeValue to stdout with newline
 * @param value RuntimeValue to print
 */
void io_print_internal(const RuntimeValue *value)
{
    if (!value) {
        printf("null\n");
        return;
    }

    char *str = rv_to_string((RuntimeValue *)value);
    if (str) {
        printf("%s\n", str);
        memory_free(str);
    } else {
        printf("null\n");
    }
}

/**
 * @brief Print RuntimeValue to stdout without newline
 * @param value RuntimeValue to print
 */
void io_print_no_newline_internal(const RuntimeValue *value)
{
    if (!value) {
        printf("null");
        return;
    }

    char *str = rv_to_string((RuntimeValue *)value);
    if (str) {
        printf("%s", str);
        memory_free(str);
    } else {
        printf("null");
    }
}

/**
 * @brief Read a line from stdin
 * @return Newly allocated string containing user input, or NULL on error
 */
char *io_input_internal(void)
{
    char *buffer = NULL;
    size_t buffer_size = 0;
    ssize_t chars_read = getline(&buffer, &buffer_size, stdin);

    if (chars_read == -1) {
        if (buffer) {
            memory_free(buffer);
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
char *io_input_prompt_internal(const char *prompt)
{
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
bool io_write_file_internal(const char *filepath, const char *content)
{
    if (!filepath || !content) {
        return false;
    }

    FILE *f = fopen(filepath, "w");
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
bool io_append_file_internal(const char *filepath, const char *content)
{
    if (!filepath || !content) {
        return false;
    }

    FILE *f = fopen(filepath, "a");
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
bool io_file_exists_internal(const char *filepath)
{
    if (!filepath) {
        return false;
    }

    FILE *f = fopen(filepath, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/**
 * @brief Load JSON file as RuntimeValue object
 * @param filepath Path to JSON file
 * @return RuntimeValue object representing the JSON data, or NULL on error
 */
RuntimeValue *io_load_json_file_internal(const char *filepath)
{
    if (!filepath || !io_file_exists_internal(filepath)) {
        return NULL;
    }

    char *content = io_read_file_internal(filepath);
    if (!content) {
        return NULL;
    }

    RuntimeValue *result = json_parse(content);
    memory_free(content);

    return result;
}

/**
 * @brief Resolve module path with extensions
 * @param module_path Base path for the module
 * @return Newly allocated string with resolved path, or NULL if not found
 */
char *io_resolve_module_path_internal(const char *module_path)
{
    if (!module_path) {
        return NULL;
    }

    // Try different extensions in order of preference
    const char *extensions[] = {".zen", ".json", ".yaml", NULL};

    for (int i = 0; extensions[i]; i++) {
        size_t path_len = strlen(module_path) + strlen(extensions[i]) + 1;
        char *full_path = memory_alloc(path_len);
        if (!full_path)
            continue;

        size_t module_len = strlen(module_path);
        strncpy(full_path, module_path, module_len);
        full_path[module_len] = '\0';
        strncat(full_path, extensions[i], path_len - module_len - 1);

        if (io_file_exists_internal(full_path)) {
            return full_path;
        }

        memory_free(full_path);
    }

    // Check if the path exists as-is
    if (io_file_exists_internal(module_path)) {
        return memory_strdup(module_path);
    }

    return NULL;
}

// STDLIB WRAPPER FUNCTIONS - Match MANIFEST.json signatures

/**
 * @brief Print function for stdlib integration
 * @param args Array of RuntimeValue arguments
 * @param argc Number of arguments
 * @return Always returns null RuntimeValue
 */
RuntimeValue *io_print(RuntimeValue **args, size_t argc)
{
    for (size_t i = 0; i < argc; i++) {
        if (i > 0)
            printf(" ");
        io_print_no_newline_internal(args[i]);
    }
    printf("\n");
    return rv_new_null();
}

/**
 * @brief Input function for stdlib integration
 * @param args Array of RuntimeValue arguments (optional prompt)
 * @param argc Number of arguments
 * @return String RuntimeValue containing user input
 */
RuntimeValue *io_input(RuntimeValue **args, size_t argc)
{
    char *input_str = NULL;

    if (argc > 0 && args[0] && args[0]->type == RV_STRING) {
        input_str = io_input_prompt_internal(args[0]->data.string.data);
    } else {
        input_str = io_input_internal();
    }

    if (!input_str) {
        return rv_new_string("");
    }

    RuntimeValue *result = rv_new_string(input_str);
    memory_free(input_str);
    return result;
}

/**
 * @brief Read file function for stdlib integration
 * @param args Array of RuntimeValue arguments (filepath)
 * @param argc Number of arguments
 * @return String RuntimeValue containing file contents or error
 */
RuntimeValue *io_read_file(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("readFile requires a string filepath argument", -1);
    }

    char *content = io_read_file_internal(args[0]->data.string.data);
    if (!content) {
        return rv_new_error("Failed to read file", -1);
    }

    RuntimeValue *result = rv_new_string(content);
    memory_free(content);
    return result;
}

/**
 * @brief Write file function for stdlib integration
 * @param args Array of RuntimeValue arguments (filepath, content)
 * @param argc Number of arguments
 * @return Boolean RuntimeValue indicating success
 */
RuntimeValue *io_write_file(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || !args[0] || !args[1] || args[0]->type != RV_STRING ||
        args[1]->type != RV_STRING) {
        return rv_new_boolean(false);
    }

    bool success = io_write_file_internal(args[0]->data.string.data, args[1]->data.string.data);
    return rv_new_boolean(success);
}

/**
 * @brief Append to file function for stdlib integration
 * @param args Array of RuntimeValue arguments (filepath, content)
 * @param argc Number of arguments
 * @return Boolean RuntimeValue indicating success
 */
RuntimeValue *io_append_file(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || !args[0] || !args[1] || args[0]->type != RV_STRING ||
        args[1]->type != RV_STRING) {
        return rv_new_boolean(false);
    }

    bool success = io_append_file_internal(args[0]->data.string.data, args[1]->data.string.data);
    return rv_new_boolean(success);
}

/**
 * @brief Check file exists function for stdlib integration
 * @param args Array of RuntimeValue arguments (filepath)
 * @param argc Number of arguments
 * @return Boolean RuntimeValue indicating if file exists
 */
RuntimeValue *io_file_exists(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != RV_STRING) {
        return rv_new_boolean(false);
    }

    bool exists = io_file_exists_internal(args[0]->data.string.data);
    return rv_new_boolean(exists);
}
