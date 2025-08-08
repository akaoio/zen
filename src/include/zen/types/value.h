#ifndef ZEN_VALUE_H
#define ZEN_VALUE_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Value types supported by ZEN
 */
typedef enum {
    VALUE_NULL,
    VALUE_BOOLEAN,
    VALUE_UNDECIDABLE,  // Gödel-approved undecidable value type
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_ARRAY,
    VALUE_OBJECT,
    VALUE_FUNCTION,
    VALUE_ERROR,
    VALUE_CLASS,
    VALUE_INSTANCE,
    VALUE_SET,
    VALUE_PRIORITY_QUEUE
} ValueType;

/**
 * @brief String structure for ZEN strings
 */
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} ZenString;

/**
 * @brief Array structure for ZEN arrays
 */
typedef struct {
    struct Value **items;
    size_t length;
    size_t capacity;
} ZenArray;

/**
 * @brief Object key-value pair
 */
typedef struct {
    char *key;
    struct Value *value;
} ZenObjectPair;

/**
 * @brief Object structure for ZEN objects
 */
typedef struct {
    ZenObjectPair *pairs;
    size_t length;
    size_t capacity;
} ZenObject;

/**
 * @brief Error structure for ZEN errors
 */
typedef struct {
    char *message;
    int code;
} ZenError;

/**
 * @brief Class definition structure
 */
typedef struct {
    char *name;
    char *parent_class_name;
    struct Value *parent_class;  // Reference to parent class
    struct Value *methods;       // Object containing methods
    struct Value *constructor;   // Special constructor method
} ZenClass;

/**
 * @brief Class instance structure
 */
typedef struct {
    struct Value *class_def;   // Reference to class definition
    struct Value *properties;  // Object containing instance properties
} ZenInstance;

/**
 * @brief Set structure for ZEN sets
 * Uses object hash table for O(1) operations
 */
typedef struct {
    struct Value *hash_table;  // Object used as hash table
    size_t size;               // Number of elements in set
} ZenSet;

/**
 * @brief Priority queue item
 */
typedef struct {
    struct Value *value;  // The actual value
    double priority;      // Priority (higher = more priority)
} ZenPriorityItem;

/**
 * @brief Priority queue structure (binary heap)
 */
typedef struct {
    ZenPriorityItem *items;  // Array of priority items
    size_t length;           // Current number of items
    size_t capacity;         // Total capacity
} ZenPriorityQueue;

/**
 * @brief Main value structure for ZEN runtime
 */
typedef struct Value {
    ValueType type;
    size_t ref_count;
    union {
        bool boolean;
        double number;
        ZenString *string;
        ZenArray *array;
        ZenObject *object;
        ZenError *error;
        void *function;  // For future function support
        ZenClass *class_def;
        ZenInstance *instance;
        ZenSet *set;
        ZenPriorityQueue *priority_queue;
    } as;
} Value;

// Forward declarations of value functions

/**
 * @brief Create new value of specified type
 * @param type The type of value to create
 * @return Newly allocated value or NULL on failure
 */
Value *value_new(ValueType type);

/**
 * @brief Create string value
 * @param str String to store (will be copied)
 * @return Newly allocated string value or NULL on failure
 */
Value *value_new_string(const char *str);

/**
 * @brief Create number value
 * @param num Number to store
 * @return Newly allocated number value or NULL on failure
 */
Value *value_new_number(double num);

/**
 * @brief Create boolean value
 * @param val Boolean value to store
 * @return Newly allocated boolean value or NULL on failure
 */
Value *value_new_boolean(bool val);

/**
 * @brief Create a new undecidable value (Gödel-approved)
 * @return New undecidable Value
 */
Value *value_new_undecidable(void);

/**
 * @brief Create null value
 * @return Newly allocated null value or NULL on failure
 */
Value *value_new_null(void);

/**
 * @brief Deep copy value
 * @param value Value to copy
 * @return Deep copy of the value or NULL on failure
 */
Value *value_copy(const Value *value);

/**
 * @brief Free value memory
 * @param value Value to free (may be NULL)
 */
