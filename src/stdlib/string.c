/*
 * string.c
 * String manipulation functions for ZEN stdlib
 *
 * These are internal stdlib functions that extend the core functionality
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/string.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Get the length of a string or array value
 * @param args Arguments array containing string or array value
 * @param argc Number of arguments
 * @return Length as a number value, or error on invalid input
 */
RuntimeValue *string_length(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return error_new("length() requires exactly 1 argument");
    }

    const RuntimeValue *value = args[0];
    if (!value) {
        return error_new("length() requires a non-null argument");
    }

    if (value->type == RV_STRING) {
        if (!value->data.string.data) {
            return rv_new_number(0);
        }
        return rv_new_number((double)value->data.string.length);
    } else if (value->type == RV_ARRAY) {
        return rv_new_number((double)value->data.array.count);
    } else {
        return error_new("length() requires a string or array argument");
    }
}

/**
 * @brief Convert string to uppercase
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in uppercase
 */
RuntimeValue *string_upper(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return error_new("upper() requires exactly 1 argument");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("upper() requires a string argument");
    }

    if (!str_value->data.string.data) {
        return rv_new_string("");
    }

    char *original = str_value->data.string.data;
    size_t len = str_value->data.string.length;
    char *upper_str = memory_alloc(len + 1);

    if (!upper_str) {
        return rv_new_string("");
    }

    for (size_t i = 0; i < len; i++) {
        upper_str[i] = toupper(original[i]);
    }
    upper_str[len] = '\0';

    RuntimeValue *result = rv_new_string(upper_str);
    memory_free(upper_str);
    return result;
}

/**
 * @brief Convert string to lowercase
 * @param str_value String value to convert
 * @return New string value in lowercase
 */
RuntimeValue *string_lower(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return error_new("lower() requires exactly 1 argument");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return rv_new_string("");
    }

    if (!str_value->data.string.data) {
        return rv_new_string("");
    }

    char *original = str_value->data.string.data;
    size_t len = str_value->data.string.length;
    char *lower_str = memory_alloc(len + 1);

    if (!lower_str) {
        return rv_new_string("");
    }

    for (size_t i = 0; i < len; i++) {
        lower_str[i] = tolower(original[i]);
    }
    lower_str[len] = '\0';

    RuntimeValue *result = rv_new_string(lower_str);
    memory_free(lower_str);
    return result;
}

/**
 * @brief Trim whitespace from both ends of string
 * @param str_value String value to trim
 * @return New trimmed string value
 */
RuntimeValue *string_trim(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return error_new("trim() requires exactly 1 argument");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return rv_new_string("");
    }

    if (!str_value->data.string.data) {
        return rv_new_string("");
    }

    char *original = str_value->data.string.data;
    size_t len = str_value->data.string.length;

    // Find start of non-whitespace
    size_t start = 0;
    while (start < len && isspace(original[start])) {
        start++;
    }

    // Find end of non-whitespace
    size_t end = len;
    while (end > start && isspace(original[end - 1])) {
        end--;
    }

    // Create trimmed string
    size_t trimmed_len = end - start;
    char *trimmed = memory_alloc(trimmed_len + 1);
    if (!trimmed) {
        return rv_new_string("");
    }

    memcpy(trimmed, original + start, trimmed_len);
    trimmed[trimmed_len] = '\0';

    RuntimeValue *result = rv_new_string(trimmed);
    memory_free(trimmed);
    return result;
}

/**
 * @brief Split string by delimiter
 * @param str_value String value to split
 * @param delimiter Delimiter string
 * @return Array value containing split parts
 */
RuntimeValue *string_split(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return error_new("split() requires exactly 2 arguments");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("split() requires a string as first argument");
    }

    const RuntimeValue *delim_value = args[1];
    if (!delim_value || delim_value->type != RV_STRING) {
        return error_new("split() requires a string delimiter as second argument");
    }

    const char *delimiter = delim_value->data.string.data;
    RuntimeValue *result = rv_new_array();
    if (!result) {
        return result;
    }

    if (!str_value || str_value->type != RV_STRING || !delimiter) {
        return result;  // Empty array
    }

    if (!str_value->data.string.data) {
        return result;  // Empty array
    }

    char *original = str_value->data.string.data;
    size_t delim_len = strlen(delimiter);

    if (delim_len == 0) {
        // Split into individual characters
        size_t len = str_value->data.string.length;
        for (size_t i = 0; i < len; i++) {
            char single_char[2] = {original[i], '\0'};
            RuntimeValue *char_val = rv_new_string(single_char);
            rv_array_push(result, char_val);
            rv_unref(char_val);
        }
        return result;
    }

    // Split the string
    char *str_copy = memory_strdup(original);
    if (!str_copy) {
        return result;
    }

    char *token = strtok(str_copy, delimiter);
    while (token != NULL) {
        RuntimeValue *token_val = rv_new_string(token);
        rv_array_push(result, token_val);
        rv_unref(token_val);
        token = strtok(NULL, delimiter);
    }

    memory_free(str_copy);
    return result;
}

