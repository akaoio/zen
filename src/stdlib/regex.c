/**
 * @file regex.c
 * @brief PCRE2-based regular expression support for ZEN language
 * @details Natural language regex patterns with comprehensive error handling
 *
 * This module implements regular expression functionality using the PCRE2 library,
 * providing pattern matching, replacement, and splitting capabilities optimized
 * for ZEN's natural language syntax.
 *
 * Performance targets:
 * - Pattern compilation: <5ms average
 * - Pattern matching: <1ms for typical text
 * - Memory usage: <1KB per compiled pattern
 *
 * @author ZEN Multi-Swarm Development System
 * @version 1.0.0
 */

#include "zen/stdlib/regex.h"

#include "zen/config.h"
#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/types/array.h"
#include "zen/types/object.h"

#include <stdio.h>
#include <string.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

/**
 * @brief Compiled regex pattern cache entry
 */
typedef struct {
    char *pattern_string;            ///< Original pattern string
    pcre2_code_8 *compiled_pattern;  ///< PCRE2 compiled pattern
    pcre2_match_data_8 *match_data;  ///< Match data structure
    size_t ref_count;                ///< Reference count for cleanup
} RegexCache;

/**
 * @brief Global pattern cache with hash table for efficient lookup
 */
static RegexCache *pattern_cache = NULL;
static size_t cache_size = 0;
static size_t cache_capacity = 0;

#define REGEX_CACHE_INITIAL_CAPACITY 32
#define REGEX_CACHE_MAX_SIZE         1024

/**
 * @brief Default PCRE2 options for ZEN patterns
 */
static const uint32_t ZEN_PCRE2_OPTIONS = PCRE2_UTF | PCRE2_UCP | PCRE2_MULTILINE;

/**
 * @brief Find cached compiled pattern
 * @param pattern Pattern string to find
 * @return Cached regex entry or NULL if not found
 */
static RegexCache *find_cached_pattern(const char *pattern)
{
    if (!pattern || !pattern_cache) {
        return NULL;
    }

    for (size_t i = 0; i < cache_size; i++) {
        if (pattern_cache[i].pattern_string &&
            strcmp(pattern_cache[i].pattern_string, pattern) == 0) {
            return &pattern_cache[i];
        }
    }
    return NULL;
}

/**
 * @brief Add pattern to cache
 * @param pattern Pattern string
 * @param compiled_pattern PCRE2 compiled pattern
 * @param match_data PCRE2 match data
 * @return Cached entry or NULL on failure
 */
static RegexCache *
add_to_cache(const char *pattern, pcre2_code_8 *compiled_pattern, pcre2_match_data_8 *match_data)
{
    if (!pattern || !compiled_pattern || !match_data) {
        return NULL;
    }

    // Expand cache if needed (with better capacity management)
    if (cache_size >= cache_capacity) {
        size_t new_capacity =
            cache_capacity == 0 ? REGEX_CACHE_INITIAL_CAPACITY : cache_capacity * 2;

        // Prevent cache from growing too large
        if (new_capacity > REGEX_CACHE_MAX_SIZE) {
            // If cache is full, remove least recently used entries
            // For simplicity, remove the first half of entries
            size_t keep_count = cache_size / 2;

            // Clean up entries being removed
            for (size_t i = 0; i < cache_size - keep_count; i++) {
                if (pattern_cache[i].pattern_string) {
                    memory_free(pattern_cache[i].pattern_string);
                }
                if (pattern_cache[i].compiled_pattern) {
                    pcre2_code_free_8(pattern_cache[i].compiled_pattern);
                }
                if (pattern_cache[i].match_data) {
                    pcre2_match_data_free_8(pattern_cache[i].match_data);
                }
            }

            // Move remaining entries to beginning
            memmove(pattern_cache,
                    &pattern_cache[cache_size - keep_count],
                    keep_count * sizeof(RegexCache));
            cache_size = keep_count;

            new_capacity = cache_capacity;  // Keep same capacity
        }

        if (new_capacity != cache_capacity) {
            RegexCache *new_cache =
                memory_realloc(pattern_cache, new_capacity * sizeof(RegexCache));
            if (!new_cache) {
                return NULL;
            }
            pattern_cache = new_cache;
            cache_capacity = new_capacity;
        }
    }

    // Add to cache
    RegexCache *entry = &pattern_cache[cache_size++];
    entry->pattern_string = memory_strdup(pattern);
    entry->compiled_pattern = compiled_pattern;
    entry->match_data = match_data;
    entry->ref_count = 1;

    if (!entry->pattern_string) {
        cache_size--;  // Rollback
        return NULL;
    }

    return entry;
}

