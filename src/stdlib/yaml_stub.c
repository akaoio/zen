/**
 * @file yaml_stub.c
 * @brief Stub implementation of YAML functions when libyaml is not available
 *
 * This provides error-returning stubs for YAML functionality so the
 * interpreter can still run without libyaml installed.
 */

#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/yaml.h"

// Stub implementations that return errors

RuntimeValue *yaml_parse(const char *yaml_string)
{
    (void)yaml_string;
    return rv_new_error("YAML support not available - libyaml not installed", -1);
}

char *yaml_stringify(const RuntimeValue *value)
{
    (void)value;
    return memory_strdup("");
}

RuntimeValue *yaml_load_file(const char *filepath)
{
    (void)filepath;
    return rv_new_error("YAML support not available - libyaml not installed", -1);
}

RuntimeValue *yaml_load_file_wrapper(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;
    return rv_new_error("YAML support not available - libyaml not installed", -1);
}