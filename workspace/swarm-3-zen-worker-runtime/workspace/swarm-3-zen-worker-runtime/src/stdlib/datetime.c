/*
 * datetime.c
 * Date and Time Functions for ZEN stdlib  
 * 
 * Contains only authorized functions as per MANIFEST.json
 */

#include "zen/types/value.h"
#include <time.h>

/**
 * @brief Get current Unix timestamp
 * @param args Arguments array (unused)
 * @param argc Number of arguments (unused)
 * @return Current timestamp as number value
 */
Value* datetime_now(Value** args, size_t argc) {
    // Ignore parameters for now
    (void)args;
    (void)argc;
    
    time_t now = time(NULL);
    return value_new_number((double)now);
}