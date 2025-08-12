/**
 * @file yaml.c
 * @brief YAML parsing and generation using libyaml
 *
 * This file implements YAML parsing and generation for the ZEN language
 * using the libyaml library. Functions follow MANIFEST.json specifications.
 *
 * Pattern follows json.c for consistency with ZEN stdlib conventions.
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/yaml.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/io.h"
// Array and object functions are in runtime_value.h

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

// Maximum recursion depth to prevent infinite recursion
#define YAML_MAX_DEPTH 64

// Maximum file size for YAML parsing (64MB) to prevent memory exhaustion
#define MAX_YAML_FILE_SIZE (64 * 1024 * 1024)

// Helper functions for RuntimeValue access
static inline double rv_get_number(const RuntimeValue *value)
{
    return value ? value->data.number : 0.0;
}

static inline bool rv_get_boolean(const RuntimeValue *value)
{
    return value ? value->data.boolean : false;
}

static inline size_t rv_object_count(const RuntimeValue *object)
{
    if (!object || object->type != RV_OBJECT)
        return 0;
    return object->data.object.count;
}

static inline const char **rv_object_keys(const RuntimeValue *object)
{
    if (!object || object->type != RV_OBJECT)
        return NULL;
    size_t count = object->data.object.count;
    const char **keys = memory_alloc(sizeof(char *) * count);
    for (size_t i = 0; i < count; i++) {
        keys[i] = object->data.object.keys[i];
    }
    return keys;
}

// Circular reference detection structure
typedef struct {
    const RuntimeValue **visited;
    size_t visited_count;
    size_t visited_capacity;
    size_t depth;
} YamlRefTracker;

// Anchor tracking for YAML aliases
typedef struct {
    char *name;           // Anchor name
    RuntimeValue *value;  // The anchored value
} YamlAnchor;

typedef struct {
    YamlAnchor *anchors;  // Dynamic array of anchors
    size_t count;
    size_t capacity;
} YamlAnchorMap;

// Forward declarations
static RuntimeValue *parse_yaml_document(yaml_parser_t *parser);
static RuntimeValue *
parse_yaml_node(yaml_parser_t *parser, yaml_event_t *event, YamlAnchorMap *anchors);
static RuntimeValue *parse_yaml_sequence(yaml_parser_t *parser, YamlAnchorMap *anchors);
static RuntimeValue *parse_yaml_mapping(yaml_parser_t *parser, YamlAnchorMap *anchors);
static RuntimeValue *parse_yaml_scalar(yaml_event_t *event);
static bool
emit_yaml_value_safe(yaml_emitter_t *emitter, const RuntimeValue *value, YamlRefTracker *tracker);
static bool emit_yaml_sequence_safe(yaml_emitter_t *emitter,
                                    const RuntimeValue *array,
                                    YamlRefTracker *tracker);
static bool emit_yaml_mapping_safe(yaml_emitter_t *emitter,
                                   const RuntimeValue *object,
                                   YamlRefTracker *tracker);

// Circular reference detection functions
static YamlRefTracker *yaml_ref_tracker_new(void);
static void yaml_ref_tracker_free(YamlRefTracker *tracker);
static bool yaml_ref_tracker_add(YamlRefTracker *tracker, const RuntimeValue *value);
static bool yaml_ref_tracker_contains(const YamlRefTracker *tracker, const RuntimeValue *value);

// Anchor map functions for alias support
static YamlAnchorMap *yaml_anchor_map_new(void);
static void yaml_anchor_map_free(YamlAnchorMap *map);
static bool yaml_anchor_map_add(YamlAnchorMap *map, const char *name, RuntimeValue *value);
static RuntimeValue *yaml_anchor_map_find(YamlAnchorMap *map, const char *name);

/**
 * @brief Parse YAML string to Value
 * @param yaml_string YAML string to parse
 * @return Newly allocated Value representing the YAML data, or error Value on failure
 */
