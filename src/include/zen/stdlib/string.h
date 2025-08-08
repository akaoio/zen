#ifndef ZEN_STDLIB_STRING_H
#define ZEN_STDLIB_STRING_H

#include "zen/types/value.h"

/**
 * @brief Get the length of a string value
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return Length as a number value, or error on invalid input
 */
Value *string_length(Value **args, size_t argc);

/**
 * @brief Convert string to uppercase
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in uppercase
 */
Value *string_upper(Value **args, size_t argc);

/**
 * @brief Convert string to lowercase
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New string value in lowercase
 */
Value *string_lower(Value **args, size_t argc);

/**
 * @brief Trim whitespace from both ends of string
 * @param args Arguments array containing string value
 * @param argc Number of arguments
 * @return New trimmed string value
 */
Value *string_trim(Value **args, size_t argc);

/**
 * @brief Split string by delimiter
 * @param args Arguments array containing string and delimiter
 * @param argc Number of arguments
 * @return Array value containing split parts
 */
Value *string_split(Value **args, size_t argc);

/**
 * @brief Check if string contains substring
 * @param args Arguments array containing string and substring
 * @param argc Number of arguments
 * @return Boolean value indicating if substring was found
 */
Value *string_contains(Value **args, size_t argc);

/**
 * @brief Replace all occurrences of substring with replacement
 * @param args Arguments array containing string, search, and replace values
 * @param argc Number of arguments
 * @return New string value with replacements made
 */
Value *string_replace(Value **args, size_t argc);

#endif
