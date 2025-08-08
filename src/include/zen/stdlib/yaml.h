#ifndef ZEN_STDLIB_YAML_H
#define ZEN_STDLIB_YAML_H

#include "zen/types/value.h"

#include <yaml.h>

/**
 * @brief Parse YAML string to Value
 * @param yaml_string YAML string to parse
 * @return Newly allocated Value representing the YAML data, or error Value on failure
 */
Value *yaml_parse(const char *yaml_string);

/**
 * @brief Convert Value to YAML string
 * @param value Value to convert to YAML
 * @return Newly allocated YAML string, or NULL on error
 */
char *yaml_stringify(const Value *value);

/**
 * @brief Load and parse a YAML file
 * @param filepath Path to YAML file
 * @return Newly allocated Value representing the YAML data, or error Value on failure
 */
Value *yaml_load_file(const char *filepath);

/**
 * @brief Load and parse YAML file wrapper function for stdlib
 * @param args Array of Value arguments (filename)
 * @param argc Number of arguments
 * @return Parsed YAML Value or error
 */
Value *yaml_load_file_wrapper(Value **args, size_t argc);

#endif