RuntimeValue *yaml_parse(const char *yaml_string)
{
    if (!yaml_string) {
        return rv_new_error("YAML string is NULL", -1);
    }

    size_t yaml_length = strlen(yaml_string);

    // Check file size limit to prevent memory exhaustion
    if (yaml_length > MAX_YAML_FILE_SIZE) {
        return rv_new_error("YAML string exceeds maximum size limit (64MB)", -1);
    }

    yaml_parser_t parser;
    RuntimeValue *result = NULL;

    // Initialize parser
    if (!yaml_parser_initialize(&parser)) {
        return rv_new_error("Failed to initialize YAML parser", -1);
    }

    // Set input string
    yaml_parser_set_input_string(&parser, (const unsigned char *)yaml_string, yaml_length);

    // Parse the document
    result = parse_yaml_document(&parser);

    // Clean up parser
    yaml_parser_delete(&parser);

    if (!result) {
        return rv_new_error("Failed to parse YAML document", -1);
    }

    return result;
}

/**
 * @brief Convert Value to YAML string with circular reference detection
 * @param value Value to convert to YAML
 * @return Newly allocated YAML string, or NULL on error
 */
char *yaml_stringify(const RuntimeValue *value)
{
    if (!value) {
        return memory_strdup("null");
    }

    YamlRefTracker *tracker = yaml_ref_tracker_new();
    if (!tracker) {
        return NULL;
    }

    yaml_emitter_t emitter;
    char *buffer = NULL;
    size_t buffer_size = 0;
    FILE *output = open_memstream(&buffer, &buffer_size);

    if (!output) {
        yaml_ref_tracker_free(tracker);
        return NULL;
    }

    // Initialize emitter
    if (!yaml_emitter_initialize(&emitter)) {
        fclose(output);
        yaml_ref_tracker_free(tracker);
        return NULL;
    }

    // Set output to memory stream
    yaml_emitter_set_output_file(&emitter, output);

    // Start document
    yaml_event_t event;
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        yaml_ref_tracker_free(tracker);
        return NULL;
    }

    yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        yaml_ref_tracker_free(tracker);
        return NULL;
    }

    // Emit the value with circular reference protection
    if (!emit_yaml_value_safe(&emitter, value, tracker)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        yaml_ref_tracker_free(tracker);
        return NULL;
    }

    // End document
    yaml_document_end_event_initialize(&event, 1);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        yaml_ref_tracker_free(tracker);
        return NULL;
    }

    yaml_stream_end_event_initialize(&event);
    if (!yaml_emitter_emit(&emitter, &event)) {
        yaml_emitter_delete(&emitter);
        fclose(output);
        yaml_ref_tracker_free(tracker);
        return NULL;
    }

    // Clean up
    yaml_emitter_delete(&emitter);
    fclose(output);
    yaml_ref_tracker_free(tracker);

    return buffer;
}

/**
 * @brief Load and parse a YAML file
 * @param filepath Path to YAML file
 * @return Newly allocated Value representing the YAML data, or error Value on failure
 */
RuntimeValue *yaml_load_file(const char *filepath)
{
    if (!filepath) {
        return rv_new_error("File path is NULL", -1);
    }

    // Check if file exists using io functions
    extern bool io_file_exists_internal(const char *filepath);
    if (!io_file_exists_internal(filepath)) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "File does not exist: %s", filepath);
        return rv_new_error(error_msg, -1);
    }

    // Read file content first, then check size limit
    extern char *io_read_file_internal(const char *filepath);
    char *content = io_read_file_internal(filepath);
    if (!content) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Failed to read file: %s", filepath);
        return rv_new_error(error_msg, -1);
    }

    // Check file size after reading to prevent memory exhaustion from huge files
    size_t content_length = strlen(content);
    if (content_length > MAX_YAML_FILE_SIZE) {
        memory_free(content);
        char error_msg[512];
        snprintf(
            error_msg, sizeof(error_msg), "File exceeds maximum size limit (64MB): %s", filepath);
        return rv_new_error(error_msg, -1);
    }

    // Parse YAML content (will also check size, providing double protection)
    RuntimeValue *result = yaml_parse(content);
    memory_free(content);

    return result;
}