void value_free(Value *value);

/**
 * @brief Convert value to string representation
 * @param value Value to convert
 * @return String representation (caller must free) or NULL on failure
 */
char *value_to_string(const Value *value);

/**
 * @brief Compare two values for equality
 * @param a First value to compare
 * @param b Second value to compare
 * @return true if values are equal, false otherwise
 */
bool value_equals(const Value *a, const Value *b);

/**
 * @brief Get string name of value type
 * @param type Value type
 * @return String name of the type (static string)
 */
const char *value_type_name(ValueType type);

/**
 * @brief Increment reference count of value
 * @param value Value to reference (may be NULL)
 * @return The same value pointer for convenience
 */
Value *value_ref(Value *value);

/**
 * @brief Decrement reference count and free if zero
 * @param value Value to unreference (may be NULL)
 */
void value_unref(Value *value);

// Class and instance creation functions

/**
 * @brief Create a new class value
 * @param name Class name
 * @param parent_name Parent class name (may be NULL)
 * @return Newly allocated class value or NULL on failure
 */
Value *value_new_class(const char *name, const char *parent_name);

/**
 * @brief Create a new instance of a class
 * @param class_def Class definition value
 * @return Newly allocated instance value or NULL on failure
 */
Value *value_new_instance(Value *class_def);

/**
 * @brief Add a method to a class
 * @param class_val Class value
 * @param method_name Method name
 * @param method_func Method function value
 */
void value_class_add_method(Value *class_val, const char *method_name, Value *method_func);

/**
 * @brief Get a method from a class
 * @param class_val Class value
 * @param method_name Method name to find
 * @return Method function value or NULL if not found
 */
Value *value_class_get_method(Value *class_val, const char *method_name);

/**
 * @brief Set a property on an instance
 * @param instance Instance value
 * @param property_name Property name
 * @param value Property value
 */
void value_instance_set_property(Value *instance, const char *property_name, Value *value);

/**
 * @brief Get a property from an instance
 * @param instance Instance value
 * @param property_name Property name to get
 * @return Property value or NULL if not found
 */
Value *value_instance_get_property(Value *instance, const char *property_name);

// Enhanced type operations for error system and stdlib support

/**
 * @brief Create an error value for stdlib functions
 * @param message Error message
 * @param error_code Error code
 * @return New error Value
 */
Value *value_new_error(const char *message, int error_code);

/**
 * @brief Check if value is truthy (for conditional expressions)
 * @param value Value to test
 * @return true if truthy, false otherwise
 */
bool value_is_truthy_public(const Value *value);

/**
 * @brief Safe conversion to number for stdlib operations
 * @param value Value to convert
 * @return Converted number or NaN if conversion fails
 */
double value_to_number_or_nan(const Value *value);

/**
 * @brief Enhanced string conversion with better error handling
 * @param value Value to convert
 * @return String representation or NULL on critical failure
 */
char *value_to_string_safe(const Value *value);

/**
 * @brief Check if two values can be compared
 * @param a First value
 * @param b Second value
 * @return true if values are comparable
 */
bool value_is_comparable(const Value *a, const Value *b);

/**
 * @brief Get the "length" of a value for stdlib length() function
 * @param value Value to get length of
 * @return Length or 0 if not applicable
 */
size_t value_get_length(const Value *value);

// Advanced type introspection functions

/**
 * @brief Get the runtime type of a value (enhanced typeof)
 * @param value Value to inspect
 * @return Detailed type information string (caller must free)
 */
char *value_typeof_enhanced(const Value *value);

/**
 * @brief Check if a value is numeric (number or numeric string)
 * @param value Value to check
 * @return true if value represents a number
 */
bool value_is_numeric(const Value *value);

/**
 * @brief Check if a value is iterable (array, object, string)
 * @param value Value to check
 * @return true if value can be iterated
 */
bool value_is_iterable(const Value *value);

/**
 * @brief Check if a value is callable (function)
 * @param value Value to check
 * @return true if value can be called
 */
bool value_is_callable(const Value *value);

