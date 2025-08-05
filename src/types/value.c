/*
 * value.c
 * Value type system
 * 
 * This file is auto-generated from MANIFEST.json
 * DO NOT modify function signatures without updating the manifest
 */

/**
 * @brief Create new value
 * @param type TODO: Describe parameter
 * @return TODO: Describe return value
 */
Value* value_new(ValueType type) {
}

/**
 * @brief Create string value
 * @param str TODO: Describe parameter
 * @return TODO: Describe return value
 */
Value* value_new_string(const char* str) {
}

/**
 * @brief Create number value
 * @param num TODO: Describe parameter
 * @return TODO: Describe return value
 */
Value* value_new_number(double num) {
}

/**
 * @brief Create boolean value
 * @param val TODO: Describe parameter
 * @return TODO: Describe return value
 */
Value* value_new_boolean(bool val) {
}

/**
 * @brief Create null value
 * @return TODO: Describe return value
 */
Value* value_new_null(void) {
}

/**
 * @brief Deep copy value
 * @param value TODO: Describe parameter
 * @return TODO: Describe return value
 */
Value* value_copy(const Value* value) {
}

/**
 * @brief Free value memory
 * @param value TODO: Describe parameter
 */
void value_free(Value* value) {
}

/**
 * @brief Convert value to string representation
 * @param value TODO: Describe parameter
 * @return TODO: Describe return value
 */
char* value_to_string(const Value* value) {
}

/**
 * @brief Compare two values for equality
 * @param a TODO: Describe parameter
 * @param b TODO: Describe parameter
 * @return TODO: Describe return value
 */
bool value_equals(const Value* a, const Value* b) {
}

/**
 * @brief Get string name of value type
 * @param type TODO: Describe parameter
 * @return TODO: Describe return value
 */
const char* value_type_name(ValueType type) {
}