/**
 * @brief Load and parse YAML file wrapper function for stdlib
 * @param args Array of Value arguments (filename)
 * @param argc Number of arguments
 * @return Parsed YAML Value or error
 */
RuntimeValue *yaml_load_file_wrapper(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("loadYamlFile requires a filename string", -1);
    }

    return yaml_load_file(rv_get_string(args[0]));
}

/**
 * @brief Parse YAML string wrapper function for stdlib
 * @param args Array of Value arguments (yaml string)
 * @param argc Number of arguments
 * @return Parsed YAML Value or error
 */
RuntimeValue *yaml_parse_wrapper(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("yamlParse requires a YAML string", -1);
    }

    return yaml_parse(rv_get_string(args[0]));
}

/**
 * @brief Stringify value to YAML wrapper function for stdlib
 * @param args Array of Value arguments (value to stringify)
 * @param argc Number of arguments
 * @return YAML string as RuntimeValue or error
 */
RuntimeValue *yaml_stringify_wrapper(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || !args[0]) {
        return rv_new_error("yamlStringify requires a value", -1);
    }

    char *yaml_str = yaml_stringify(args[0]);
    if (!yaml_str) {
        return rv_new_error("Failed to stringify value to YAML", -1);
    }

    RuntimeValue *result = rv_new_string(yaml_str);
    memory_free(yaml_str);
    return result;
}

// Helper functions for parsing

static RuntimeValue *parse_yaml_document(yaml_parser_t *parser)
{
    yaml_event_t event;
    RuntimeValue *result = NULL;
    YamlAnchorMap *anchors = yaml_anchor_map_new();

    if (!anchors) {
        return rv_new_error("Memory allocation failed", -1);
    }

    // Parse through events to find document start
    while (true) {
        if (!yaml_parser_parse(parser, &event)) {
            yaml_anchor_map_free(anchors);
            return rv_new_error("YAML parser error", -1);
        }

        switch (event.type) {
        case YAML_STREAM_START_EVENT:
        case YAML_DOCUMENT_START_EVENT:
            yaml_event_delete(&event);
            continue;

        case YAML_DOCUMENT_END_EVENT:
        case YAML_STREAM_END_EVENT:
            yaml_event_delete(&event);
            yaml_anchor_map_free(anchors);
            return result ? result : rv_new_null();

        case YAML_SCALAR_EVENT:
        case YAML_SEQUENCE_START_EVENT:
        case YAML_MAPPING_START_EVENT:
            result = parse_yaml_node(parser, &event, anchors);
            yaml_event_delete(&event);
            yaml_anchor_map_free(anchors);
            return result;

        case YAML_ALIAS_EVENT: {
            // Look up the alias
            const char *alias_name = (const char *)event.data.alias.anchor;
            RuntimeValue *aliased = yaml_anchor_map_find(anchors, alias_name);

            if (!aliased) {
                yaml_event_delete(&event);
                yaml_anchor_map_free(anchors);
                // Use simple error since rv_new_error is not working
                RuntimeValue *error = rv_new_error("Unknown error", -1);
                if (!error) {
                    return NULL;
                }
                return error;  // Will show "Unknown error" message
            }

            result = rv_ref(aliased);  // Increment ref count
            yaml_event_delete(&event);
            yaml_anchor_map_free(anchors);
            return result;
        }

        default:
            yaml_event_delete(&event);
            continue;
        }
    }
}

