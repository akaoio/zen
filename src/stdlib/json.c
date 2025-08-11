/*
 * json.c
 * JSON parsing and generation - Self-contained implementation
 *
 * This file follows MANIFEST.json specification
 * Function signatures must match manifest exactly
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/json.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum file size for JSON parsing (64MB) to prevent memory exhaustion
#define MAX_JSON_FILE_SIZE (64 * 1024 * 1024)

// JSON parser state
typedef struct {
    const char *input;
    size_t pos;
    size_t length;
    char current_char;
} JsonParser;

// Forward declarations
static RuntimeValue *parse_value(JsonParser *parser);
static RuntimeValue *parse_object(JsonParser *parser);
static RuntimeValue *parse_array(JsonParser *parser);
static RuntimeValue *parse_string(JsonParser *parser);
static RuntimeValue *parse_number(JsonParser *parser);
static void skip_whitespace(JsonParser *parser);
static bool peek_char(JsonParser *parser, char expected);
static bool consume_char(JsonParser *parser, char expected);
static void advance(JsonParser *parser);
static char *json_stringify_internal(const RuntimeValue *value, int indent_level, int indent_size);
static char *escape_json_string(const char *str, size_t len);
static char *unescape_json_string(const char *str, size_t len, size_t *out_len);

// Parser helper functions
static void advance(JsonParser *parser)
{
    if (parser->pos < parser->length) {
        parser->pos++;
        parser->current_char = (parser->pos < parser->length) ? parser->input[parser->pos] : '\0';
    } else {
        parser->current_char = '\0';
    }
}

static void skip_whitespace(JsonParser *parser)
{
    while (parser->current_char && (parser->current_char == ' ' || parser->current_char == '\t' ||
                                    parser->current_char == '\n' || parser->current_char == '\r')) {
        advance(parser);
    }
}

static bool peek_char(JsonParser *parser, char expected)
{
    return parser->current_char == expected;
}

static bool consume_char(JsonParser *parser, char expected)
{
    if (parser->current_char == expected) {
        advance(parser);
        return true;
    }
    return false;
}

static char *unescape_json_string(const char *str, size_t len, size_t *out_len)
{
    char *result = memory_alloc(len + 1);
    if (!result)
        return NULL;

    size_t write_pos = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\\' && i + 1 < len) {
            i++;  // Skip the backslash
            switch (str[i]) {
            case '"':
                result[write_pos++] = '"';
                break;
            case '\\':
                result[write_pos++] = '\\';
                break;
            case '/':
                result[write_pos++] = '/';
                break;
            case 'b':
                result[write_pos++] = '\b';
                break;
            case 'f':
                result[write_pos++] = '\f';
                break;
            case 'n':
                result[write_pos++] = '\n';
                break;
            case 'r':
                result[write_pos++] = '\r';
                break;
            case 't':
                result[write_pos++] = '\t';
                break;
            case 'u': {
                // Unicode escape sequence \uXXXX
                if (i + 4 < len) {
                    // For simplicity, just copy the original sequence
                    // A full implementation would convert to UTF-8
                    result[write_pos++] = '\\';
                    result[write_pos++] = 'u';
                    result[write_pos++] = str[++i];
                    result[write_pos++] = str[++i];
                    result[write_pos++] = str[++i];
                    result[write_pos++] = str[++i];
                } else {
                    result[write_pos++] = str[i];
                }
                break;
            }
            default:
                result[write_pos++] = str[i];
                break;
            }
        } else {
            result[write_pos++] = str[i];
        }
    }

    result[write_pos] = '\0';
    if (out_len)
        *out_len = write_pos;
    return result;
}

static char *escape_json_string(const char *str, size_t len)
{
    if (!str)
        return memory_strdup(""
                             "");

    // Estimate escaped length (worst case: every char needs escaping)
    size_t escaped_len = len * 2 + 3;  // +3 for quotes and null terminator
    char *buffer = memory_alloc(escaped_len);
    if (!buffer)
        return NULL;

    buffer[0] = '"';
    size_t j = 1;

    for (size_t i = 0; i < len && str[i]; i++) {
        char c = str[i];
        if (j >= escaped_len - 2)
            break;  // Safety check

        switch (c) {
        case '"':
            buffer[j++] = '\\';
            buffer[j++] = '"';
            break;
        case '\\':
            buffer[j++] = '\\';
            buffer[j++] = '\\';
            break;
        case '\b':
            buffer[j++] = '\\';
            buffer[j++] = 'b';
            break;
        case '\f':
            buffer[j++] = '\\';
            buffer[j++] = 'f';
            break;
        case '\n':
            buffer[j++] = '\\';
            buffer[j++] = 'n';
            break;
        case '\r':
            buffer[j++] = '\\';
            buffer[j++] = 'r';
            break;
        case '\t':
            buffer[j++] = '\\';
            buffer[j++] = 't';
            break;
        default:
            if ((unsigned char)c < 0x20) {
                // Control characters - escape as \uXXXX
                j += snprintf(&buffer[j], escaped_len - j, "\\u%04x", (unsigned char)c);
            } else {
                buffer[j++] = c;
            }
            break;
        }
    }

    buffer[j++] = '"';
    buffer[j] = '\0';
    return buffer;
}

static RuntimeValue *parse_string(JsonParser *parser)
{
    if (!consume_char(parser, '"')) {
        return rv_new_error("Expected '\"' at start of string", -1);
    }

    size_t start = parser->pos;
    while (parser->current_char && parser->current_char != '"') {
        if (parser->current_char == '\\') {
            advance(parser);  // Skip backslash
            if (parser->current_char) {
                advance(parser);  // Skip escaped character
            }
        } else {
            advance(parser);
        }
    }

    if (!peek_char(parser, '"')) {
        return rv_new_error("Unterminated string", -1);
    }

    size_t len = parser->pos - start;
    size_t unescaped_len;
    char *unescaped = unescape_json_string(parser->input + start, len, &unescaped_len);
    if (!unescaped) {
        return rv_new_error("Failed to unescape string", -1);
    }

    advance(parser);  // Skip closing quote

    RuntimeValue *result = rv_new_string(unescaped);
    memory_free(unescaped);
    return result;
}

static RuntimeValue *parse_number(JsonParser *parser)
{
    size_t start = parser->pos;

    // Handle negative sign
    if (parser->current_char == '-') {
        advance(parser);
    }

    // Must have at least one digit
    if (!parser->current_char || !isdigit(parser->current_char)) {
        return rv_new_error("Invalid number format", -1);
    }

    // Parse integer part
    if (parser->current_char == '0') {
        advance(parser);
        // After '0', we can only have '.', 'e', 'E', or end
    } else {
        // Parse digits 1-9 followed by any digits
        while (parser->current_char && isdigit(parser->current_char)) {
            advance(parser);
        }
    }

    // Parse fractional part
    if (parser->current_char == '.') {
        advance(parser);
        if (!parser->current_char || !isdigit(parser->current_char)) {
            return rv_new_error("Invalid number format: expected digit after '.'", -1);
        }
        while (parser->current_char && isdigit(parser->current_char)) {
            advance(parser);
        }
    }

    // Parse exponent part
    if (parser->current_char == 'e' || parser->current_char == 'E') {
        advance(parser);
        if (parser->current_char == '+' || parser->current_char == '-') {
            advance(parser);
        }
        if (!parser->current_char || !isdigit(parser->current_char)) {
            return rv_new_error("Invalid number format: expected digit in exponent", -1);
        }
        while (parser->current_char && isdigit(parser->current_char)) {
            advance(parser);
        }
    }

    size_t len = parser->pos - start;
    if (len == 0) {
        return rv_new_error("Invalid number format: empty", -1);
    }

    char *num_str = memory_alloc(len + 1);
    if (!num_str) {
        return rv_new_error("Memory allocation failed", -1);
    }
    memcpy(num_str, parser->input + start, len);
    num_str[len] = '\0';

    char *endptr;
    errno = 0;
    double num = strtod(num_str, &endptr);

    if (*endptr != '\0' || errno == ERANGE) {
        memory_free(num_str);
        return rv_new_error("Invalid number format", -1);
    }

    memory_free(num_str);
    return rv_new_number(num);
}

static RuntimeValue *parse_array(JsonParser *parser)
{
    if (!consume_char(parser, '[')) {
        return rv_new_error("Expected '[' at start of array", -1);
    }

    RuntimeValue *array = rv_new_array();
    if (!array) {
        return rv_new_error("Failed to create array", -1);
    }

    skip_whitespace(parser);

    // Handle empty array
    if (peek_char(parser, ']')) {
        advance(parser);
        return array;
    }

    // Parse array elements
    while (true) {
        skip_whitespace(parser);

        RuntimeValue *element = parse_value(parser);
        if (rv_is_error(element)) {
            rv_unref(array);
            return element;
        }

        rv_array_push(array, element);
        rv_unref(element);  // array now owns the reference

        skip_whitespace(parser);

        if (peek_char(parser, ']')) {
            advance(parser);
            break;
        } else if (peek_char(parser, ',')) {
            advance(parser);
        } else {
            rv_unref(array);
            return rv_new_error("Expected ',' or ']' in array", -1);
        }
    }

    return array;
}

static RuntimeValue *parse_object(JsonParser *parser)
{
    if (!consume_char(parser, '{')) {
        return rv_new_error("Expected '{' at start of object", -1);
    }

    RuntimeValue *object = rv_new_object();
    if (!object) {
        return rv_new_error("Failed to create object", -1);
    }

    skip_whitespace(parser);

    // Handle empty object
    if (peek_char(parser, '}')) {
        advance(parser);
        return object;
    }

    // Parse object key-value pairs
    while (true) {
        skip_whitespace(parser);

        // Parse key (must be string)
        if (!peek_char(parser, '"')) {
            rv_unref(object);
            return rv_new_error("Expected string key in object", -1);
        }

        RuntimeValue *key_rv = parse_string(parser);
        if (rv_is_error(key_rv)) {
            rv_unref(object);
            return key_rv;
        }

        const char *key = rv_get_string(key_rv);

        skip_whitespace(parser);

        if (!consume_char(parser, ':')) {
            rv_unref(key_rv);
            rv_unref(object);
            return rv_new_error("Expected ':' after object key", -1);
        }

        skip_whitespace(parser);

        RuntimeValue *value = parse_value(parser);
        if (rv_is_error(value)) {
            rv_unref(key_rv);
            rv_unref(object);
            return value;
        }

        rv_object_set(object, key, value);
        rv_unref(key_rv);
        rv_unref(value);  // object now owns the reference

        skip_whitespace(parser);

        if (peek_char(parser, '}')) {
            advance(parser);
            break;
        } else if (peek_char(parser, ',')) {
            advance(parser);
        } else {
            rv_unref(object);
            return rv_new_error("Expected ',' or '}' in object", -1);
        }
    }

    return object;
}

static RuntimeValue *parse_value(JsonParser *parser)
{
    skip_whitespace(parser);

    switch (parser->current_char) {
    case '"':
        return parse_string(parser);
    case '[':
        return parse_array(parser);
    case '{':
        return parse_object(parser);
    case 't':
        if (parser->pos + 3 <= parser->length &&
            strncmp(parser->input + parser->pos, "true", 4) == 0) {
            parser->pos += 4;
            parser->current_char =
                (parser->pos < parser->length) ? parser->input[parser->pos] : '\0';
            return rv_new_boolean(true);
        }
        break;
    case 'f':
        if (parser->pos + 4 <= parser->length &&
            strncmp(parser->input + parser->pos, "false", 5) == 0) {
            parser->pos += 5;
            parser->current_char =
                (parser->pos < parser->length) ? parser->input[parser->pos] : '\0';
            return rv_new_boolean(false);
        }
        break;
    case 'n':
        if (parser->pos + 3 <= parser->length &&
            strncmp(parser->input + parser->pos, "null", 4) == 0) {
            parser->pos += 4;
            parser->current_char =
                (parser->pos < parser->length) ? parser->input[parser->pos] : '\0';
            return rv_new_null();
        }
        break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return parse_number(parser);
    }

    return rv_new_error("Unexpected character in JSON", -1);
}

/**
 * @brief Parse JSON string to RuntimeValue
 * @param json_string JSON string to parse
 * @return Newly allocated RuntimeValue representing the JSON data, or error RuntimeValue on failure
 */