/**
 * @brief Compile regex pattern with caching
 * @param pattern Pattern string to compile
 * @return Cached regex entry or NULL on failure
 */
static RegexCache *compile_pattern_cached(const char *pattern)
{
    if (!pattern) {
        return NULL;
    }

    // Check cache first
    RegexCache *cached = find_cached_pattern(pattern);
    if (cached) {
        cached->ref_count++;
        return cached;
    }

    // Compile new pattern
    int error_code;
    PCRE2_SIZE error_offset;

    pcre2_code_8 *compiled_pattern = pcre2_compile_8((PCRE2_SPTR8)pattern,
                                                     PCRE2_ZERO_TERMINATED,
                                                     ZEN_PCRE2_OPTIONS,
                                                     &error_code,
                                                     &error_offset,
                                                     NULL);

    if (!compiled_pattern) {
        return NULL;
    }

    // Create match data
    pcre2_match_data_8 *match_data = pcre2_match_data_create_from_pattern_8(compiled_pattern, NULL);
    if (!match_data) {
        pcre2_code_free_8(compiled_pattern);
        return NULL;
    }

    // Add to cache
    RegexCache *entry = add_to_cache(pattern, compiled_pattern, match_data);
    if (!entry) {
        pcre2_match_data_free_8(match_data);
        pcre2_code_free_8(compiled_pattern);
        return NULL;
    }

    return entry;
}

/**
 * @brief Get detailed PCRE2 error message
 * @param error_code PCRE2 error code
 * @param error_offset Error offset in pattern
 * @param pattern Original pattern string
 * @return Error message Value
 */
static Value *get_pcre2_error_message(int error_code, PCRE2_SIZE error_offset, const char *pattern)
{
    PCRE2_UCHAR8 error_buffer[ZEN_MAX_ERROR_BUFFER_SIZE];
    pcre2_get_error_message_8(error_code, error_buffer, sizeof(error_buffer));

    char full_error[ZEN_MAX_FORMAT_BUFFER_SIZE];
    snprintf(full_error,
             sizeof(full_error),
             "Regex compilation failed: %s (at position %zu in pattern '%s')",
             error_buffer,
             error_offset,
             pattern ? pattern : "null");

    return error_new(full_error);
}

/**
 * @brief Validate input value is a string
 * @param value Value to validate
 * @param param_name Parameter name for error messages
 * @return NULL if valid, error Value if invalid
 */
static Value *validate_string_input(const Value *value, const char *param_name)
{
    if (!value) {
        return error_null_pointer("regex validation");
    }

    if (value->type != VALUE_STRING) {
        char error_msg[128];
        snprintf(error_msg,
                 sizeof(error_msg),
                 "%s must be a string, got %s",
                 param_name,
                 value_type_name(value->type));
        return error_new(error_msg);
    }

    if (!value->as.string || !value->as.string->data) {
        return error_new("String value is null");
    }

    return NULL;  // Valid
}

/**
 * @brief Match text against regular expression pattern
 * @param text_value String value containing text to match
 * @param pattern_value String value containing regex pattern
 * @return Object with match results or error Value
 */