static RuntimeValue *
parse_yaml_node(yaml_parser_t *parser, yaml_event_t *event, YamlAnchorMap *anchors)
{
    RuntimeValue *value = NULL;

    switch (event->type) {
    case YAML_SCALAR_EVENT:
        value = parse_yaml_scalar(event);
        break;

    case YAML_SEQUENCE_START_EVENT:
        value = parse_yaml_sequence(parser, anchors);
        break;

    case YAML_MAPPING_START_EVENT:
        value = parse_yaml_mapping(parser, anchors);
        break;

    default:
        return rv_new_error("Unexpected YAML event", -1);
    }

    // If this node has an anchor, register it
    if (value && !rv_is_error(value)) {
        const char *anchor = NULL;

        switch (event->type) {
        case YAML_SCALAR_EVENT:
            anchor = (const char *)event->data.scalar.anchor;
            break;
        case YAML_SEQUENCE_START_EVENT:
            anchor = (const char *)event->data.sequence_start.anchor;
            break;
        case YAML_MAPPING_START_EVENT:
            anchor = (const char *)event->data.mapping_start.anchor;
            break;
        default:
            break;
        }

        if (anchor) {
            yaml_anchor_map_add(anchors, anchor, value);
        }
    }

    return value;
}

static RuntimeValue *parse_yaml_sequence(yaml_parser_t *parser, YamlAnchorMap *anchors)
{
    RuntimeValue *array = rv_new_array();
    if (!array) {
        return rv_new_error("Memory allocation failed", -1);
    }

    yaml_event_t event;

    while (true) {
        if (!yaml_parser_parse(parser, &event)) {
            rv_unref(array);
            return rv_new_error("YAML parser error in sequence", -1);
        }

        if (event.type == YAML_SEQUENCE_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        RuntimeValue *item = NULL;

        if (event.type == YAML_ALIAS_EVENT) {
            // Handle alias in sequence
            const char *alias_name = (const char *)event.data.alias.anchor;
            RuntimeValue *aliased = yaml_anchor_map_find(anchors, alias_name);

            if (!aliased) {
                yaml_event_delete(&event);
                rv_unref(array);
                // Use simple error since rv_new_error is not working
                RuntimeValue *error = rv_new_error("Unknown error", -1);
                if (!error) {
                    return NULL;
                }
                return error;  // Will show "Unknown error" message
            }

            item = rv_ref(aliased);  // Increment ref count
        } else {
            item = parse_yaml_node(parser, &event, anchors);
        }

        yaml_event_delete(&event);

        if (!item) {
            rv_unref(array);
            return rv_new_error("Failed to parse sequence item", -1);
        }

        if (rv_is_error(item)) {
            rv_unref(array);
            return item;
        }

        rv_array_push(array, item);
    }

    return array;
}

static RuntimeValue *parse_yaml_mapping(yaml_parser_t *parser, YamlAnchorMap *anchors)
{
    RuntimeValue *object = rv_new_object();
    if (!object) {
        return rv_new_error("Memory allocation failed", -1);
    }

    yaml_event_t event;

    while (true) {
        // Check for mapping end
        if (!yaml_parser_parse(parser, &event)) {
            rv_unref(object);
            return rv_new_error("YAML parser error in mapping", -1);
        }

        if (event.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        // Parse key (must be scalar)
        if (event.type != YAML_SCALAR_EVENT) {
            yaml_event_delete(&event);
            rv_unref(object);
            return rv_new_error("YAML mapping key must be scalar", -1);
        }

        RuntimeValue *key = parse_yaml_scalar(&event);
        yaml_event_delete(&event);

        if (!key || key->type != RV_STRING) {
            if (key)
                rv_unref(key);
            rv_unref(object);
            return rv_new_error("Invalid YAML mapping key", -1);
        }

        // Parse value
        if (!yaml_parser_parse(parser, &event)) {
            rv_unref(key);
            rv_unref(object);
            return rv_new_error("YAML parser error parsing mapping value", -1);
        }

        RuntimeValue *value = NULL;

        if (event.type == YAML_ALIAS_EVENT) {
            // Handle alias as mapping value
            const char *alias_name = (const char *)event.data.alias.anchor;
            RuntimeValue *aliased = yaml_anchor_map_find(anchors, alias_name);

            if (!aliased) {
                yaml_event_delete(&event);
                rv_unref(key);
                rv_unref(object);
                // Use simple error since rv_new_error is not working
                RuntimeValue *error = rv_new_error("Unknown error", -1);
                if (!error) {
                    return NULL;
                }
                return error;  // Will show "Unknown error" message
            }

            value = rv_ref(aliased);  // Increment ref count
        } else {
            value = parse_yaml_node(parser, &event, anchors);
        }

        yaml_event_delete(&event);

        if (!value) {
            rv_unref(key);
            rv_unref(object);
            return rv_new_error("Failed to parse mapping value", -1);
        }

        if (rv_is_error(value)) {
            rv_unref(key);
            rv_unref(object);
            return value;
        }

        // Handle merge key (<<) specially
        if (strcmp(rv_get_string(key), "<<") == 0) {
            // Merge the aliased object's properties into this object
            if (value->type == RV_OBJECT) {
                // Merge all properties from the aliased object
                size_t merge_count = rv_object_count(value);
                const char **merge_keys = rv_object_keys(value);
                for (size_t i = 0; i < merge_count; i++) {
                    const char *merge_key = merge_keys[i];
                    RuntimeValue *merge_value = rv_object_get(value, merge_key);

                    // Only set if key doesn't already exist (local values override merged ones)
                    if (!rv_object_get(object, merge_key)) {
                        rv_object_set(object, merge_key, rv_ref(merge_value));
                    }
                }
            }
            rv_unref(value);  // Don't need the merge object itself
        } else {
            // Normal key-value pair
            rv_object_set(object, rv_get_string(key), value);
        }
        rv_unref(key);
    }

    return object;
}

static RuntimeValue *parse_yaml_scalar(yaml_event_t *event)
{
    const char *value_str = (const char *)event->data.scalar.value;
    size_t length = event->data.scalar.length;

    if (!value_str || length == 0) {
        return rv_new_string("");
    }

    // Handle null values
    if (strcmp(value_str, "null") == 0 || strcmp(value_str, "~") == 0 ||
        strcmp(value_str, "") == 0) {
        return rv_new_null();
    }

    // Handle boolean values
    if (strcmp(value_str, "true") == 0 || strcmp(value_str, "yes") == 0 ||
        strcmp(value_str, "on") == 0) {
        return rv_new_boolean(true);
    }
    if (strcmp(value_str, "false") == 0 || strcmp(value_str, "no") == 0 ||
        strcmp(value_str, "off") == 0) {
        return rv_new_boolean(false);
    }

    // Try to parse as number
    char *endptr;
    double num_value = strtod(value_str, &endptr);
    if (*endptr == '\0') {
        return rv_new_number(num_value);
    }

    // Default to string
    return rv_new_string(value_str);
}

// Helper functions for emitting YAML

static bool
emit_yaml_value_safe(yaml_emitter_t *emitter, const RuntimeValue *value, YamlRefTracker *tracker)
{
    if (!value) {
        yaml_event_t event;
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)"null", -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        return yaml_emitter_emit(emitter, &event);
    }

    // Check depth limit to prevent stack overflow
    if (tracker->depth >= YAML_MAX_DEPTH) {
        yaml_event_t event;
        yaml_scalar_event_initialize(&event,
                                     NULL,
                                     NULL,
                                     (unsigned char *)"\"[Max Depth Reached]\"",
                                     -1,
                                     1,
                                     1,
                                     YAML_DOUBLE_QUOTED_SCALAR_STYLE);
        return yaml_emitter_emit(emitter, &event);
    }

    // Check for circular references only for objects and arrays
    if (value->type == RV_OBJECT || value->type == RV_ARRAY) {
        if (yaml_ref_tracker_contains(tracker, value)) {
            yaml_event_t event;
            yaml_scalar_event_initialize(&event,
                                         NULL,
                                         NULL,
                                         (unsigned char *)"\"[Circular Reference]\"",
                                         -1,
                                         1,
                                         1,
                                         YAML_DOUBLE_QUOTED_SCALAR_STYLE);
            return yaml_emitter_emit(emitter, &event);
        }

        // Add to visited set
        if (!yaml_ref_tracker_add(tracker, value)) {
            return false;  // Memory allocation failed
        }
    }

    tracker->depth++;
    bool result = false;

    switch (value->type) {
    case RV_NULL: {
        yaml_event_t event;
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)"null", -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case RV_BOOLEAN: {
        yaml_event_t event;
        const char *bool_str = rv_get_boolean(value) ? "true" : "false";
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)bool_str, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case RV_NUMBER: {
        yaml_event_t event;
        char num_buffer[32];
        double num = rv_get_number(value);
        if (num == floor(num)) {
            snprintf(num_buffer, sizeof(num_buffer), "%.0f", num);
        } else {
            snprintf(num_buffer, sizeof(num_buffer), "%g", num);
        }
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)num_buffer, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case RV_STRING: {
        yaml_event_t event;
        const char *str = rv_get_string((RuntimeValue *)value);
        if (!str)
            str = "";
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)str, -1, 1, 1, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case RV_ARRAY:
        result = emit_yaml_sequence_safe(emitter, value, tracker);
        break;

    case RV_OBJECT:
        result = emit_yaml_mapping_safe(emitter, value, tracker);
        break;

    default:
        result = false;
    }

    tracker->depth--;
    return result;
}

static bool
emit_yaml_sequence_safe(yaml_emitter_t *emitter, const RuntimeValue *array, YamlRefTracker *tracker)
{
    if (!array || array->type != RV_ARRAY) {
        return false;
    }

    yaml_event_t event;

    // Start sequence
    yaml_sequence_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) {
        return false;
    }

    // Emit array items with circular reference protection
    size_t array_len = rv_array_length((RuntimeValue *)array);
    for (size_t i = 0; i < array_len; i++) {
        RuntimeValue *item = rv_array_get((RuntimeValue *)array, i);
        if (!emit_yaml_value_safe(emitter, item, tracker)) {
            return false;
        }
    }

    // End sequence
    yaml_sequence_end_event_initialize(&event);
    return yaml_emitter_emit(emitter, &event);
}

