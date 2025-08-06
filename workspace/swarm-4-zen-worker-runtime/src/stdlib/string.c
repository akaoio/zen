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

// Forward declaration
Value* object_get(Value* object, const char* key);

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

// ============================================================================
// ADVANCED STRING PROCESSING FUNCTIONS
// ============================================================================

/**
 * @brief Pattern matching using simple glob-style patterns (* and ?)
 * @param str_value String to match against
 * @param pattern_value Pattern with * and ? wildcards
 * @return Boolean indicating if string matches pattern
 */
Value* zen_string_match_pattern(const Value* str_value, const Value* pattern_value) {
    if (!str_value || str_value->type != VALUE_STRING ||
        !pattern_value || pattern_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    if (!str_value->as.string || !str_value->as.string->data ||
        !pattern_value->as.string || !pattern_value->as.string->data) {
        return value_new_boolean(false);
    }
    
    const char* str = str_value->as.string->data;
    const char* pattern = pattern_value->as.string->data;
    
    // Simple pattern matching with * and ?
    while (*pattern && *str) {
        if (*pattern == '*') {
            // Skip consecutive asterisks
            while (*pattern == '*') pattern++;
            
            if (!*pattern) return value_new_boolean(true); // Pattern ends with *
            
            // Try matching from every position
            while (*str) {
                Value* sub_str = value_new_string(str);
                Value* sub_pattern = value_new_string(pattern);
                Value* result = zen_string_match_pattern(sub_str, sub_pattern);
                
                bool match = result->as.boolean;
                value_unref(sub_str);
                value_unref(sub_pattern);
                value_unref(result);
                
                if (match) return value_new_boolean(true);
                str++;
            }
            return value_new_boolean(false);
        } else if (*pattern == '?' || *pattern == *str) {
            pattern++;
            str++;
        } else {
            return value_new_boolean(false);
        }
    }
    
    // Skip trailing asterisks
    while (*pattern == '*') pattern++;
    
    return value_new_boolean(!*pattern && !*str);
}

/**
 * @brief String interpolation with placeholder substitution
 * @param template_value Template string with {key} placeholders
 * @param values_value Object with key-value pairs for substitution
 * @return String with placeholders replaced
 */
Value* zen_string_interpolate(const Value* template_value, const Value* values_value) {
    if (!template_value || template_value->type != VALUE_STRING) {
        return value_new_string("");
    }
    
    if (!values_value || values_value->type != VALUE_OBJECT) {
        return value_copy(template_value);
    }
    
    if (!template_value->as.string || !template_value->as.string->data) {
        return value_new_string("");
    }
    
    const char* template_str = template_value->as.string->data;
    size_t template_len = strlen(template_str);
    
    // Estimate result size (template length * 2 for safety)
    size_t result_capacity = template_len * 2;
    char* result = malloc(result_capacity);
    if (!result) {
        return value_new_string("");
    }
    
    size_t result_pos = 0;
    size_t i = 0;
    
    while (i < template_len) {
        if (template_str[i] == '{') {
            // Find closing brace
            size_t key_start = i + 1;
            size_t key_end = key_start;
            
            while (key_end < template_len && template_str[key_end] != '}') {
                key_end++;
            }
            
            if (key_end < template_len) {
                // Extract key
                size_t key_len = key_end - key_start;
                char* key = malloc(key_len + 1);
                if (key) {
                    memcpy(key, template_str + key_start, key_len);
                    key[key_len] = '\0';
                    
                    // Look up value in object
                    Value* replacement = object_get((Value*)values_value, key);
                    if (replacement && replacement->type == VALUE_STRING &&
                        replacement->as.string && replacement->as.string->data) {
                        
                        const char* replacement_str = replacement->as.string->data;
                        size_t replacement_len = strlen(replacement_str);
                        
                        // Ensure capacity
                        while (result_pos + replacement_len >= result_capacity) {
                            result_capacity *= 2;
                            char* new_result = realloc(result, result_capacity);
                            if (!new_result) {
                                free(result);
                                free(key);
                                return value_new_string("");
                            }
                            result = new_result;
                        }
                        
                        // Copy replacement
                        memcpy(result + result_pos, replacement_str, replacement_len);
                        result_pos += replacement_len;
                    }
                    
                    free(key);
                }
                
                i = key_end + 1; // Skip past closing brace
            } else {
                // No closing brace found, treat as literal
                result[result_pos++] = template_str[i];
                i++;
            }
        } else {
            // Regular character
            if (result_pos >= result_capacity - 1) {
                result_capacity *= 2;
                char* new_result = realloc(result, result_capacity);
                if (!new_result) {
                    free(result);
                    return value_new_string("");
                }
                result = new_result;
            }
            
            result[result_pos++] = template_str[i];
            i++;
        }
    }
    
    result[result_pos] = '\0';
    Value* result_value = value_new_string(result);
    free(result);
    return result_value;
}

/**
 * @brief Pad string to specified length with padding character
 * @param str_value String to pad
 * @param length_value Target length
 * @param pad_char_value Padding character (default space)
 * @param left_pad_value True for left padding, false for right padding
 * @return Padded string
 */
Value* zen_string_pad(const Value* str_value, const Value* length_value, 
                      const Value* pad_char_value, const Value* left_pad_value) {
    if (!str_value || str_value->type != VALUE_STRING ||
        !length_value || length_value->type != VALUE_NUMBER) {
        return value_copy(str_value);
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    const char* original = str_value->as.string->data;
    size_t original_len = strlen(original);
    size_t target_len = (size_t)length_value->as.number;
    
    if (target_len <= original_len) {
        return value_copy(str_value);
    }
    
    char pad_char = ' '; // Default padding character
    if (pad_char_value && pad_char_value->type == VALUE_STRING &&
        pad_char_value->as.string && pad_char_value->as.string->data &&
        strlen(pad_char_value->as.string->data) > 0) {
        pad_char = pad_char_value->as.string->data[0];
    }
    
    bool left_pad = true; // Default to left padding
    if (left_pad_value && left_pad_value->type == VALUE_BOOLEAN) {
        left_pad = left_pad_value->as.boolean;
    }
    
    char* result = malloc(target_len + 1);
    if (!result) {
        return value_copy(str_value);
    }
    
    size_t pad_len = target_len - original_len;
    
    if (left_pad) {
        // Left padding
        memset(result, pad_char, pad_len);
        strcpy(result + pad_len, original);
    } else {
        // Right padding
        strcpy(result, original);
        memset(result + original_len, pad_char, pad_len);
        result[target_len] = '\0';
    }
    
    Value* result_value = value_new_string(result);
    free(result);
    return result_value;
}

/**
 * @brief Reverse a string
 * @param str_value String to reverse
 * @return Reversed string
 */
Value* zen_string_reverse(const Value* str_value) {
    if (!str_value || str_value->type != VALUE_STRING) {
        return value_new_string("");
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    const char* original = str_value->as.string->data;
    size_t len = strlen(original);
    
    char* reversed = malloc(len + 1);
    if (!reversed) {
        return value_new_string("");
    }
    
    for (size_t i = 0; i < len; i++) {
        reversed[i] = original[len - 1 - i];
    }
    reversed[len] = '\0';
    
    Value* result = value_new_string(reversed);
    free(reversed);
    return result;
}

/**
 * @brief Extract substring from string
 * @param str_value Source string
 * @param start_value Start position (0-based)
 * @param length_value Length to extract (optional)
 * @return Extracted substring
 */
Value* zen_string_substr(const Value* str_value, const Value* start_value, const Value* length_value) {
    if (!str_value || str_value->type != VALUE_STRING ||
        !start_value || start_value->type != VALUE_NUMBER) {
        return value_new_string("");
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    const char* original = str_value->as.string->data;
    size_t original_len = strlen(original);
    int start = (int)start_value->as.number;
    
    // Handle negative start (from end)
    if (start < 0) {
        start = (int)original_len + start;
    }
    
    if (start < 0 || start >= (int)original_len) {
        return value_new_string("");
    }
    
    size_t extract_len = original_len - start;
    if (length_value && length_value->type == VALUE_NUMBER) {
        size_t requested_len = (size_t)length_value->as.number;
        if (requested_len < extract_len) {
            extract_len = requested_len;
        }
    }
    
    char* result = malloc(extract_len + 1);
    if (!result) {
        return value_new_string("");
    }
    
    memcpy(result, original + start, extract_len);
    result[extract_len] = '\0';
    
    Value* result_value = value_new_string(result);
    free(result);
    return result_value;
}

/**
 * @brief Find index of substring in string
 * @param str_value String to search in
 * @param search_value Substring to find
 * @param start_value Start position for search (optional)
 * @return Index of first occurrence, or -1 if not found
 */
Value* zen_string_index_of(const Value* str_value, const Value* search_value, const Value* start_value) {
    if (!str_value || str_value->type != VALUE_STRING ||
        !search_value || search_value->type != VALUE_STRING) {
        return value_new_number(-1);
    }
    
    if (!str_value->as.string || !str_value->as.string->data ||
        !search_value->as.string || !search_value->as.string->data) {
        return value_new_number(-1);
    }
    
    const char* haystack = str_value->as.string->data;
    const char* needle = search_value->as.string->data;
    size_t start_pos = 0;
    
    if (start_value && start_value->type == VALUE_NUMBER) {
        int start = (int)start_value->as.number;
        if (start > 0) {
            start_pos = (size_t)start;
        }
    }
    
    if (start_pos >= strlen(haystack)) {
        return value_new_number(-1);
    }
    
    const char* found = strstr(haystack + start_pos, needle);
    if (!found) {
        return value_new_number(-1);
    }
    
    return value_new_number((double)(found - haystack));
}

/**
 * @brief Capitalize first letter of each word
 * @param str_value String to title case
 * @return String with first letter of each word capitalized
 */
Value* zen_string_title_case(const Value* str_value) {
    if (!str_value || str_value->type != VALUE_STRING) {
        return value_new_string("");
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    const char* original = str_value->as.string->data;
    size_t len = strlen(original);
    
    char* result = malloc(len + 1);
    if (!result) {
        return value_new_string("");
    }
    
    bool capitalize_next = true;
    for (size_t i = 0; i < len; i++) {
        if (isspace(original[i])) {
            result[i] = original[i];
            capitalize_next = true;
        } else if (capitalize_next && isalpha(original[i])) {
            result[i] = toupper(original[i]);
            capitalize_next = false;
        } else {
            result[i] = tolower(original[i]);
        }
    }
    result[len] = '\0';
    
    Value* result_value = value_new_string(result);
    free(result);
    return result_value;
}

/**
 * @brief Escape special characters for use in strings
 * @param str_value String to escape
 * @return Escaped string with special characters escaped
 */
Value* zen_string_escape(const Value* str_value) {
    if (!str_value || str_value->type != VALUE_STRING) {
        return value_new_string("");
    }
    
    if (!str_value->as.string || !str_value->as.string->data) {
        return value_new_string("");
    }
    
    const char* original = str_value->as.string->data;
    size_t len = strlen(original);
    
    // Estimate result size (worst case: every char needs escaping)
    char* result = malloc(len * 2 + 1);
    if (!result) {
        return value_new_string("");
    }
    
    size_t result_pos = 0;
    for (size_t i = 0; i < len; i++) {
        switch (original[i]) {
            case '\n': result[result_pos++] = '\\'; result[result_pos++] = 'n'; break;
            case '\r': result[result_pos++] = '\\'; result[result_pos++] = 'r'; break;
            case '\t': result[result_pos++] = '\\'; result[result_pos++] = 't'; break;
            case '\\': result[result_pos++] = '\\'; result[result_pos++] = '\\'; break;
            case '"':  result[result_pos++] = '\\'; result[result_pos++] = '"'; break;
            case '\'': result[result_pos++] = '\\'; result[result_pos++] = '\''; break;
            default:   result[result_pos++] = original[i]; break;
        }
    }
    result[result_pos] = '\0';
    
    Value* result_value = value_new_string(result);
    free(result);
    return result_value;
}