Value *regex_match(const Value *text_value, const Value *pattern_value)
{
    // Validate inputs
    Value *error = validate_string_input(text_value, "text");
    if (error)
        return error;

    error = validate_string_input(pattern_value, "pattern");
    if (error)
        return error;

    const char *text = text_value->as.string->data;
    const char *pattern = pattern_value->as.string->data;

    // Compile pattern
    RegexCache *regex_cache = compile_pattern_cached(pattern);
    if (!regex_cache) {
        int error_code;
        PCRE2_SIZE error_offset;
        pcre2_compile_8((PCRE2_SPTR8)pattern,
                        PCRE2_ZERO_TERMINATED,
                        ZEN_PCRE2_OPTIONS,
                        &error_code,
                        &error_offset,
                        NULL);
        return get_pcre2_error_message(error_code, error_offset, pattern);
    }

    // Perform match
    int match_count = pcre2_match_8(regex_cache->compiled_pattern,
                                    (PCRE2_SPTR8)text,
                                    strlen(text),
                                    0,  // Start offset
                                    0,  // Options
                                    regex_cache->match_data,
                                    NULL  // Context
    );

    // Create result object
    Value *result = object_new();
    if (!result) {
        return error_memory_allocation();
    }

    if (match_count >= 0) {
        // Match found
        object_set(result, "matched", value_new_boolean(true));

        // Get match groups
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer_8(regex_cache->match_data);
        Value *matches = array_new(4);

        for (int i = 0; i < match_count; i++) {
            PCRE2_SIZE start = ovector[2 * i];
            PCRE2_SIZE end = ovector[2 * i + 1];

            if (start != PCRE2_UNSET && end != PCRE2_UNSET) {
                size_t length = end - start;
                char *match_str = memory_alloc(length + 1);
                if (match_str) {
                    memcpy(match_str, text + start, length);
                    match_str[length] = '\0';

                    Value *match_obj = object_new();
                    object_set(match_obj, "text", value_new_string(match_str));
                    object_set(match_obj, "start", value_new_number((double)start));
                    object_set(match_obj, "end", value_new_number((double)end));

                    array_push(matches, match_obj);
                    memory_free(match_str);
                }
            }
        }

        object_set(result, "matches", matches);
        object_set(result, "count", value_new_number((double)match_count));
    } else if (match_count == PCRE2_ERROR_NOMATCH) {
        // No match
        object_set(result, "matched", value_new_boolean(false));
        object_set(result, "matches", array_new(0));
        object_set(result, "count", value_new_number(0));
    } else {
        // Error occurred
        value_free(result);
        PCRE2_UCHAR8 error_buffer[ZEN_MAX_ERROR_BUFFER_SIZE];
        pcre2_get_error_message_8(match_count, error_buffer, sizeof(error_buffer));
        return error_new((char *)error_buffer);
    }

    return result;
}

/**
 * @brief Replace pattern matches in text with replacement
 * @param text_value String value containing text
 * @param pattern_value String value containing regex pattern
 * @param replacement_value String value containing replacement text
 * @return New string with replacements or error Value
 */
Value *
regex_replace(const Value *text_value, const Value *pattern_value, const Value *replacement_value)
{
    // Validate inputs
    Value *error = validate_string_input(text_value, "text");
    if (error)
        return error;

    error = validate_string_input(pattern_value, "pattern");
    if (error)
        return error;

    error = validate_string_input(replacement_value, "replacement");
    if (error)
        return error;

    const char *text = text_value->as.string->data;
    const char *pattern = pattern_value->as.string->data;
    const char *replacement = replacement_value->as.string->data;

    // Compile pattern
    RegexCache *regex_cache = compile_pattern_cached(pattern);
    if (!regex_cache) {
        int error_code;
        PCRE2_SIZE error_offset;
        pcre2_compile_8((PCRE2_SPTR8)pattern,
                        PCRE2_ZERO_TERMINATED,
                        ZEN_PCRE2_OPTIONS,
                        &error_code,
                        &error_offset,
                        NULL);
        return get_pcre2_error_message(error_code, error_offset, pattern);
    }

    // Perform replacement
    PCRE2_SIZE output_length = strlen(text) * 2;  // Initial guess
    PCRE2_UCHAR8 *output_buffer = memory_alloc(output_length);
    if (!output_buffer) {
        return error_memory_allocation();
    }

    int result_code = pcre2_substitute_8(regex_cache->compiled_pattern,
                                         (PCRE2_SPTR8)text,
                                         PCRE2_ZERO_TERMINATED,
                                         0,                        // Start offset
                                         PCRE2_SUBSTITUTE_GLOBAL,  // Replace all matches
                                         regex_cache->match_data,
                                         NULL,  // Context
                                         (PCRE2_SPTR8)replacement,
                                         PCRE2_ZERO_TERMINATED,
                                         output_buffer,
                                         &output_length);

    if (result_code < 0) {
        memory_free(output_buffer);
        if (result_code == PCRE2_ERROR_NOMEMORY) {
            // Try with larger buffer
            output_length *= 2;
            output_buffer = memory_alloc(output_length);
            if (!output_buffer) {
                return error_memory_allocation();
            }

            result_code = pcre2_substitute_8(regex_cache->compiled_pattern,
                                             (PCRE2_SPTR8)text,
                                             PCRE2_ZERO_TERMINATED,
                                             0,
                                             PCRE2_SUBSTITUTE_GLOBAL,
                                             regex_cache->match_data,
                                             NULL,
                                             (PCRE2_SPTR8)replacement,
                                             PCRE2_ZERO_TERMINATED,
                                             output_buffer,
                                             &output_length);
        }

        if (result_code < 0) {
            memory_free(output_buffer);
            PCRE2_UCHAR8 error_buffer[ZEN_MAX_ERROR_BUFFER_SIZE];
            pcre2_get_error_message_8(result_code, error_buffer, sizeof(error_buffer));
            return error_new((char *)error_buffer);
        }
    }

    // Create result string
    Value *result = value_new_string((char *)output_buffer);
    memory_free(output_buffer);

    return result;
}