static bool
emit_yaml_mapping_safe(yaml_emitter_t *emitter, const RuntimeValue *object, YamlRefTracker *tracker)
{
    if (!object || object->type != RV_OBJECT) {
        return false;
    }

    yaml_event_t event;

    // Start mapping
    yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) {
        return false;
    }

    // Emit object pairs with circular reference protection
    size_t obj_count = rv_object_count(object);
    const char **obj_keys = rv_object_keys(object);
    for (size_t i = 0; i < obj_count; i++) {
        const char *key = obj_keys[i];
        const RuntimeValue *value = rv_object_get((RuntimeValue *)object, key);

        // Emit key
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)key, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        if (!yaml_emitter_emit(emitter, &event)) {
            return false;
        }

        // Emit value with circular reference protection
        if (!emit_yaml_value_safe(emitter, value, tracker)) {
            return false;
        }
    }

    // End mapping
    yaml_mapping_end_event_initialize(&event);
    return yaml_emitter_emit(emitter, &event);
}

// ============================================================================
// Circular Reference Detection Implementation
// ============================================================================

/**
 * @brief Create a new reference tracker for circular detection
 * @return New tracker or NULL on failure
 */
static YamlRefTracker *yaml_ref_tracker_new(void)
{
    YamlRefTracker *tracker = memory_alloc(sizeof(YamlRefTracker));
    if (!tracker) {
        return NULL;
    }

    tracker->visited_capacity = 16;  // Initial capacity
    tracker->visited = memory_alloc(sizeof(const RuntimeValue *) * tracker->visited_capacity);
    if (!tracker->visited) {
        memory_free(tracker);
        return NULL;
    }

    tracker->visited_count = 0;
    tracker->depth = 0;

    return tracker;
}