/**
 * @brief Check if a value is an instance of a specific class
 * @param value Value to check
 * @param class_name Class name to check against
 * @return true if value is an instance of the class
 */
bool value_instanceof(const Value *value, const char *class_name);

// Enhanced type conversions

/**
 * @brief Convert value to number with precision information
 * @param value Value to convert
 * @param is_lossless Pointer to store whether conversion is lossless
 * @return Converted number or NaN if conversion fails
 */
double value_to_number_with_precision(const Value *value, bool *is_lossless);

/**
 * @brief Parse number from string with enhanced format support
 * @param str String to parse
 * @param result Pointer to store parsed number
 * @return true if parsing succeeded
 */
bool value_parse_number_enhanced(const char *str, double *result);

// Performance optimization features

/**
 * @brief Create a shallow copy of a value (for copy-on-write optimization)
 * @param value Value to copy
 * @return Shallow copy with shared data where appropriate
 */
Value *value_shallow_copy(const Value *value);

/**
 * @brief Check if a value can be safely shared (immutable)
 * @param value Value to check
 * @return true if value is immutable and can be shared
 */
bool value_is_immutable(const Value *value);

/**
 * @brief Get hash code for a value (for optimization purposes)
 * @param value Value to hash
 * @return Hash code for the value
 */
size_t value_hash(const Value *value);

// Value interning for performance

/**
 * @brief Initialize value singletons for optimization
 */
void value_init_singletons(void);

/**
 * @brief Get singleton value for common constants
 * @param type Type of singleton
 * @param number_val Number value (for numbers)
 * @param bool_val Boolean value (for booleans)
 * @return Singleton value or NULL if not available
 */
Value *value_get_singleton(ValueType type, double number_val, bool bool_val);

/**
 * @brief Cleanup value singletons
 */
void value_cleanup_singletons(void);

// Set operations

/**
 * @brief Create new set value
 * @return Newly allocated set value or NULL on failure
 */
Value *value_new_set(void);

/**
 * @brief Add item to set
 * @param set_val Set value
 * @param item Item to add
 * @return true if item was added (wasn't already present), false otherwise
 */
bool value_set_add(Value *set_val, const Value *item);

/**
 * @brief Check if set contains item
 * @param set_val Set value
 * @param item Item to check
 * @return true if item is in set, false otherwise
 */
bool value_set_contains(const Value *set_val, const Value *item);

/**
 * @brief Remove item from set
 * @param set_val Set value
 * @param item Item to remove
 * @return true if item was removed (was present), false otherwise
 */
bool value_set_remove(Value *set_val, const Value *item);

/**
 * @brief Get size of set
 * @param set_val Set value
 * @return Number of items in set
 */
size_t value_set_size(const Value *set_val);

/**
 * @brief Convert set to array
 * @param set_val Set value
 * @return Array containing all set items or NULL on failure
 */
Value *value_set_to_array(const Value *set_val);

// Priority queue operations

/**
 * @brief Create new priority queue value
 * @return Newly allocated priority queue value or NULL on failure
 */
Value *value_new_priority_queue(void);

/**
 * @brief Add item with priority to queue
 * @param queue_val Priority queue value
 * @param item Item to add
 * @param priority Priority of item (higher = more priority)
 * @return true if successful, false on failure
 */
bool value_priority_queue_push(Value *queue_val, const Value *item, double priority);

/**
 * @brief Remove and return highest priority item
 * @param queue_val Priority queue value
 * @return Highest priority item or NULL if queue is empty
 */
Value *value_priority_queue_pop(Value *queue_val);

/**
 * @brief Peek at highest priority item without removing
 * @param queue_val Priority queue value
 * @return Highest priority item or NULL if queue is empty
 */
Value *value_priority_queue_peek(const Value *queue_val);

/**
 * @brief Get size of priority queue
 * @param queue_val Priority queue value
 * @return Number of items in queue
 */
size_t value_priority_queue_size(const Value *queue_val);

/**
 * @brief Check if priority queue is empty
 * @param queue_val Priority queue value
 * @return true if empty, false otherwise
 */
bool value_priority_queue_is_empty(const Value *queue_val);

#endif
