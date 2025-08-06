/*
 * string.c
 * String manipulation functions for ZEN stdlib
 * 
 * These are internal stdlib functions that extend the core functionality
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/string.h"
#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/**
 * @brief Get the length of a string value
 * @param str_value String value to measure
 * @return Length as a number value, or error on invalid input
 */
Value* zen_string_length(const Value* str_value) {
    if (!str_value || str_value->type != VALUE_STRING) {
        return value_new_number(0);
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_number(0);
    }
    
    return value_new_number((double)str_value->as.string->length);
}

/**
 * @brief Convert string to uppercase
 * @param str_value String value to convert
 * @return New string value in uppercase
 */
Value* zen_string_upper(const Value* str_value) {
    if (!str_value || str_value->type != VALUE_STRING) {
        return value_new_string("");
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    char* original = str_value->as.string->data;
    size_t len = str_value->as.string->length;
    char* upper_str = malloc(len + 1);
    
    if (!upper_str) {
        return value_new_string("");
    }
    
    for (size_t i = 0; i < len; i++) {
        upper_str[i] = toupper(original[i]);
    }
    upper_str[len] = '\0';
    
    Value* result = value_new_string(upper_str);
    free(upper_str);
    return result;
}

/**
 * @brief Convert string to lowercase
 * @param str_value String value to convert
 * @return New string value in lowercase
 */
Value* zen_string_lower(const Value* str_value) {
    if (!str_value || str_value->type != VALUE_STRING) {
        return value_new_string("");
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    char* original = str_value->as.string->data;
    size_t len = str_value->as.string->length;
    char* lower_str = malloc(len + 1);
    
    if (!lower_str) {
        return value_new_string("");
    }
    
    for (size_t i = 0; i < len; i++) {
        lower_str[i] = tolower(original[i]);
    }
    lower_str[len] = '\0';
    
    Value* result = value_new_string(lower_str);
    free(lower_str);
    return result;
}

/**
 * @brief Trim whitespace from both ends of string
 * @param str_value String value to trim
 * @return New trimmed string value
 */
Value* zen_string_trim(const Value* str_value) {
    if (!str_value || str_value->type != VALUE_STRING) {
        return value_new_string("");
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    char* original = str_value->as.string->data;
    size_t len = str_value->as.string->length;
    
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
    char* trimmed = malloc(trimmed_len + 1);
    if (!trimmed) {
        return value_new_string("");
    }
    
    memcpy(trimmed, original + start, trimmed_len);
    trimmed[trimmed_len] = '\0';
    
    Value* result = value_new_string(trimmed);
    free(trimmed);
    return result;
}

/**
 * @brief Split string by delimiter
 * @param str_value String value to split
 * @param delimiter Delimiter string
 * @return Array value containing split parts
 */
Value* zen_string_split(const Value* str_value, const char* delimiter) {
    Value* result = value_new(VALUE_ARRAY);
    if (!result || !result->as.array) {
        return result;
    }
    
    if (!str_value || str_value->type != VALUE_STRING || !delimiter) {
        return result; // Empty array
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return result; // Empty array
    }
    
    char* original = str_value->as.string->data;
    size_t delim_len = strlen(delimiter);
    
    if (delim_len == 0) {
        // Split into individual characters
        size_t len = str_value->as.string->length;
        result->as.array->items = malloc(sizeof(Value*) * len);
        if (result->as.array->items) {
            result->as.array->capacity = len;
            for (size_t i = 0; i < len; i++) {
                char single_char[2] = {original[i], '\0'};
                result->as.array->items[i] = value_new_string(single_char);
                result->as.array->length++;
            }
        }
        return result;
    }
    
    // Count occurrences to pre-allocate
    size_t count = 1;
    char* pos = original;
    while ((pos = strstr(pos, delimiter)) != NULL) {
        count++;
        pos += delim_len;
    }
    
    result->as.array->items = malloc(sizeof(Value*) * count);
    if (!result->as.array->items) {
        return result;
    }
    result->as.array->capacity = count;
    
    // Split the string
    char* str_copy = strdup(original);
    if (!str_copy) {
        return result;
    }
    
    char* token = strtok(str_copy, delimiter);
    while (token != NULL && result->as.array->length < count) {
        result->as.array->items[result->as.array->length] = value_new_string(token);
        result->as.array->length++;
        token = strtok(NULL, delimiter);
    }
    
    free(str_copy);
    return result;
}

/**
 * @brief Check if string contains substring
 * @param str_value String value to search in
 * @param substring Substring to find
 * @return Boolean value indicating if substring was found
 */
Value* zen_string_contains(const Value* str_value, const char* substring) {
    if (!str_value || str_value->type != VALUE_STRING || !substring) {
        return value_new_boolean(false);
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_boolean(false);
    }
    
    char* found = strstr(str_value->as.string->data, substring);
    return value_new_boolean(found != NULL);
}

/**
 * @brief Replace all occurrences of substring with replacement
 * @param str_value String value to search in
 * @param search Substring to replace
 * @param replace Replacement string
 * @return New string value with replacements made
 */
Value* zen_string_replace(const Value* str_value, const char* search, const char* replace) {
    if (!str_value || str_value->type != VALUE_STRING || !search || !replace) {
        return value_copy(str_value);
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    char* original = str_value->as.string->data;
    size_t search_len = strlen(search);
    size_t replace_len = strlen(replace);
    
    if (search_len == 0) {
        return value_copy(str_value);
    }
    
    // Count occurrences
    size_t count = 0;
    char* pos = original;
    while ((pos = strstr(pos, search)) != NULL) {
        count++;
        pos += search_len;
    }
    
    if (count == 0) {
        return value_copy(str_value);
    }
    
    // Calculate new length
    size_t original_len = str_value->as.string->length;
    size_t new_len = original_len - (count * search_len) + (count * replace_len);
    
    char* result_str = malloc(new_len + 1);
    if (!result_str) {
        return value_new_string("");
    }
    
    // Perform replacements
    char* src = original;
    char* dst = result_str;
    
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
    
    Value* result = value_new_string(result_str);
    free(result_str);
    return result;
}