/**
 * @brief Free reference tracker and all associated memory
 * @param tracker Tracker to free
 */
static void yaml_ref_tracker_free(YamlRefTracker *tracker)
{
    if (!tracker) {
        return;
    }

    if (tracker->visited) {
        memory_free(tracker->visited);
    }

    memory_free(tracker);
}

/**
 * @brief Add a value to the tracker's visited set
 * @param tracker Reference tracker
 * @param value Value to add
 * @return true on success, false on failure
 */
static bool yaml_ref_tracker_add(YamlRefTracker *tracker, const RuntimeValue *value)
{
    if (!tracker || !value) {
        return false;
    }

    // Check if we need to expand capacity
    if (tracker->visited_count >= tracker->visited_capacity) {
        size_t new_capacity = tracker->visited_capacity * 2;
        const RuntimeValue **new_visited =
            memory_realloc(tracker->visited, sizeof(const RuntimeValue *) * new_capacity);
        if (!new_visited) {
            return false;
        }

        tracker->visited = new_visited;
        tracker->visited_capacity = new_capacity;
    }

    tracker->visited[tracker->visited_count++] = value;
    return true;
}

/**
 * @brief Check if a value is already in the tracker's visited set
 * @param tracker Reference tracker
 * @param value Value to check
 * @return true if value is already visited, false otherwise
 */