/**
 * @brief Check if string contains substring
 * @param str_value String value to search in
 * @param substring Substring to find
 * @return Boolean value indicating if substring was found
 */
RuntimeValue *string_contains(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return error_new("contains() requires exactly 2 arguments");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("contains() requires a string as first argument");
    }

    const RuntimeValue *substr_value = args[1];
    if (!substr_value || substr_value->type != RV_STRING) {
        return error_new("contains() requires a string as second argument");
    }

    const char *substring = substr_value->data.string.data;
    if (!str_value || str_value->type != RV_STRING || !substring) {
        return rv_new_boolean(false);
    }

    if (!str_value->data.string.data) {
        return rv_new_boolean(false);
    }

    char *found = strstr(str_value->data.string.data, substring);
    return rv_new_boolean(found != NULL);
}

/**
 * @brief Replace all occurrences of substring with replacement
 * @param str_value String value to search in
 * @param search Substring to replace
 * @param replace Replacement string
 * @return New string value with replacements made
 */
RuntimeValue *string_replace(RuntimeValue **args, size_t argc)
{
    if (argc != 3) {
        return error_new("replace() requires exactly 3 arguments");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("replace() requires a string as first argument");
    }

    const RuntimeValue *search_value = args[1];
    if (!search_value || search_value->type != RV_STRING) {
        return error_new("replace() requires a string as second argument");
    }

    const RuntimeValue *replace_value = args[2];
    if (!replace_value || replace_value->type != RV_STRING) {
        return error_new("replace() requires a string as third argument");
    }

    const char *search = search_value->data.string.data;
    const char *replace = replace_value->data.string.data;
    if (!str_value || str_value->type != RV_STRING || !search || !replace) {
        return rv_copy((RuntimeValue *)str_value);
    }

    if (!str_value->data.string.data) {
        return rv_new_string("");
    }

    char *original = str_value->data.string.data;
    size_t search_len = strlen(search);
    size_t replace_len = strlen(replace);

    if (search_len == 0) {
        return rv_copy((RuntimeValue *)str_value);
    }

    // Count occurrences
    size_t count = 0;
    char *pos = original;
    while ((pos = strstr(pos, search)) != NULL) {
        count++;
        pos += search_len;
    }

    if (count == 0) {
        return rv_copy((RuntimeValue *)str_value);
    }

    // Calculate new length
    size_t original_len = str_value->data.string.length;
    size_t new_len = original_len - (count * search_len) + (count * replace_len);

    char *result_str = memory_alloc(new_len + 1);
    if (!result_str) {
        return rv_new_string("");
    }

    // Perform replacements
    char *src = original;
    char *dst = result_str;

    while ((pos = strstr(src, search)) != NULL) {
        // Copy part before match
        size_t part_len = pos - src;
        memcpy(dst, src, part_len);
        dst += part_len;

        // Copy replacement
        memcpy(dst, replace, replace_len);
        dst += replace_len;

        // Move source pointer past the match
        src = pos + search_len;
    }

    // Copy remaining part
    strcpy(dst, src);

    RuntimeValue *result = rv_new_string(result_str);
    memory_free(result_str);
    return result;
}

/**
 * @brief Extract substring from string
 * @param args Arguments array containing string, start index, and optional length
 * @param argc Number of arguments (2 or 3)
 * @return New string value containing the substring
 */
RuntimeValue *string_substring(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || argc > 3) {
        return error_new("substring() requires 2 or 3 arguments (string, start [, length])");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("substring() requires a string as first argument");
    }

    const RuntimeValue *start_value = args[1];
    if (!start_value || start_value->type != RV_NUMBER) {
        return error_new("substring() requires a number as second argument (start index)");
    }

    size_t length = SIZE_MAX;  // Default to end of string
    if (argc == 3) {
        const RuntimeValue *length_value = args[2];
        if (!length_value || length_value->type != RV_NUMBER) {
            return error_new("substring() requires a number as third argument (length)");
        }
        if (length_value->data.number < 0) {
            return error_new("substring() length cannot be negative");
        }
        length = (size_t)length_value->data.number;
    }

    if (!str_value->data.string.data) {
        return rv_new_string("");
    }

    const char *original = str_value->data.string.data;
    size_t str_len = str_value->data.string.length;
    int start_idx = (int)start_value->data.number;

    // Handle negative start index (count from end)
    if (start_idx < 0) {
        start_idx = (int)str_len + start_idx;
        if (start_idx < 0) {
            start_idx = 0;  // Clamp to start of string
        }
    }

    // Bounds checking
    if ((size_t)start_idx >= str_len) {
        return rv_new_string("");
    }

    size_t start_pos = (size_t)start_idx;
    size_t remaining = str_len - start_pos;
    size_t actual_length = (length == SIZE_MAX || length > remaining) ? remaining : length;

    char *substring = memory_alloc(actual_length + 1);
    if (!substring) {
        return rv_new_string("");
    }

    memcpy(substring, original + start_pos, actual_length);
    substring[actual_length] = '\0';

    RuntimeValue *result = rv_new_string(substring);
    memory_free(substring);
    return result;
}