RuntimeValue *json_parse(const char *json_string)
{
    if (!json_string) {
        return rv_new_error("JSON string is NULL", -1);
    }

    JsonParser parser = {.input = json_string,
                         .pos = 0,
                         .length = strlen(json_string),
                         .current_char = json_string[0]};

    RuntimeValue *result = parse_value(&parser);

    // Check for trailing content
    skip_whitespace(&parser);
    if (parser.current_char != '\0') {
        if (!rv_is_error(result)) {
            rv_unref(result);
        }
        return rv_new_error("Unexpected content after JSON value", -1);
    }

    return result;
}

static char *json_stringify_internal(const RuntimeValue *value, int indent_level, int indent_size)
{
    if (!value) {
        return memory_strdup("null");
    }

    switch (value->type) {
    case RV_NULL:
        return memory_strdup("null");

    case RV_BOOLEAN:
        return memory_strdup(value->data.boolean ? "true" : "false");

    case RV_NUMBER: {
        char buffer[32];
        // Check if it's an integer
        if (value->data.number == floor(value->data.number) &&
            value->data.number >= -9007199254740992.0 && value->data.number <= 9007199254740992.0) {
            snprintf(buffer, sizeof(buffer), "%.0f", value->data.number);
        } else {
            snprintf(buffer, sizeof(buffer), "%g", value->data.number);
        }
        return memory_strdup(buffer);
    }

    case RV_STRING:
        return escape_json_string(value->data.string.data, value->data.string.length);

    case RV_ARRAY: {
        size_t count = rv_array_length((RuntimeValue *)value);
        if (count == 0) {
            return memory_strdup("[]");
        }

        // Start with opening bracket
        char *result = memory_strdup("[");
        if (!result)
            return NULL;

        for (size_t i = 0; i < count; i++) {
            RuntimeValue *element = rv_array_get((RuntimeValue *)value, i);
            char *element_json = json_stringify_internal(element, indent_level + 1, indent_size);
            if (!element_json) {
                memory_free(result);
                return NULL;
            }

            // Concatenate element
            size_t old_len = strlen(result);
            size_t elem_len = strlen(element_json);
            size_t new_len =
                old_len + elem_len + (i < count - 1 ? 1 : 0) + 1;  // +1 for comma, +1 for null

            char *new_result = memory_alloc(new_len + 1);
            strcpy(new_result, result);
            strcat(new_result, element_json);
            if (i < count - 1) {
                strcat(new_result, ",");
            }

            memory_free(result);
            memory_free(element_json);
            result = new_result;
        }

        // Add closing bracket
        size_t len = strlen(result);
        char *final_result = memory_alloc(len + 2);
        strcpy(final_result, result);
        strcat(final_result, "]");
        memory_free(result);
        return final_result;
    }

    case RV_OBJECT: {
        RuntimeValue *obj = (RuntimeValue *)value;
        if (obj->data.object.count == 0) {
            return memory_strdup("{}");
        }

        char *result = memory_strdup("{");
        if (!result)
            return NULL;

        for (size_t i = 0; i < obj->data.object.count; i++) {
            const char *key = obj->data.object.keys[i];
            RuntimeValue *val = obj->data.object.values[i];

            // Escape key
            char *key_json = escape_json_string(key, strlen(key));
            if (!key_json) {
                memory_free(result);
                return NULL;
            }

            // Stringify value
            char *val_json = json_stringify_internal(val, indent_level + 1, indent_size);
            if (!val_json) {
                memory_free(result);
                memory_free(key_json);
                return NULL;
            }

            // Concatenate key:value pair
            size_t old_len = strlen(result);
            size_t key_len = strlen(key_json);
            size_t val_len = strlen(val_json);
            size_t new_len =
                old_len + key_len + val_len + 2 + (i < obj->data.object.count - 1 ? 1 : 0) + 1;
            // +2 for ":", +1 for comma, +1 for null

            char *new_result = memory_alloc(new_len + 1);
            strcpy(new_result, result);
            strcat(new_result, key_json);
            strcat(new_result, ":");
            strcat(new_result, val_json);
            if (i < obj->data.object.count - 1) {
                strcat(new_result, ",");
            }

            memory_free(result);
            memory_free(key_json);
            memory_free(val_json);
            result = new_result;
        }

        // Add closing brace
        size_t len = strlen(result);
        char *final_result = memory_alloc(len + 2);
        strcpy(final_result, result);
        strcat(final_result, "}");
        memory_free(result);
        return final_result;
    }

    case RV_ERROR:
    case RV_FUNCTION:
    default:
        return memory_strdup("null");
    }
}

