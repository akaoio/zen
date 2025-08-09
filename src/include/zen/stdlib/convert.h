#ifndef ZEN_STDLIB_CONVERT_H
#define ZEN_STDLIB_CONVERT_H

#include "zen/core/runtime_value.h"

/**
 * @brief Convert value to string
 * @param value Value to convert
 * @return String value representing the input value
 */
RuntimeValue *convert_to_string_internal(const RuntimeValue *value);

/**
 * @brief Convert value to number
 * @param value Value to convert
 * @return Number value representing the input value
 */
RuntimeValue *convert_to_number_internal(const RuntimeValue *value);

/**
 * @brief Convert value to boolean
 * @param value Value to convert
 * @return Boolean value representing the truthiness of input value
 */
RuntimeValue *convert_to_boolean_internal(const RuntimeValue *value);

/**
 * @brief Get the type name of a value
 * @param value Value to get type of
 * @return String value containing the type name
 */
RuntimeValue *convert_type_of_internal(const RuntimeValue *value);

/**
 * @brief Check if value is of specified type
 * @param value Value to check
 * @param type_name Type name to check against
 * @return Boolean value indicating if value is of specified type
 */
RuntimeValue *convert_is_type_internal(const RuntimeValue *value, const char *type_name);

/**
 * @brief Parse integer from string with specified base
 * @param str_value String value to parse
 * @param base_value Base value (2-36, default 10)
 * @return Number value containing parsed integer, or error on failure
 */
RuntimeValue *convert_parse_int_internal(const RuntimeValue *str_value,
                                         const RuntimeValue *base_value);

/**
 * @brief Parse float from string
 * @param str_value String value to parse
 * @return Number value containing parsed float, or error on failure
 */
RuntimeValue *convert_parse_float_internal(const RuntimeValue *str_value);

#endif
