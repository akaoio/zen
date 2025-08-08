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
    // Support optional format parameter
    if (argc > 1) {
        return error_new("datetime_now() accepts at most 1 argument (format)");
    }
    
    time_t now = time(NULL);
    
    // If no format specified, return Unix timestamp
    if (argc == 0) {
        return value_new_number((double)now);
    }
    
    // Parse format argument
    if (args[0]->type != VALUE_STRING) {
        return error_new("Format argument must be a string");
    }
    
    const char* format = args[0]->as.string->data;
    struct tm* tm_info = localtime(&now);
    
    if (!tm_info) {
        return error_new("Failed to get local time");
    }
    
    // Handle different format options
    if (strcmp(format, "unix") == 0) {
        return value_new_number((double)now);
    } else if (strcmp(format, "iso") == 0 || strcmp(format, "iso8601") == 0) {
        // ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ
        char iso_buffer[32];
        strftime(iso_buffer, sizeof(iso_buffer), "%Y-%m-%dT%H:%M:%SZ", tm_info);
        return value_new_string(iso_buffer);
    } else if (strcmp(format, "date") == 0) {
        // Date only: YYYY-MM-DD
        char date_buffer[16];
        strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", tm_info);
        return value_new_string(date_buffer);
    } else if (strcmp(format, "time") == 0) {
        // Time only: HH:MM:SS
        char time_buffer[16];
        strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);
        return value_new_string(time_buffer);
    } else if (strcmp(format, "readable") == 0 || strcmp(format, "human") == 0) {
        // Human-readable format
        char readable_buffer[64];
        strftime(readable_buffer, sizeof(readable_buffer), "%A, %B %d, %Y at %I:%M %p", tm_info);
        return value_new_string(readable_buffer);
    } else {
        // Custom format - pass directly to strftime
        char custom_buffer[256];
        size_t result = strftime(custom_buffer, sizeof(custom_buffer), format, tm_info);
        
        if (result == 0) {
            return error_new("Invalid format string or resulting string too long");
        }
        
        return value_new_string(custom_buffer);
    }
}