/**
 * @brief Convert RuntimeValue to compact JSON string
 * @param value RuntimeValue to convert to JSON
 * @return Newly allocated JSON string, or NULL on error
 */
char *json_stringify(const RuntimeValue *value) { return json_stringify_internal(value, 0, 0); }

static char *
json_stringify_pretty_internal(const RuntimeValue *value, int indent_level, int indent_size)
{
    if (!value) {
        return memory_strdup("null");
    }

    switch (value->type) {
    case RV_NULL:
        return memory_strdup("null");

    case RV_BOOLEAN:
        return memory_strdup(value->data.boolean ? "true" : "false");

    case RV_NUMBER: {
        char buffer[32];
        if (value->data.number == floor(value->data.number) &&
            value->data.number >= -9007199254740992.0 && value->data.number <= 9007199254740992.0) {
            snprintf(buffer, sizeof(buffer), "%.0f", value->data.number);
        } else {
            snprintf(buffer, sizeof(buffer), "%g", value->data.number);
        }
        return memory_strdup(buffer);
    }

    case RV_STRING:
        return escape_json_string(value->data.string.data, value->data.string.length);

    case RV_ARRAY: {
        size_t count = rv_array_length((RuntimeValue *)value);
        if (count == 0) {
            return memory_strdup("[]");
        }

        char *result = memory_strdup("[");
        if (!result)
            return NULL;

        for (size_t i = 0; i < count; i++) {
            // Add newline and indentation
            char *indent = memory_alloc((indent_level + 1) * indent_size + 2);
            indent[0] = '\n';
            for (int j = 0; j < (indent_level + 1) * indent_size; j++) {
                indent[j + 1] = ' ';
            }
            indent[(indent_level + 1) * indent_size + 1] = '\0';

            RuntimeValue *element = rv_array_get((RuntimeValue *)value, i);
            char *element_json =
                json_stringify_pretty_internal(element, indent_level + 1, indent_size);

            // Concatenate
            size_t old_len = strlen(result);
            size_t indent_len = strlen(indent);
            size_t elem_len = strlen(element_json);
            size_t comma_len = (i < count - 1) ? 1 : 0;

            char *new_result = memory_alloc(old_len + indent_len + elem_len + comma_len + 1);
            strcpy(new_result, result);
            strcat(new_result, indent);
            strcat(new_result, element_json);
            if (i < count - 1) {
                strcat(new_result, ",");
            }

            memory_free(result);
            memory_free(indent);
            memory_free(element_json);
            result = new_result;
        }

        // Add closing bracket with proper indentation
        char *closing_indent = memory_alloc(indent_level * indent_size + 3);
        closing_indent[0] = '\n';
        for (int j = 0; j < indent_level * indent_size; j++) {
            closing_indent[j + 1] = ' ';
        }
        closing_indent[indent_level * indent_size + 1] = ']';
        closing_indent[indent_level * indent_size + 2] = '\0';

        size_t len = strlen(result);
        size_t closing_len = strlen(closing_indent);
        char *final_result = memory_alloc(len + closing_len + 1);
        strcpy(final_result, result);
        strcat(final_result, closing_indent);

        memory_free(result);
        memory_free(closing_indent);
        return final_result;
    }

    case RV_OBJECT: {
        RuntimeValue *obj = (RuntimeValue *)value;
        if (obj->data.object.count == 0) {
            return memory_strdup("{}");
        }

        char *result = memory_strdup("{");
        if (!result)
            return NULL;

        for (size_t i = 0; i < obj->data.object.count; i++) {
            // Add newline and indentation
            char *indent = memory_alloc((indent_level + 1) * indent_size + 2);
            indent[0] = '\n';
            for (int j = 0; j < (indent_level + 1) * indent_size; j++) {
                indent[j + 1] = ' ';
            }
            indent[(indent_level + 1) * indent_size + 1] = '\0';

            const char *key = obj->data.object.keys[i];
            RuntimeValue *val = obj->data.object.values[i];

            char *key_json = escape_json_string(key, strlen(key));
            char *val_json = json_stringify_pretty_internal(val, indent_level + 1, indent_size);

            // Concatenate key: value
            size_t old_len = strlen(result);
            size_t indent_len = strlen(indent);
            size_t key_len = strlen(key_json);
            size_t val_len = strlen(val_json);
            size_t comma_len = (i < obj->data.object.count - 1) ? 1 : 0;

            char *new_result =
                memory_alloc(old_len + indent_len + key_len + val_len + 3 + comma_len + 1);
            strcpy(new_result, result);
            strcat(new_result, indent);
            strcat(new_result, key_json);
            strcat(new_result, ": ");
            strcat(new_result, val_json);
            if (i < obj->data.object.count - 1) {
                strcat(new_result, ",");
            }

            memory_free(result);
            memory_free(indent);
            memory_free(key_json);
            memory_free(val_json);
            result = new_result;
        }

        // Add closing brace with proper indentation
        char *closing_indent = memory_alloc(indent_level * indent_size + 3);
        closing_indent[0] = '\n';
        for (int j = 0; j < indent_level * indent_size; j++) {
            closing_indent[j + 1] = ' ';
        }
        closing_indent[indent_level * indent_size + 1] = '}';
        closing_indent[indent_level * indent_size + 2] = '\0';

        size_t len = strlen(result);
        size_t closing_len = strlen(closing_indent);
        char *final_result = memory_alloc(len + closing_len + 1);
        strcpy(final_result, result);
        strcat(final_result, closing_indent);

        memory_free(result);
        memory_free(closing_indent);
        return final_result;
    }

    case RV_ERROR:
    case RV_FUNCTION:
    default:
        return memory_strdup("null");
    }
}

