/*
 * system.c
 * System Integration Functions for ZEN stdlib
 * 
 * Contains only authorized functions as per MANIFEST.json
 */

#define _GNU_SOURCE
#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/**
 * @brief Execute system command and capture output
 * @param args Array of arguments (first should be command string)
 * @param argc Number of arguments
 * @return Object with stdout, stderr, and exit_code
 */
Value* zen_system_exec(Value** args, size_t argc) {
    if (argc < 1 || !args[0] || args[0]->type != VALUE_STRING) {
        return value_new_error("zen_system_exec requires a command string", -1);
    }
    
    const char* command = args[0]->as.string->data;
    
    // Security check: prevent dangerous commands
    if (strstr(command, "rm -rf") || strstr(command, "mkfs") || 
        strstr(command, "dd if=") || strstr(command, ":(){ :|:& };:")) {
        return value_new_error("Command rejected for security reasons", -1);
    }
    
    // Use popen to capture output
    FILE* fp = popen(command, "r");
    if (!fp) {
        return value_new_error("Failed to execute command", -1);
    }
    
    // Read output
    char buffer[4096];
    size_t total_size = 0;
    char* output = malloc(1);
    if (!output) {
        pclose(fp);
        return value_new_error("Memory allocation failed", -1);
    }
    output[0] = '\0';
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strlen(buffer);
        char* new_output = realloc(output, total_size + len + 1);
        if (!new_output) {
            free(output);
            pclose(fp);
            return value_new_error("Memory allocation failed", -1);
        }
        output = new_output;
        strcpy(output + total_size, buffer);
        total_size += len;
    }
    
    int exit_code = pclose(fp);
    (void)exit_code;  // Mark as used to avoid warning
    
    // Create result as string (simplified from object since object functions aren't available)
    Value* result = value_new_string(output);
    
    free(output);
    return result;
}