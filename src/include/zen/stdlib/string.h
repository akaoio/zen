#ifndef ZEN_STDLIB_STRING_H
#define ZEN_STDLIB_STRING_H

#include "zen/core/runtime_value.h"

/**
 * @brief Get the length of a string value
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return Length as a number value, or error on invalid input
 */
RuntimeValue *string_length(RuntimeValue **args, size_t argc);

/**
 * @brief Convert string to uppercase
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in uppercase
 */
RuntimeValue *string_upper(RuntimeValue **args, size_t argc);

/**
 * @brief Convert string to lowercase
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in lowercase
 */
RuntimeValue *string_lower(RuntimeValue **args, size_t argc);

/**
 * @brief Trim whitespace from both ends of string
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New trimmed string value
 */
RuntimeValue *string_trim(RuntimeValue **args, size_t argc);

/**
 * @brief Split string by delimiter
 * @param args Arguments array containing string and delimiter
 * @param argc Number of arguments
 * @return Array value containing split parts
 */
RuntimeValue *string_split(RuntimeValue **args, size_t argc);

/**
 * @brief Check if string contains substring
 * @param args Arguments array containing string and substring
 * @param argc Number of arguments
 * @return Boolean value indicating if substring was found
 */
RuntimeValue *string_contains(RuntimeValue **args, size_t argc);

/**
 * @brief Replace all occurrences of substring with replacement
 * @param args Arguments array containing string, search, and replace values
 * @param argc Number of arguments
 * @return New string value with replacements made
 */
RuntimeValue *string_replace(RuntimeValue **args, size_t argc);

/**
 * @brief Extract substring from string
 * @param args Arguments array containing string, start index, and optional length
 * @param argc Number of arguments (2 or 3)
 * @return New string value containing the substring
 */
RuntimeValue *string_substring(RuntimeValue **args, size_t argc);

/**
 * @brief Find index of substring in string
 * @param args Arguments array containing string and substring
 * @param argc Number of arguments
 * @return Number value with index of substring, or -1 if not found
 */
RuntimeValue *string_index_of(RuntimeValue **args, size_t argc);

/**
 * @brief Concatenate two or more strings
 * @param args Arguments array containing strings to concatenate
 * @param argc Number of arguments
 * @return New string value containing concatenated result
 */
RuntimeValue *string_concat(RuntimeValue **args, size_t argc);

/**
 * @brief Check if string starts with prefix
 * @param args Arguments array containing string and prefix
 * @param argc Number of arguments
 * @return Boolean value indicating if string starts with prefix
 */
RuntimeValue *string_starts_with(RuntimeValue **args, size_t argc);

/**
 * @brief Check if string ends with suffix
 * @param args Arguments array containing string and suffix
 * @param argc Number of arguments
 * @return Boolean value indicating if string ends with suffix
 */
RuntimeValue *string_ends_with(RuntimeValue **args, size_t argc);

/**
 * @brief Convert string to uppercase (alias for string_upper)
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in uppercase
 */
RuntimeValue *string_to_upper(RuntimeValue **args, size_t argc);

/**
 * @brief Convert string to lowercase (alias for string_lower)
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in lowercase
 */
RuntimeValue *string_to_lower(RuntimeValue **args, size_t argc);

#endif