/**
 * @brief Convert RuntimeValue to formatted JSON string with indentation
 * @param value RuntimeValue to convert to JSON
 * @param indent_size Number of spaces per indentation level
 * @return Newly allocated formatted JSON string, or NULL on error
 */
char *json_stringify_pretty(const RuntimeValue *value, int indent_size)
{
    if (indent_size <= 0) {
        return json_stringify(value);
    }
    return json_stringify_pretty_internal(value, 0, indent_size);
}

/**
 * @brief Parse JSON file with comprehensive error handling
 * @param filename Path to JSON file to parse
 * @return Parsed RuntimeValue or error RuntimeValue with helpful error message
 */
RuntimeValue *json_parse_file_safe(const char *filename)
{
    if (!filename) {
        return rv_new_error("Filename is NULL", -1);
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        return rv_new_error("Cannot open file", -1);
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return rv_new_error("Cannot determine file size", -1);
    }

    if ((unsigned long)file_size > MAX_JSON_FILE_SIZE) {
        fclose(file);
        return rv_new_error("File too large", -2);
    }

    fseek(file, 0, SEEK_SET);

    char *content = memory_alloc(file_size + 1);
    if (!content) {
        fclose(file);
        return rv_new_error("Memory allocation failed", -1);
    }

    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);

    if ((long)read_size != file_size) {
        memory_free(content);
        return rv_new_error("Failed to read file completely", -1);
    }

    content[file_size] = '\0';

    RuntimeValue *result = json_parse(content);
    memory_free(content);

    return result;
}

