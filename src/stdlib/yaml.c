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
#include "zen/types/array.h"
#include "zen/types/object.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

// Maximum recursion depth to prevent infinite recursion
#define YAML_MAX_DEPTH 64

// Maximum file size for YAML parsing (64MB) to prevent memory exhaustion
#define MAX_YAML_FILE_SIZE (64 * 1024 * 1024)

// Circular reference detection structure
typedef struct {
    const Value **visited;
    size_t visited_count;
    size_t visited_capacity;
    size_t depth;
} YamlRefTracker;

// Anchor tracking for YAML aliases
typedef struct {
    char *name;    // Anchor name
    Value *value;  // The anchored value
} YamlAnchor;

typedef struct {
    YamlAnchor *anchors;  // Dynamic array of anchors
    size_t count;
    size_t capacity;
} YamlAnchorMap;

// Forward declarations
static Value *parse_yaml_document(yaml_parser_t *parser);
static Value *parse_yaml_node(yaml_parser_t *parser, yaml_event_t *event, YamlAnchorMap *anchors);
static Value *parse_yaml_sequence(yaml_parser_t *parser, YamlAnchorMap *anchors);
static Value *parse_yaml_mapping(yaml_parser_t *parser, YamlAnchorMap *anchors);
static Value *parse_yaml_scalar(yaml_event_t *event);
static bool
emit_yaml_value_safe(yaml_emitter_t *emitter, const Value *value, YamlRefTracker *tracker);
static bool
emit_yaml_sequence_safe(yaml_emitter_t *emitter, const Value *array, YamlRefTracker *tracker);
static bool
emit_yaml_mapping_safe(yaml_emitter_t *emitter, const Value *object, YamlRefTracker *tracker);

// Circular reference detection functions
static YamlRefTracker *yaml_ref_tracker_new(void);
static void yaml_ref_tracker_free(YamlRefTracker *tracker);
static bool yaml_ref_tracker_add(YamlRefTracker *tracker, const Value *value);
static bool yaml_ref_tracker_contains(const YamlRefTracker *tracker, const Value *value);

// Anchor map functions for alias support
static YamlAnchorMap *yaml_anchor_map_new(void);
static void yaml_anchor_map_free(YamlAnchorMap *map);
static bool yaml_anchor_map_add(YamlAnchorMap *map, const char *name, Value *value);
static Value *yaml_anchor_map_find(YamlAnchorMap *map, const char *name);

/**
 * @brief Parse YAML string to Value
 * @param yaml_string YAML string to parse
 * @return Newly allocated Value representing the YAML data, or error Value on failure
 */
Value *yaml_parse(const char *yaml_string)
{
    if (!yaml_string) {
        return error_new("YAML string is NULL");
    }

    size_t yaml_length = strlen(yaml_string);

    // Check file size limit to prevent memory exhaustion
    if (yaml_length > MAX_YAML_FILE_SIZE) {
        return error_new("YAML string exceeds maximum size limit (64MB)");
    }

    yaml_parser_t parser;
    Value *result = NULL;

    // Initialize parser
    if (!yaml_parser_initialize(&parser)) {
        return error_new("Failed to initialize YAML parser");
    }

    // Set input string
    yaml_parser_set_input_string(&parser, (const unsigned char *)yaml_string, yaml_length);

    // Parse the document
    result = parse_yaml_document(&parser);

    // Clean up parser
    yaml_parser_delete(&parser);

    if (!result) {
        return error_new("Failed to parse YAML document");
    }

    return result;
}

/**
 * @brief Convert Value to YAML string with circular reference detection
 * @param value Value to convert to YAML
 * @return Newly allocated YAML string, or NULL on error
 */
char *yaml_stringify(const Value *value)
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
Value *yaml_load_file(const char *filepath)
{
    if (!filepath) {
        return error_new("File path is NULL");
    }

    // Check if file exists using io functions
    extern bool io_file_exists_internal(const char *filepath);
    if (!io_file_exists_internal(filepath)) {
        return error_new_formatted("File does not exist: %s", filepath);
    }

    // Read file content first, then check size limit
    extern char *io_read_file_internal(const char *filepath);
    char *content = io_read_file_internal(filepath);
    if (!content) {
        return error_new_formatted("Failed to read file: %s", filepath);
    }

    // Check file size after reading to prevent memory exhaustion from huge files
    size_t content_length = strlen(content);
    if (content_length > MAX_YAML_FILE_SIZE) {
        memory_free(content);
        return error_new_formatted("File exceeds maximum size limit (64MB): %s", filepath);
    }

    // Parse YAML content (will also check size, providing double protection)
    Value *result = yaml_parse(content);
    memory_free(content);

    return result;
}

/**
 * @brief Load and parse YAML file wrapper function for stdlib
 * @param args Array of Value arguments (filename)
 * @param argc Number of arguments
 * @return Parsed YAML Value or error
 */
Value *yaml_load_file_wrapper(Value **args, size_t argc)
{
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return error_new("loadYamlFile requires a filename string");
    }

    return yaml_load_file(args[0]->as.string->data);
}

// Helper functions for parsing

