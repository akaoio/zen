/*
 * system.c
 * System Integration Functions for ZEN stdlib
 * 
 * Provides environment variables, process execution, file system utilities,
 * and basic network operations with security considerations
 */

#define _GNU_SOURCE
#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>

// Forward declarations
Value* array_new(size_t initial_capacity);
void array_push(Value* array, Value* item);
Value* object_new(void);
void object_set(Value* object, const char* key, Value* value);

/**
 * @brief Get environment variable value
 * @param var_name_value Environment variable name
 * @return Environment variable value or null if not found
 */
Value* zen_system_getenv(const Value* var_name_value) {
    if (!var_name_value || var_name_value->type != VALUE_STRING) {
        return value_new_null();
    }
    
    const char* value = getenv(var_name_value->as.string->data);
    if (!value) {
        return value_new_null();
    }
    
    return value_new_string(value);
}

/**
 * @brief Set environment variable
 * @param var_name_value Environment variable name
 * @param var_value_value Environment variable value
 * @return Boolean indicating success
 */
Value* zen_system_setenv(const Value* var_name_value, const Value* var_value_value) {
    if (!var_name_value || var_name_value->type != VALUE_STRING ||
        !var_value_value || var_value_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    int result = setenv(var_name_value->as.string->data, var_value_value->as.string->data, 1);
    return value_new_boolean(result == 0);
}

/**
 * @brief Execute system command and capture output
 * @param command_value Command to execute
 * @return Object with stdout, stderr, and exit_code
 */
Value* zen_system_exec(const Value* command_value) {
    if (!command_value || command_value->type != VALUE_STRING) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Invalid command string");
            error->as.error->code = -1;
        }
        return error;
    }
    
    const char* command = command_value->as.string->data;
    
    // Security check: prevent dangerous commands
    if (strstr(command, "rm -rf") || strstr(command, "mkfs") || 
        strstr(command, "dd if=") || strstr(command, ":(){ :|:& };:")) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Command rejected for security reasons");
            error->as.error->code = -1;
        }
        return error;
    }
    
    // Use popen to capture output
    FILE* fp = popen(command, "r");
    if (!fp) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Failed to execute command");
            error->as.error->code = -1;
        }
        return error;
    }
    
    // Read output
    char buffer[4096];
    size_t total_size = 0;
    char* output = malloc(1);
    if (!output) {
        pclose(fp);
        return value_new(VALUE_ERROR);
    }
    output[0] = '\0';
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strlen(buffer);
        char* new_output = realloc(output, total_size + len + 1);
        if (!new_output) {
            free(output);
            pclose(fp);
            return value_new(VALUE_ERROR);
        }
        output = new_output;
        strcpy(output + total_size, buffer);
        total_size += len;
    }
    
    int exit_code = pclose(fp);
    
    // Create result object
    Value* result = object_new();
    object_set(result, "stdout", value_new_string(output));
    object_set(result, "exit_code", value_new_number((double)exit_code));
    
    free(output);
    return result;
}

/**
 * @brief Get current working directory
 * @return Current working directory path
 */
Value* zen_system_getcwd(void) {
    char* cwd = getcwd(NULL, 0);
    if (!cwd) {
        return value_new_string("");
    }
    
    Value* result = value_new_string(cwd);
    free(cwd);
    return result;
}

/**
 * @brief Change current working directory
 * @param path_value Path to change to
 * @return Boolean indicating success
 */
Value* zen_system_chdir(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    int result = chdir(path_value->as.string->data);
    return value_new_boolean(result == 0);
}

/**
 * @brief Create directory
 * @param path_value Directory path to create
 * @param mode_value Directory permissions (optional, default 0755)
 * @return Boolean indicating success
 */
Value* zen_system_mkdir(const Value* path_value, const Value* mode_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    mode_t mode = 0755; // Default permissions
    if (mode_value && mode_value->type == VALUE_NUMBER) {
        mode = (mode_t)mode_value->as.number;
    }
    
    int result = mkdir(path_value->as.string->data, mode);
    return value_new_boolean(result == 0);
}

/**
 * @brief Remove directory (only if empty)
 * @param path_value Directory path to remove
 * @return Boolean indicating success
 */
Value* zen_system_rmdir(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    int result = rmdir(path_value->as.string->data);
    return value_new_boolean(result == 0);
}

/**
 * @brief List directory contents
 * @param path_value Directory path to list
 * @return Array of filenames in directory
 */
Value* zen_system_listdir(const Value* path_value) {
    const char* path = "."; // Default to current directory
    if (path_value && path_value->type == VALUE_STRING) {
        path = path_value->as.string->data;
    }
    
    DIR* dir = opendir(path);
    if (!dir) {
        return array_new(0);
    }
    
    Value* result_array = array_new(0);
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        Value* filename = value_new_string(entry->d_name);
        array_push(result_array, filename);
    }
    
    closedir(dir);
    return result_array;
}

/**
 * @brief Check if path exists
 * @param path_value Path to check
 * @return Boolean indicating if path exists
 */
Value* zen_system_path_exists(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    struct stat st;
    int result = stat(path_value->as.string->data, &st);
    return value_new_boolean(result == 0);
}

/**
 * @brief Check if path is a directory
 * @param path_value Path to check
 * @return Boolean indicating if path is directory
 */
Value* zen_system_is_dir(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    struct stat st;
    if (stat(path_value->as.string->data, &st) != 0) {
        return value_new_boolean(false);
    }
    
    return value_new_boolean(S_ISDIR(st.st_mode));
}

/**
 * @brief Check if path is a regular file
 * @param path_value Path to check
 * @return Boolean indicating if path is file
 */