/**
 * @brief Find index of substring in string
 * @param args Arguments array containing string and substring
 * @param argc Number of arguments
 * @return Number value with index of substring, or -1 if not found
 */
RuntimeValue *string_index_of(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return error_new("index_of() requires exactly 2 arguments");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("index_of() requires a string as first argument");
    }

    const RuntimeValue *substr_value = args[1];
    if (!substr_value || substr_value->type != RV_STRING) {
        return error_new("index_of() requires a string as second argument");
    }

    if (!str_value->data.string.data || !substr_value->data.string.data) {
        return rv_new_number(-1);
    }

    const char *haystack = str_value->data.string.data;
    const char *needle = substr_value->data.string.data;

    char *found = strstr(haystack, needle);
    if (!found) {
        return rv_new_number(-1);
    }

    return rv_new_number((double)(found - haystack));
}

/**
 * @brief Concatenate two or more strings
 * @param args Arguments array containing strings to concatenate
 * @param argc Number of arguments
 * @return New string value containing concatenated result
 */
RuntimeValue *string_concat(RuntimeValue **args, size_t argc)
{
    if (argc == 0) {
        return rv_new_string("");
    }

    // Calculate total length needed
    size_t total_length = 0;
    for (size_t i = 0; i < argc; i++) {
        if (!args[i] || args[i]->type != RV_STRING) {
            return error_new("concat() requires all arguments to be strings");
        }
        if (args[i]->data.string.data) {
            total_length += args[i]->data.string.length;
        }
    }

    char *result_str = memory_alloc(total_length + 1);
    if (!result_str) {
        return rv_new_string("");
    }

    // Concatenate all strings
    char *pos = result_str;
    for (size_t i = 0; i < argc; i++) {
        if (args[i]->data.string.data) {
            size_t len = args[i]->data.string.length;
            memcpy(pos, args[i]->data.string.data, len);
            pos += len;
        }
    }
    *pos = '\0';

    RuntimeValue *result = rv_new_string(result_str);
    memory_free(result_str);
    return result;
}

/**
 * @brief Check if string starts with prefix
 * @param args Arguments array containing string and prefix
 * @param argc Number of arguments
 * @return Boolean value indicating if string starts with prefix
 */
RuntimeValue *string_starts_with(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return error_new("starts_with() requires exactly 2 arguments");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("starts_with() requires a string as first argument");
    }

    const RuntimeValue *prefix_value = args[1];
    if (!prefix_value || prefix_value->type != RV_STRING) {
        return error_new("starts_with() requires a string as second argument");
    }

    if (!str_value->data.string.data || !prefix_value->data.string.data) {
        return rv_new_boolean(false);
    }

    const char *string = str_value->data.string.data;
    const char *prefix = prefix_value->data.string.data;
    size_t prefix_len = prefix_value->data.string.length;
    size_t str_len = str_value->data.string.length;

    if (prefix_len > str_len) {
        return rv_new_boolean(false);
    }

    bool matches = memcmp(string, prefix, prefix_len) == 0;
    return rv_new_boolean(matches);
}

/**
 * @brief Check if string ends with suffix
 * @param args Arguments array containing string and suffix
 * @param argc Number of arguments
 * @return Boolean value indicating if string ends with suffix
 */
RuntimeValue *string_ends_with(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return error_new("ends_with() requires exactly 2 arguments");
    }

    const RuntimeValue *str_value = args[0];
    if (!str_value || str_value->type != RV_STRING) {
        return error_new("ends_with() requires a string as first argument");
    }

    const RuntimeValue *suffix_value = args[1];
    if (!suffix_value || suffix_value->type != RV_STRING) {
        return error_new("ends_with() requires a string as second argument");
    }

    if (!str_value->data.string.data || !suffix_value->data.string.data) {
        return rv_new_boolean(false);
    }

    const char *string = str_value->data.string.data;
    const char *suffix = suffix_value->data.string.data;
    size_t suffix_len = suffix_value->data.string.length;
    size_t str_len = str_value->data.string.length;

    if (suffix_len > str_len) {
        return rv_new_boolean(false);
    }

    const char *start_pos = string + str_len - suffix_len;
    bool matches = memcmp(start_pos, suffix, suffix_len) == 0;
    return rv_new_boolean(matches);
}

/**
 * @brief Convert string to uppercase (alias for string_upper)
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in uppercase
 */
RuntimeValue *string_to_upper(RuntimeValue **args, size_t argc) { return string_upper(args, argc); }

/**
 * @brief Convert string to lowercase (alias for string_lower)
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in lowercase
 */
RuntimeValue *string_to_lower(RuntimeValue **args, size_t argc) { return string_lower(args, argc); }