static Value *parse_yaml_document(yaml_parser_t *parser)
{
    yaml_event_t event;
    Value *result = NULL;
    YamlAnchorMap *anchors = yaml_anchor_map_new();

    if (!anchors) {
        return error_memory_allocation();
    }

    // Parse through events to find document start
    while (true) {
        if (!yaml_parser_parse(parser, &event)) {
            yaml_anchor_map_free(anchors);
            return error_new("YAML parser error");
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
            return result ? result : value_new_null();

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
            Value *aliased = yaml_anchor_map_find(anchors, alias_name);

            if (!aliased) {
                yaml_event_delete(&event);
                yaml_anchor_map_free(anchors);
                // Use simple error since error_new_formatted is not working
                Value *error = value_new(VALUE_ERROR);
                if (!error) {
                    return NULL;
                }
                return error;  // Will show "Unknown error" message
            }

            result = value_ref(aliased);  // Increment ref count
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

static Value *parse_yaml_node(yaml_parser_t *parser, yaml_event_t *event, YamlAnchorMap *anchors)
{
    Value *value = NULL;

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
        return error_new("Unexpected YAML event");
    }

    // If this node has an anchor, register it
    if (value && !error_is_error(value)) {
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

static Value *parse_yaml_sequence(yaml_parser_t *parser, YamlAnchorMap *anchors)
{
    Value *array = value_new(VALUE_ARRAY);
    if (!array) {
        return error_memory_allocation();
    }

    yaml_event_t event;

    while (true) {
        if (!yaml_parser_parse(parser, &event)) {
            value_unref(array);
            return error_new("YAML parser error in sequence");
        }

        if (event.type == YAML_SEQUENCE_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        Value *item = NULL;

        if (event.type == YAML_ALIAS_EVENT) {
            // Handle alias in sequence
            const char *alias_name = (const char *)event.data.alias.anchor;
            Value *aliased = yaml_anchor_map_find(anchors, alias_name);

            if (!aliased) {
                yaml_event_delete(&event);
                value_unref(array);
                // Use simple error since error_new_formatted is not working
                Value *error = value_new(VALUE_ERROR);
                if (!error) {
                    return NULL;
                }
                return error;  // Will show "Unknown error" message
            }

            item = value_ref(aliased);  // Increment ref count
        } else {
            item = parse_yaml_node(parser, &event, anchors);
        }

        yaml_event_delete(&event);

        if (!item) {
            value_unref(array);
            return error_new("Failed to parse sequence item");
        }

        if (error_is_error(item)) {
            value_unref(array);
            return item;
        }

        array_push(array, item);
    }

    return array;
}

static Value *parse_yaml_mapping(yaml_parser_t *parser, YamlAnchorMap *anchors)
{
    Value *object = value_new(VALUE_OBJECT);
    if (!object) {
        return error_memory_allocation();
    }

    yaml_event_t event;

    while (true) {
        // Check for mapping end
        if (!yaml_parser_parse(parser, &event)) {
            value_unref(object);
            return error_new("YAML parser error in mapping");
        }

        if (event.type == YAML_MAPPING_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        // Parse key (must be scalar)
        if (event.type != YAML_SCALAR_EVENT) {
            yaml_event_delete(&event);
            value_unref(object);
            return error_new("YAML mapping key must be scalar");
        }

        Value *key = parse_yaml_scalar(&event);
        yaml_event_delete(&event);

        if (!key || key->type != VALUE_STRING) {
            if (key)
                value_unref(key);
            value_unref(object);
            return error_new("Invalid YAML mapping key");
        }

        // Parse value
        if (!yaml_parser_parse(parser, &event)) {
            value_unref(key);
            value_unref(object);
            return error_new("YAML parser error parsing mapping value");
        }

        Value *value = NULL;

        if (event.type == YAML_ALIAS_EVENT) {
            // Handle alias as mapping value
            const char *alias_name = (const char *)event.data.alias.anchor;
            Value *aliased = yaml_anchor_map_find(anchors, alias_name);

            if (!aliased) {
                yaml_event_delete(&event);
                value_unref(key);
                value_unref(object);
                // Use simple error since error_new_formatted is not working
                Value *error = value_new(VALUE_ERROR);
                if (!error) {
                    return NULL;
                }
                return error;  // Will show "Unknown error" message
            }

            value = value_ref(aliased);  // Increment ref count
        } else {
            value = parse_yaml_node(parser, &event, anchors);
        }

        yaml_event_delete(&event);

        if (!value) {
            value_unref(key);
            value_unref(object);
            return error_new("Failed to parse mapping value");
        }

        if (error_is_error(value)) {
            value_unref(key);
            value_unref(object);
            return value;
        }

        // Handle merge key (<<) specially
        if (strcmp(key->as.string->data, "<<") == 0) {
            // Merge the aliased object's properties into this object
            if (value->type == VALUE_OBJECT) {
                // Merge all properties from the aliased object
                for (size_t i = 0; i < value->as.object->length; i++) {
                    const char *merge_key = value->as.object->pairs[i].key;
                    Value *merge_value = value->as.object->pairs[i].value;

                    // Only set if key doesn't already exist (local values override merged ones)
                    if (!object_get(object, merge_key)) {
                        object_set(object, merge_key, value_ref(merge_value));
                    }
                }
            }
            value_unref(value);  // Don't need the merge object itself
        } else {
            // Normal key-value pair
            object_set(object, key->as.string->data, value);
        }
        value_unref(key);
    }

    return object;
}

static Value *parse_yaml_scalar(yaml_event_t *event)
{
    const char *value_str = (const char *)event->data.scalar.value;
    size_t length = event->data.scalar.length;

    if (!value_str || length == 0) {
        return value_new_string("");
    }

    // Handle null values
    if (strcmp(value_str, "null") == 0 || strcmp(value_str, "~") == 0 ||
        strcmp(value_str, "") == 0) {
        return value_new_null();
    }

    // Handle boolean values
    if (strcmp(value_str, "true") == 0 || strcmp(value_str, "yes") == 0 ||
        strcmp(value_str, "on") == 0) {
        return value_new_boolean(true);
    }
    if (strcmp(value_str, "false") == 0 || strcmp(value_str, "no") == 0 ||
        strcmp(value_str, "off") == 0) {
        return value_new_boolean(false);
    }

    // Try to parse as number
    char *endptr;
    double num_value = strtod(value_str, &endptr);
    if (*endptr == '\0') {
        return value_new_number(num_value);
    }

    // Default to string
    return value_new_string(value_str);
}

// Helper functions for emitting YAML

static bool
emit_yaml_value_safe(yaml_emitter_t *emitter, const Value *value, YamlRefTracker *tracker)
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
    if (value->type == VALUE_OBJECT || value->type == VALUE_ARRAY) {
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
    case VALUE_NULL: {
        yaml_event_t event;
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)"null", -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case VALUE_BOOLEAN: {
        yaml_event_t event;
        const char *bool_str = value->as.boolean ? "true" : "false";
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)bool_str, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case VALUE_NUMBER: {
        yaml_event_t event;
        char num_buffer[32];
        if (value->as.number == floor(value->as.number)) {
            snprintf(num_buffer, sizeof(num_buffer), "%.0f", value->as.number);
        } else {
            snprintf(num_buffer, sizeof(num_buffer), "%g", value->as.number);
        }
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)num_buffer, -1, 1, 1, YAML_PLAIN_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case VALUE_STRING: {
        yaml_event_t event;
        const char *str =
            (value->as.string && value->as.string->data) ? value->as.string->data : "";
        yaml_scalar_event_initialize(
            &event, NULL, NULL, (unsigned char *)str, -1, 1, 1, YAML_DOUBLE_QUOTED_SCALAR_STYLE);
        result = yaml_emitter_emit(emitter, &event);
        break;
    }

    case VALUE_ARRAY:
        result = emit_yaml_sequence_safe(emitter, value, tracker);
        break;

    case VALUE_OBJECT:
        result = emit_yaml_mapping_safe(emitter, value, tracker);
        break;

    default:
        result = false;
    }

    tracker->depth--;
    return result;
}

static bool
emit_yaml_sequence_safe(yaml_emitter_t *emitter, const Value *array, YamlRefTracker *tracker)
{
    if (!array || !array->as.array) {
        return false;
    }

    yaml_event_t event;

    // Start sequence
    yaml_sequence_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) {
        return false;
    }

    // Emit array items with circular reference protection
    for (size_t i = 0; i < array->as.array->length; i++) {
        if (!emit_yaml_value_safe(emitter, array->as.array->items[i], tracker)) {
            return false;
        }
    }

    // End sequence
    yaml_sequence_end_event_initialize(&event);
    return yaml_emitter_emit(emitter, &event);
}

static bool
emit_yaml_mapping_safe(yaml_emitter_t *emitter, const Value *object, YamlRefTracker *tracker)
{
    if (!object || !object->as.object) {
        return false;
    }

    yaml_event_t event;

    // Start mapping
    yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
    if (!yaml_emitter_emit(emitter, &event)) {
        return false;
    }

    // Emit object pairs with circular reference protection
    for (size_t i = 0; i < object->as.object->length; i++) {
        const char *key = object->as.object->pairs[i].key;
        const Value *value = object->as.object->pairs[i].value;

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
    tracker->visited = memory_alloc(sizeof(const Value *) * tracker->visited_capacity);
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
static bool yaml_ref_tracker_add(YamlRefTracker *tracker, const Value *value)
{
    if (!tracker || !value) {
        return false;
    }

    // Check if we need to expand capacity
    if (tracker->visited_count >= tracker->visited_capacity) {
        size_t new_capacity = tracker->visited_capacity * 2;
        const Value **new_visited =
            memory_realloc(tracker->visited, sizeof(const Value *) * new_capacity);
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
static bool yaml_ref_tracker_contains(const YamlRefTracker *tracker, const Value *value)
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
static bool yaml_anchor_map_add(YamlAnchorMap *map, const char *name, Value *value)
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
static Value *yaml_anchor_map_find(YamlAnchorMap *map, const char *name)
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