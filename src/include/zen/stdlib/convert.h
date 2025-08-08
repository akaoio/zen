#ifndef ZEN_STDLIB_CONVERT_H
#define ZEN_STDLIB_CONVERT_H

#include "zen/types/value.h"

/**
 * @brief Convert value to string
 * @param value Value to convert
 * @return String value representing the input value
 */
Value* convert_to_string_internal(const Value* value);

/**
 * @brief Convert value to number
 * @param value Value to convert
 * @return Number value representing the input value
 */
Value* convert_to_number_internal(const Value* value);

/**
 * @brief Convert value to boolean
 * @param value Value to convert
 * @return Boolean value representing the truthiness of input value
 */
Value* convert_to_boolean_internal(const Value* value);

/**
 * @brief Get the type name of a value
 * @param value Value to get type of
 * @return String value containing the type name
 */
Value* convert_type_of_internal(const Value* value);

/**
 * @brief Check if value is of specified type
 * @param value Value to check
 * @param type_name Type name to check against
 * @return Boolean value indicating if value is of specified type
 */
Value* convert_is_type_internal(const Value* value, const char* type_name);

/**
 * @brief Parse integer from string with specified base
 * @param str_value String value to parse
 * @param base_value Base value (2-36, default 10)
 * @return Number value containing parsed integer, or error on failure
 */
Value* convert_parse_int_internal(const Value* str_value, const Value* base_value);

/**
 * @brief Parse float from string
 * @param str_value String value to parse
 * @return Number value containing parsed float, or error on failure
 */
Value* convert_parse_float_internal(const Value* str_value);

#endif