/**
 * @brief Load JSON file wrapper function for ZEN stdlib
 * @param args Array of RuntimeValue arguments (filename)
 * @param argc Number of arguments (must be 1)
 * @return Parsed JSON RuntimeValue or error
 */
RuntimeValue *json_load_file(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("json_load_file requires exactly 1 argument", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("json_load_file requires a string filename", -1);
    }

    return json_parse_file_safe(args[0]->data.string.data);
}

// Stdlib wrapper functions
RuntimeValue *json_parse_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("jsonParse requires exactly 1 argument", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("jsonParse requires a string argument", -1);
    }

    return json_parse(args[0]->data.string.data);
}

RuntimeValue *json_stringify_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("jsonStringify requires exactly 1 argument", -1);
    }

    char *json_str = json_stringify(args[0]);
    if (!json_str) {
        return rv_new_error("Failed to stringify to JSON", -1);
    }

    RuntimeValue *result = rv_new_string(json_str);
    memory_free(json_str);
    return result;
}

RuntimeValue *json_stringify_pretty_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || argc > 2) {
        return rv_new_error("jsonPretty requires 1 or 2 arguments", -1);
    }

    int indent = 2;
    if (argc == 2 && args[1]->type == RV_NUMBER) {
        indent = (int)args[1]->data.number;
    }

    char *json_str = json_stringify_pretty(args[0], indent);
    if (!json_str) {
        return rv_new_error("Failed to stringify to pretty JSON", -1);
    }

    RuntimeValue *result = rv_new_string(json_str);
    memory_free(json_str);
    return result;
}