/**
 * @brief Split text by regular expression pattern
 * @param text_value String value containing text to split
 * @param pattern_value String value containing regex pattern
 * @return Array of string parts or error Value
 */
Value *regex_split(const Value *text_value, const Value *pattern_value)
{
    // Validate inputs
    Value *error = validate_string_input(text_value, "text");
    if (error)
        return error;

    error = validate_string_input(pattern_value, "pattern");
    if (error)
        return error;

    const char *text = text_value->as.string->data;
    const char *pattern = pattern_value->as.string->data;
    size_t text_length = strlen(text);

    // Compile pattern
    RegexCache *regex_cache = compile_pattern_cached(pattern);
    if (!regex_cache) {
        int error_code;
        PCRE2_SIZE error_offset;
        pcre2_compile_8((PCRE2_SPTR8)pattern,
                        PCRE2_ZERO_TERMINATED,
                        ZEN_PCRE2_OPTIONS,
                        &error_code,
                        &error_offset,
                        NULL);
        return get_pcre2_error_message(error_code, error_offset, pattern);
    }

    Value *result_array = array_new(8);
    if (!result_array) {
        return error_memory_allocation();
    }

    size_t offset = 0;
    size_t last_end = 0;

    while (offset <= text_length) {
        int match_count = pcre2_match_8(regex_cache->compiled_pattern,
                                        (PCRE2_SPTR8)text,
                                        text_length,
                                        offset,
                                        0,
                                        regex_cache->match_data,
                                        NULL);

        if (match_count >= 0) {
            PCRE2_SIZE *ovector = pcre2_get_ovector_pointer_8(regex_cache->match_data);
            PCRE2_SIZE match_start = ovector[0];
            PCRE2_SIZE match_end = ovector[1];

            // Add text before match
            if (match_start > last_end) {
                size_t part_length = match_start - last_end;
                char *part_str = memory_alloc(part_length + 1);
                if (part_str) {
                    memcpy(part_str, text + last_end, part_length);
                    part_str[part_length] = '\0';
                    array_push(result_array, value_new_string(part_str));
                    memory_free(part_str);
                }
            }

            last_end = match_end;
            offset = match_end;

            // Prevent infinite loop on zero-width matches
            if (match_start == match_end) {
                offset++;
            }
        } else if (match_count == PCRE2_ERROR_NOMATCH) {
            // No more matches - add remaining text
            if (last_end < text_length) {
                size_t remaining_length = text_length - last_end;
                char *remaining_str = memory_alloc(remaining_length + 1);
                if (remaining_str) {
                    memcpy(remaining_str, text + last_end, remaining_length);
                    remaining_str[remaining_length] = '\0';
                    array_push(result_array, value_new_string(remaining_str));
                    memory_free(remaining_str);
                }
            }
            break;
        } else {
            // Error occurred
            value_free(result_array);
            PCRE2_UCHAR8 error_buffer[ZEN_MAX_ERROR_BUFFER_SIZE];
            pcre2_get_error_message_8(match_count, error_buffer, sizeof(error_buffer));
            return error_new((char *)error_buffer);
        }
    }

    // If no matches were found, return array with original text
    if (array_length(result_array) == 0) {
        array_push(result_array, value_new_string(text));
    }

    return result_array;
}