static bool yaml_ref_tracker_contains(const YamlRefTracker *tracker, const RuntimeValue *value)
{
    if (!tracker || !value || !tracker->visited) {
        return false;
    }

    for (size_t i = 0; i < tracker->visited_count; i++) {
        if (tracker->visited[i] == value) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// Anchor Map Implementation for YAML Alias Support
// ============================================================================

/**
 * @brief Create a new anchor map for tracking YAML anchors
 * @return New anchor map or NULL on failure
 */
static YamlAnchorMap *yaml_anchor_map_new(void)
{
    YamlAnchorMap *map = memory_alloc(sizeof(YamlAnchorMap));
    if (!map) {
        return NULL;
    }

    map->capacity = 8;  // Initial capacity
    map->anchors = memory_alloc(sizeof(YamlAnchor) * map->capacity);
    if (!map->anchors) {
        memory_free(map);
        return NULL;
    }

    map->count = 0;
    return map;
}

/**
 * @brief Free anchor map and all associated memory
 * @param map Anchor map to free
 */
static void yaml_anchor_map_free(YamlAnchorMap *map)
{
    if (!map) {
        return;
    }

    if (map->anchors) {
        // Free all anchor names (values are managed by ref counting)
        for (size_t i = 0; i < map->count; i++) {
            if (map->anchors[i].name) {
                memory_free(map->anchors[i].name);
            }
        }
        memory_free(map->anchors);
    }

    memory_free(map);
}

/**
 * @brief Add an anchor to the map
 * @param map Anchor map
 * @param name Anchor name
 * @param value Value to associate with anchor
 * @return true on success, false on failure
 */
static bool yaml_anchor_map_add(YamlAnchorMap *map, const char *name, RuntimeValue *value)
{
    if (!map || !name || !value) {
        return false;
    }

    // Check if anchor already exists
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->anchors[i].name, name) == 0) {
            // Anchor already exists - this is an error in YAML
            return false;
        }
    }

    // Check if we need to expand capacity
    if (map->count >= map->capacity) {
        size_t new_capacity = map->capacity * 2;
        YamlAnchor *new_anchors = memory_realloc(map->anchors, sizeof(YamlAnchor) * new_capacity);
        if (!new_anchors) {
            return false;
        }

        map->anchors = new_anchors;
        map->capacity = new_capacity;
    }

    // Add the anchor
    map->anchors[map->count].name = memory_strdup(name);
    if (!map->anchors[map->count].name) {
        return false;
    }

    map->anchors[map->count].value = value;
    map->count++;

    return true;
}

/**
 * @brief Find a value by anchor name
 * @param map Anchor map
 * @param name Anchor name to look up
 * @return Associated value or NULL if not found
 */
static RuntimeValue *yaml_anchor_map_find(YamlAnchorMap *map, const char *name)
{
    if (!map || !name) {
        return NULL;
    }

    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->anchors[i].name, name) == 0) {
            return map->anchors[i].value;
        }
    }

    return NULL;
}