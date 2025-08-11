#!/bin/bash

# Migrate all stdlib files from Value to RuntimeValue system

echo "Starting migration of stdlib files..."

# Replace in all stdlib files
for file in src/stdlib/*.c; do
    echo "Migrating $(basename $file)..."
    
    # Replace includes
    sed -i 's|#include "zen/types/value.h"|#include "zen/core/runtime_value.h"|g' "$file"
    sed -i 's|#include "zen/types/array.h"|#include "zen/core/runtime_value.h"|g' "$file"
    sed -i 's|#include "zen/types/object.h"|#include "zen/core/runtime_value.h"|g' "$file"
    
    # Replace Value types
    sed -i 's/\bValue \*/RuntimeValue */g' "$file"
    sed -i 's/\bconst Value \*/const RuntimeValue */g' "$file"
    sed -i 's/\bValue\*\*/RuntimeValue**/g' "$file"
    
    # Replace value_ functions with rv_ equivalents
    sed -i 's/\bvalue_new_string/rv_new_string/g' "$file"
    sed -i 's/\bvalue_new_number/rv_new_number/g' "$file"
    sed -i 's/\bvalue_new_boolean/rv_new_boolean/g' "$file"
    sed -i 's/\bvalue_new_null/rv_new_null/g' "$file"
    sed -i 's/\bvalue_new_array/rv_new_array/g' "$file"
    sed -i 's/\bvalue_new_object/rv_new_object/g' "$file"
    sed -i 's/\bvalue_new_error/rv_new_error/g' "$file"
    sed -i 's/\bvalue_new_function/rv_new_function/g' "$file"
    
    sed -i 's/\bvalue_ref/rv_ref/g' "$file"
    sed -i 's/\bvalue_unref/rv_unref/g' "$file"
    
    sed -i 's/\bvalue_is_null/rv_is_null/g' "$file"
    sed -i 's/\bvalue_is_boolean/rv_is_boolean/g' "$file"
    sed -i 's/\bvalue_is_number/rv_is_number/g' "$file"
    sed -i 's/\bvalue_is_string/rv_is_string/g' "$file"
    sed -i 's/\bvalue_is_array/rv_is_array/g' "$file"
    sed -i 's/\bvalue_is_object/rv_is_object/g' "$file"
    sed -i 's/\bvalue_is_function/rv_is_function/g' "$file"
    sed -i 's/\bvalue_is_error/rv_is_error/g' "$file"
    
    sed -i 's/\bvalue_get_string/rv_get_string/g' "$file"
    sed -i 's/\bvalue_get_number/rv_get_number/g' "$file"
    sed -i 's/\bvalue_get_boolean/rv_get_boolean/g' "$file"
    
    sed -i 's/\bvalue_to_string/rv_to_string/g' "$file"
    sed -i 's/\bvalue_to_number/rv_to_number/g' "$file"
    sed -i 's/\bvalue_to_boolean/rv_to_boolean/g' "$file"
    
    sed -i 's/\barray_push/rv_array_push/g' "$file"
    sed -i 's/\barray_get/rv_array_get/g' "$file"
    sed -i 's/\barray_set/rv_array_set/g' "$file"
    sed -i 's/\barray_length/rv_array_length/g' "$file"
    
    sed -i 's/\bobject_set/rv_object_set/g' "$file"
    sed -i 's/\bobject_get/rv_object_get/g' "$file"
    
    # Replace VALUE_ enums
    sed -i 's/\bVALUE_NULL\b/RV_TYPE_NULL/g' "$file"
    sed -i 's/\bVALUE_BOOLEAN\b/RV_TYPE_BOOLEAN/g' "$file"
    sed -i 's/\bVALUE_NUMBER\b/RV_TYPE_NUMBER/g' "$file"
    sed -i 's/\bVALUE_STRING\b/RV_TYPE_STRING/g' "$file"
    sed -i 's/\bVALUE_ARRAY\b/RV_TYPE_ARRAY/g' "$file"
    sed -i 's/\bVALUE_OBJECT\b/RV_TYPE_OBJECT/g' "$file"
    sed -i 's/\bVALUE_FUNCTION\b/RV_TYPE_FUNCTION/g' "$file"
    sed -i 's/\bVALUE_ERROR\b/RV_TYPE_ERROR/g' "$file"
    
    # Fix struct access patterns
    sed -i 's/->as\.string->data/->data.string/g' "$file"
    sed -i 's/->as\.string->length/->data.string ? strlen(value->data.string) : 0/g' "$file"
    sed -i 's/->as\.number/->data.number/g' "$file"
    sed -i 's/->as\.boolean/->data.boolean/g' "$file"
    sed -i 's/->as\.array/->data.array/g' "$file"
    sed -i 's/->as\.object/->data.object/g' "$file"
    sed -i 's/->as\.error/->data.error/g' "$file"
done

# Update header files too
for file in src/include/zen/stdlib/*.h; do
    echo "Migrating header $(basename $file)..."
    
    sed -i 's|#include "zen/types/value.h"|#include "zen/core/runtime_value.h"|g' "$file"
    sed -i 's/\bValue \*/RuntimeValue */g' "$file"
    sed -i 's/\bconst Value \*/const RuntimeValue */g' "$file"
done

echo "Migration complete!"