/**
 * @brief Compile regular expression pattern for reuse
 * @param pattern_value String value containing regex pattern
 * @return Compiled pattern object or error Value
 */
Value *regex_compile(const Value *pattern_value)
{
    // Validate input
    Value *error = validate_string_input(pattern_value, "pattern");
    if (error)
        return error;

    const char *pattern = pattern_value->as.string->data;

    // Compile and cache pattern
    RegexCache *regex_cache = compile_pattern_cached(pattern);
    if (!regex_cache) {
        int error_code;
        PCRE2_SIZE error_offset;
        pcre2_compile_8((PCRE2_SPTR8)pattern,
                        PCRE2_ZERO_TERMINATED,
                        ZEN_PCRE2_OPTIONS,
                        &error_code,
                        &error_offset,
                        NULL);
        return get_pcre2_error_message(error_code, error_offset, pattern);
    }

    // Create result object with pattern info
    Value *result = object_new();
    if (!result) {
        return error_memory_allocation();
    }

    object_set(result, "pattern", value_new_string(pattern));
    object_set(result, "compiled", value_new_boolean(true));

    // Get pattern info
    uint32_t capture_count;
    pcre2_pattern_info_8(regex_cache->compiled_pattern, PCRE2_INFO_CAPTURECOUNT, &capture_count);
    object_set(result, "capture_count", value_new_number((double)capture_count));

    uint32_t options;
    pcre2_pattern_info_8(regex_cache->compiled_pattern, PCRE2_INFO_ALLOPTIONS, &options);
    object_set(result, "utf8", value_new_boolean((options & PCRE2_UTF) != 0));
    object_set(result, "multiline", value_new_boolean((options & PCRE2_MULTILINE) != 0));

    return result;
}

/**
 * @brief Cleanup regex cache on shutdown
 * This function should be called during program shutdown
 */
void regex_cleanup(void)
{
    if (pattern_cache) {
        for (size_t i = 0; i < cache_size; i++) {
            if (pattern_cache[i].pattern_string) {
                memory_free(pattern_cache[i].pattern_string);
            }
            if (pattern_cache[i].match_data) {
                pcre2_match_data_free_8(pattern_cache[i].match_data);
            }
            if (pattern_cache[i].compiled_pattern) {
                pcre2_code_free_8(pattern_cache[i].compiled_pattern);
            }
        }
        memory_free(pattern_cache);
        pattern_cache = NULL;
        cache_size = 0;
        cache_capacity = 0;
    }
}

// Stdlib wrapper functions (for ZEN runtime integration)

/**
 * @brief Regex match stdlib wrapper
 * @param args Arguments array: [text_string, pattern_string]
 * @param argc Number of arguments
 * @return Match results object or error value
 */
Value *regex_match_stdlib(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("regexMatch requires exactly 2 arguments: text and pattern");
    }
    return regex_match(args[0], args[1]);
}

/**
 * @brief Regex replace stdlib wrapper
 * @param args Arguments array: [text_string, pattern_string, replacement_string]
 * @param argc Number of arguments
 * @return New string with replacements or error value
 */
Value *regex_replace_stdlib(Value **args, size_t argc)
{
    if (argc != 3) {
        return error_new(
            "regexReplace requires exactly 3 arguments: text, pattern, and replacement");
    }
    return regex_replace(args[0], args[1], args[2]);
}

/**
 * @brief Regex split stdlib wrapper
 * @param args Arguments array: [text_string, pattern_string]
 * @param argc Number of arguments
 * @return Array of string parts or error value
 */
Value *regex_split_stdlib(Value **args, size_t argc)
{
    if (argc != 2) {
        return error_new("regexSplit requires exactly 2 arguments: text and pattern");
    }
    return regex_split(args[0], args[1]);
}

/**
 * @brief Regex compile stdlib wrapper
 * @param args Arguments array: [pattern_string]
 * @param argc Number of arguments
 * @return Compiled pattern object or error value
 */
Value *regex_compile_stdlib(Value **args, size_t argc)
{
    if (argc != 1) {
        return error_new("regexCompile requires exactly 1 argument: pattern");
    }
    return regex_compile(args[0]);
}