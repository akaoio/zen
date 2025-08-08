#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// Read the current manifest
const manifestPath = path.join(__dirname, '..', 'MANIFEST.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));

// Define the missing stdlib files and their functions
const missingFiles = {
  "src/stdlib/array.c": {
    description: "Array manipulation functions for the standard library",
    functions: [
      {
        name: "array_push_stdlib",
        signature: "Value *array_push_stdlib(Value **args, size_t argc)",
        description: "Add element to end of array",
        line: 23,
        isStatic: false
      },
      {
        name: "array_pop_stdlib",
        signature: "Value *array_pop_stdlib(Value **args, size_t argc)",
        description: "Remove and return last element from array",
        line: 70,
        isStatic: false
      }
    ]
  },
  "src/stdlib/convert.c": {
    description: "Type conversion functions for the standard library",
    functions: [
      {
        name: "convert_to_string_internal",
        signature: "Value *convert_to_string_internal(const Value *value)",
        description: "Internal helper for string conversion",
        line: 27,
        isStatic: false
      },
      {
        name: "convert_to_number_internal",
        signature: "Value *convert_to_number_internal(const Value *value)",
        description: "Internal helper for number conversion",
        line: 171,
        isStatic: false
      },
      {
        name: "convert_to_boolean_internal",
        signature: "Value *convert_to_boolean_internal(const Value *value)",
        description: "Internal helper for boolean conversion",
        line: 255,
        isStatic: false
      },
      {
        name: "convert_type_of_internal",
        signature: "Value *convert_type_of_internal(const Value *value)",
        description: "Internal helper to get type name",
        line: 305,
        isStatic: false
      },
      {
        name: "convert_is_type_internal",
        signature: "Value *convert_is_type_internal(const Value *value, const char *type_name)",
        description: "Internal helper to check type",
        line: 339,
        isStatic: false
      },
      {
        name: "convert_parse_int_internal",
        signature: "Value *convert_parse_int_internal(const Value *str_value, const Value *base_value)",
        description: "Internal helper to parse integer",
        line: 362,
        isStatic: false
      },
      {
        name: "convert_parse_float_internal",
        signature: "Value *convert_parse_float_internal(const Value *str_value)",
        description: "Internal helper to parse float",
        line: 422,
        isStatic: false
      },
      {
        name: "convert_to_string",
        signature: "Value *convert_to_string(Value **args, size_t argc)",
        description: "Convert value to string representation",
        line: 472,
        isStatic: false
      },
      {
        name: "convert_to_number",
        signature: "Value *convert_to_number(Value **args, size_t argc)",
        description: "Convert value to number",
        line: 486,
        isStatic: false
      },
      {
        name: "convert_to_boolean",
        signature: "Value *convert_to_boolean(Value **args, size_t argc)",
        description: "Convert value to boolean",
        line: 500,
        isStatic: false
      },
      {
        name: "convert_type_of",
        signature: "Value *convert_type_of(Value **args, size_t argc)",
        description: "Get type name of value",
        line: 514,
        isStatic: false
      },
      {
        name: "convert_is_type",
        signature: "Value *convert_is_type(Value **args, size_t argc)",
        description: "Check if value is of specific type",
        line: 528,
        isStatic: false
      },
      {
        name: "convert_parse_int",
        signature: "Value *convert_parse_int(Value **args, size_t argc)",
        description: "Parse integer from string with optional radix",
        line: 542,
        isStatic: false
      },
      {
        name: "convert_parse_float",
        signature: "Value *convert_parse_float(Value **args, size_t argc)",
        description: "Parse floating point number from string",
        line: 558,
        isStatic: false
      }
    ]
  },
  "src/stdlib/datastructures.c": {
    description: "Advanced data structure implementations (set, priority queue)",
    functions: [
      {
        name: "datastructures_set_new",
        signature: "Value *datastructures_set_new(void)",
        description: "Create a new set",
        line: 20,
        isStatic: false
      },
      {
        name: "datastructures_set_add",
        signature: "Value *datastructures_set_add(Value *set_value, const Value *item_value)",
        description: "Add item to set",
        line: 28,
        isStatic: false
      },
      {
        name: "datastructures_set_contains",
        signature: "Value *datastructures_set_contains(Value *set_value, const Value *item_value)",
        description: "Check if set contains item",
        line: 49,
        isStatic: false
      },
      {
        name: "datastructures_set_remove",
        signature: "Value *datastructures_set_remove(Value *set_value, const Value *item_value)",
        description: "Remove item from set",
        line: 70,
        isStatic: false
      },
      {
        name: "datastructures_set_size",
        signature: "Value *datastructures_set_size(Value *set_value)",
        description: "Get size of set",
        line: 90,
        isStatic: false
      },
      {
        name: "datastructures_set_to_array",
        signature: "Value *datastructures_set_to_array(Value *set_value)",
        description: "Convert set to array",
        line: 109,
        isStatic: false
      },
      {
        name: "datastructures_set_union",
        signature: "Value *datastructures_set_union(Value *set1_value, Value *set2_value)",
        description: "Get union of two sets",
        line: 129,
        isStatic: false
      },
      {
        name: "datastructures_set_intersection",
        signature: "Value *datastructures_set_intersection(Value *set1_value, Value *set2_value)",
        description: "Get intersection of two sets",
        line: 149,
        isStatic: false
      },
      {
        name: "datastructures_set_difference",
        signature: "Value *datastructures_set_difference(Value *set1_value, Value *set2_value)",
        description: "Get difference of two sets",
        line: 169,
        isStatic: false
      },
      {
        name: "datastructures_set_is_subset",
        signature: "Value *datastructures_set_is_subset(Value *subset_value, Value *superset_value)",
        description: "Check if one set is subset of another",
        line: 189,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_new",
        signature: "Value *datastructures_priority_queue_new(void)",
        description: "Create a new priority queue",
        line: 209,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_push",
        signature: "Value *datastructures_priority_queue_push(Value *queue_value, const Value *item_value, const Value *priority_value)",
        description: "Push item with priority to queue",
        line: 218,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_pop",
        signature: "Value *datastructures_priority_queue_pop(Value *queue_value)",
        description: "Pop highest priority item from queue",
        line: 244,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_peek",
        signature: "Value *datastructures_priority_queue_peek(Value *queue_value)",
        description: "Peek at highest priority item without removing",
        line: 264,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_size",
        signature: "Value *datastructures_priority_queue_size(Value *queue_value)",
        description: "Get size of priority queue",
        line: 284,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_is_empty",
        signature: "Value *datastructures_priority_queue_is_empty(Value *queue_value)",
        description: "Check if priority queue is empty",
        line: 304,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_clear",
        signature: "Value *datastructures_priority_queue_clear(Value *queue_value)",
        description: "Clear all items from priority queue",
        line: 324,
        isStatic: false
      },
      {
        name: "datastructures_priority_queue_to_array",
        signature: "Value *datastructures_priority_queue_to_array(Value *queue_value)",
        description: "Convert priority queue to array",
        line: 344,
        isStatic: false
      }
    ]
  },
  "src/stdlib/datetime.c": {
    description: "Date and time manipulation functions",
    functions: [
      {
        name: "datetime_now",
        signature: "Value *datetime_now(Value **args, size_t argc)",
        description: "Get current date and time",
        line: 20,
        isStatic: false
      }
    ]
  },
  "src/stdlib/string.c": {
    description: "String manipulation functions for the standard library",
    functions: [
      {
        name: "string_length",
        signature: "Value *string_length(Value **args, size_t argc)",
        description: "Get length of string, array, or object",
        line: 26,
        isStatic: false
      },
      {
        name: "string_upper",
        signature: "Value *string_upper(Value **args, size_t argc)",
        description: "Convert string to uppercase",
        line: 50,
        isStatic: false
      },
      {
        name: "string_lower",
        signature: "Value *string_lower(Value **args, size_t argc)",
        description: "Convert string to lowercase",
        line: 88,
        isStatic: false
      },
      {
        name: "string_trim",
        signature: "Value *string_trim(Value **args, size_t argc)",
        description: "Trim whitespace from both ends of string",
        line: 126,
        isStatic: false
      },
      {
        name: "string_split",
        signature: "Value *string_split(Value **args, size_t argc)",
        description: "Split string by delimiter",
        line: 177,
        isStatic: false
      },
      {
        name: "string_contains",
        signature: "Value *string_contains(Value **args, size_t argc)",
        description: "Check if string contains substring",
        line: 262,
        isStatic: false
      },
      {
        name: "string_replace",
        signature: "Value *string_replace(Value **args, size_t argc)",
        description: "Replace substring in string",
        line: 298,
        isStatic: false
      }
    ]
  },
  "src/stdlib/system.c": {
    description: "System interaction functions",
    functions: [
      {
        name: "system_exec",
        signature: "Value *system_exec(Value **args, size_t argc)",
        description: "Execute system command",
        line: 23,
        isStatic: false
      }
    ]
  }
};

// Add the missing files to the manifest
for (const [filePath, fileData] of Object.entries(missingFiles)) {
  manifest.files[filePath] = fileData;
}

// Sort the files object by key for better organization
const sortedFiles = {};
Object.keys(manifest.files).sort().forEach(key => {
  sortedFiles[key] = manifest.files[key];
});
manifest.files = sortedFiles;

// Write the updated manifest
fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');

console.log('Successfully added missing stdlib files to MANIFEST.json:');
Object.keys(missingFiles).forEach(file => {
  console.log(`  - ${file}`);
});
console.log('\nTotal functions added:', 
  Object.values(missingFiles).reduce((sum, file) => sum + file.functions.length, 0)
);