Value* zen_system_is_file(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    struct stat st;
    if (stat(path_value->as.string->data, &st) != 0) {
        return value_new_boolean(false);
    }
    
    return value_new_boolean(S_ISREG(st.st_mode));
}

/**
 * @brief Get file size in bytes
 * @param path_value File path
 * @return File size in bytes, or -1 if error
 */
Value* zen_system_file_size(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_number(-1);
    }
    
    struct stat st;
    if (stat(path_value->as.string->data, &st) != 0) {
        return value_new_number(-1);
    }
    
    return value_new_number((double)st.st_size);
}

/**
 * @brief Get file modification time as timestamp
 * @param path_value File path
 * @return File modification time as Unix timestamp
 */
Value* zen_system_file_mtime(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_number(0);
    }
    
    struct stat st;
    if (stat(path_value->as.string->data, &st) != 0) {
        return value_new_number(0);
    }
    
    return value_new_number((double)st.st_mtime);
}

/**
 * @brief Sleep for specified number of seconds
 * @param seconds_value Number of seconds to sleep
 * @return Boolean indicating success
 */
Value* zen_system_sleep(const Value* seconds_value) {
    if (!seconds_value || seconds_value->type != VALUE_NUMBER) {
        return value_new_boolean(false);
    }
    
    double seconds = seconds_value->as.number;
    if (seconds < 0) {
        return value_new_boolean(false);
    }
    
    if (seconds >= 1.0) {
        sleep((unsigned int)seconds);
    }
    
    // Handle fractional seconds with usleep
    double fractional = seconds - (int)seconds;
    if (fractional > 0) {
        usleep((useconds_t)(fractional * 1000000));
    }
    
    return value_new_boolean(true);
}

/**
 * @brief Get process ID
 * @return Current process ID
 */
Value* zen_system_getpid(void) {
    return value_new_number((double)getpid());
}

/**
 * @brief Get parent process ID
 * @return Parent process ID
 */
Value* zen_system_getppid(void) {
    return value_new_number((double)getppid());
}

/**
 * @brief Get system hostname
 * @return System hostname as string
 */
Value* zen_system_hostname(void) {
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return value_new_string("unknown");
    }
    
    hostname[HOST_NAME_MAX] = '\0'; // Ensure null termination
    return value_new_string(hostname);
}

/**
 * @brief Get username of current user
 * @return Username as string
 */
Value* zen_system_username(void) {
    const char* username = getenv("USER");
    if (!username) {
        username = getenv("USERNAME"); // Windows fallback
    }
    if (!username) {
        username = "unknown";
    }
    
    return value_new_string(username);
}

/**
 * @brief Copy file from source to destination
 * @param src_value Source file path
 * @param dest_value Destination file path
 * @return Boolean indicating success
 */
Value* zen_system_copy_file(const Value* src_value, const Value* dest_value) {
    if (!src_value || src_value->type != VALUE_STRING ||
        !dest_value || dest_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    FILE* src_fp = fopen(src_value->as.string->data, "rb");
    if (!src_fp) {
        return value_new_boolean(false);
    }
    
    FILE* dest_fp = fopen(dest_value->as.string->data, "wb");
    if (!dest_fp) {
        fclose(src_fp);
        return value_new_boolean(false);
    }
    
    char buffer[8192];
    size_t bytes_read;
    bool success = true;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_fp) != bytes_read) {
            success = false;
            break;
        }
    }
    
    fclose(src_fp);
    fclose(dest_fp);
    
    return value_new_boolean(success);
}

/**
 * @brief Move/rename file
 * @param src_value Source file path
 * @param dest_value Destination file path
 * @return Boolean indicating success
 */
Value* zen_system_move_file(const Value* src_value, const Value* dest_value) {
    if (!src_value || src_value->type != VALUE_STRING ||
        !dest_value || dest_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    int result = rename(src_value->as.string->data, dest_value->as.string->data);
    return value_new_boolean(result == 0);
}

/**
 * @brief Delete file
 * @param path_value File path to delete
 * @return Boolean indicating success
 */
Value* zen_system_delete_file(const Value* path_value) {
    if (!path_value || path_value->type != VALUE_STRING) {
        return value_new_boolean(false);
    }
    
    int result = unlink(path_value->as.string->data);
    return value_new_boolean(result == 0);
}

/**
 * @brief Get system uptime in seconds
 * @return System uptime in seconds (Linux only, returns 0 on other systems)
 */
Value* zen_system_uptime(void) {
#ifdef __linux__
    FILE* fp = fopen("/proc/uptime", "r");
    if (!fp) {
        return value_new_number(0.0);
    }
    
    double uptime;
    if (fscanf(fp, "%lf", &uptime) == 1) {
        fclose(fp);
        return value_new_number(uptime);
    }
    
    fclose(fp);
#endif
    return value_new_number(0.0);
}

/**
 * @brief Get system load average (Linux/Unix only)
 * @return Array with 1, 5, and 15 minute load averages
 */
Value* zen_system_load_average(void) {
    Value* result_array = array_new(3);
    
#ifdef __linux__
    double loadavg[3];
    if (getloadavg(loadavg, 3) != -1) {
        array_push(result_array, value_new_number(loadavg[0]));
        array_push(result_array, value_new_number(loadavg[1]));
        array_push(result_array, value_new_number(loadavg[2]));
        return result_array;
    }
#endif
    
    // Return zeros if not available
    array_push(result_array, value_new_number(0.0));
    array_push(result_array, value_new_number(0.0));
    array_push(result_array, value_new_number(0.0));
    return result_array;
}