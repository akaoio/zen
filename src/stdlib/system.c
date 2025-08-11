/*
 * system.c
 * System Integration Functions for ZEN stdlib
 *
 * This is a stub implementation to allow compilation.
 * Full system integration features will be implemented in a later phase.
 */

#define _GNU_SOURCE  // For popen/pclose
#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Stub implementations for system functions
// These return appropriate error messages indicating future implementation

RuntimeValue *system_exec(RuntimeValue **args, size_t argc)
{
    // Validate arguments
    if (argc != 1) {
        return rv_new_error("exec requires exactly 1 argument (command)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("exec requires a string command", -1);
    }

    const char *command = args[0]->data.string.data;

    // Use popen to execute the command and capture output
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        return rv_new_error("Failed to execute command", -1);
    }

    // Read the output
    char buffer[256];
    size_t total_size = 0;
    size_t capacity = 1024;
    char *output = memory_alloc(capacity);
    if (!output) {
        pclose(pipe);
        return rv_new_error("Memory allocation failed", -1);
    }
    output[0] = '\0';

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t len = strlen(buffer);
        if (total_size + len + 1 > capacity) {
            capacity *= 2;
            char *new_output = memory_realloc(output, capacity);
            if (!new_output) {
                memory_free(output);
                pclose(pipe);
                return rv_new_error("Memory allocation failed", -1);
            }
            output = new_output;
        }
        strcat(output, buffer);
        total_size += len;
    }

    // Get the exit status
    int exit_status = pclose(pipe);
    int return_code = WIFEXITED(exit_status) ? WEXITSTATUS(exit_status) : -1;

    // Create result object with output and exit code
    RuntimeValue *result = rv_new_object();
    if (!result) {
        memory_free(output);
        return rv_new_error("Failed to create result object", -1);
    }

    // Add output string
    RuntimeValue *output_val = rv_new_string(output);
    memory_free(output);
    if (!output_val) {
        rv_unref(result);
        return rv_new_error("Failed to create output string", -1);
    }
    rv_object_set(result, "output", output_val);
    rv_unref(output_val);

    // Add exit code
    RuntimeValue *code_val = rv_new_number(return_code);
    rv_object_set(result, "code", code_val);
    rv_unref(code_val);

    // Add success flag
    RuntimeValue *success_val = rv_new_boolean(return_code == 0);
    rv_object_set(result, "success", success_val);
    rv_unref(success_val);

    return result;
}

RuntimeValue *system_env_get(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("envGet requires exactly 1 argument (variable name)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("envGet requires a string variable name", -1);
    }

    const char *var_name = args[0]->data.string.data;
    const char *value = getenv(var_name);

    if (value) {
        return rv_new_string(value);
    } else {
        return rv_new_null();
    }
}

RuntimeValue *system_env_set(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("envSet requires exactly 2 arguments (name, value)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("envSet requires string name and value", -1);
    }

    const char *name = args[0]->data.string.data;
    const char *value = args[1]->data.string.data;

    int result = setenv(name, value, 1);  // 1 = overwrite if exists
    if (result == 0) {
        return rv_new_boolean(true);
    } else {
        return rv_new_error("Failed to set environment variable", -1);
    }
}

RuntimeValue *system_env_list(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    // Access the global environ variable
    extern char **environ;

    RuntimeValue *env_array = rv_new_array();
    if (!env_array) {
        return rv_new_error("Failed to create environment array", -1);
    }

    for (int i = 0; environ[i] != NULL; i++) {
        RuntimeValue *env_str = rv_new_string(environ[i]);
        if (!env_str) {
            rv_unref(env_array);
            return rv_new_error("Failed to create environment string", -1);
        }
        rv_array_push(env_array, env_str);
        rv_unref(env_str);
    }

    return env_array;
}

RuntimeValue *system_process_list(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    // Create array to hold process information
    RuntimeValue *process_array = rv_new_array();

    // Use 'ps' command to get process list
    FILE *fp = popen("ps -eo pid,ppid,user,comm --no-headers", "r");
    if (fp == NULL) {
        rv_unref(process_array);
        return rv_new_error("Failed to execute ps command", -1);
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Parse ps output: PID PPID USER COMMAND
        int pid, ppid;
        char user[256], command[256];

        if (sscanf(line, "%d %d %255s %255s", &pid, &ppid, user, command) == 4) {
            // Create process object
            RuntimeValue *process_obj = rv_new_object();
            rv_object_set(process_obj, "pid", rv_new_number(pid));
            rv_object_set(process_obj, "ppid", rv_new_number(ppid));
            rv_object_set(process_obj, "user", rv_new_string(user));
            rv_object_set(process_obj, "command", rv_new_string(command));

            rv_array_push(process_array, process_obj);
            rv_unref(process_obj);
        }
    }

    pclose(fp);
    return process_array;
}

RuntimeValue *system_process_kill(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("processKill requires 1 or 2 arguments (pid, [signal])", -1);
    }

    if (args[0]->type != RV_NUMBER) {
        return rv_new_error("processKill: first argument must be a number (pid)", -1);
    }

    int pid = (int)args[0]->data.number;
    int signal = 15;  // SIGTERM by default

    if (argc >= 2) {
        if (args[1]->type != RV_NUMBER) {
            return rv_new_error("processKill: second argument must be a number (signal)", -1);
        }
        signal = (int)args[1]->data.number;
    }

    // Use kill command for cross-platform compatibility
    char kill_cmd[128];
    snprintf(kill_cmd, sizeof(kill_cmd), "kill -%d %d", signal, pid);

    int result = system(kill_cmd);
    if (result == 0) {
        return rv_new_boolean(true);
    } else {
        return rv_new_boolean(false);
    }
}

RuntimeValue *system_filesystem_list(RuntimeValue **args, size_t argc)
{
    const char *path = ".";  // Default to current directory

    if (argc >= 1) {
        if (args[0]->type != RV_STRING) {
            return rv_new_error("filesystemList: path must be a string", -1);
        }
        path = args[0]->data.string.data;
    }

    RuntimeValue *files_array = rv_new_array();

    DIR *dir = opendir(path);
    if (dir == NULL) {
        rv_unref(files_array);
        return rv_new_error("Failed to open directory", -1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Create full path for stat
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat file_stat;
        if (stat(full_path, &file_stat) == 0) {
            RuntimeValue *file_obj = rv_new_object();
            rv_object_set(file_obj, "name", rv_new_string(entry->d_name));
            rv_object_set(file_obj, "size", rv_new_number(file_stat.st_size));
            rv_object_set(file_obj, "isDirectory", rv_new_boolean(S_ISDIR(file_stat.st_mode)));
            rv_object_set(file_obj, "isFile", rv_new_boolean(S_ISREG(file_stat.st_mode)));
            rv_object_set(file_obj, "modified", rv_new_number(file_stat.st_mtime));

            rv_array_push(files_array, file_obj);
            rv_unref(file_obj);
        }
    }

    closedir(dir);
    return files_array;
}

RuntimeValue *system_filesystem_info(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("filesystemInfo requires 1 argument (path)", -1);
    }

    if (args[0]->type != RV_STRING) {
        return rv_new_error("filesystemInfo: path must be a string", -1);
    }

    const char *path = args[0]->data.string.data;
    struct stat file_stat;

    if (stat(path, &file_stat) != 0) {
        return rv_new_error("Failed to get file information", -1);
    }

    RuntimeValue *info_obj = rv_new_object();
    rv_object_set(info_obj, "path", rv_new_string(path));
    rv_object_set(info_obj, "size", rv_new_number(file_stat.st_size));
    rv_object_set(info_obj, "isDirectory", rv_new_boolean(S_ISDIR(file_stat.st_mode)));
    rv_object_set(info_obj, "isFile", rv_new_boolean(S_ISREG(file_stat.st_mode)));
    rv_object_set(info_obj, "isLink", rv_new_boolean(S_ISLNK(file_stat.st_mode)));
    rv_object_set(info_obj, "permissions", rv_new_number(file_stat.st_mode & 0777));
    rv_object_set(info_obj, "created", rv_new_number(file_stat.st_ctime));
    rv_object_set(info_obj, "modified", rv_new_number(file_stat.st_mtime));
    rv_object_set(info_obj, "accessed", rv_new_number(file_stat.st_atime));
    rv_object_set(info_obj, "uid", rv_new_number(file_stat.st_uid));
    rv_object_set(info_obj, "gid", rv_new_number(file_stat.st_gid));

    return info_obj;
}

RuntimeValue *system_hardware_cpu(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    RuntimeValue *cpu_obj = rv_new_object();

    // Get CPU information from /proc/cpuinfo
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp != NULL) {
        char line[512];
        char model_name[256] = "Unknown";
        int cores = 0;
        double mhz = 0.0;

        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    sscanf(colon + 1, "%255[^\n]", model_name);
                    // Trim leading spaces
                    char *start = model_name;
                    while (*start == ' ')
                        start++;
                    if (start != model_name) {
                        memmove(model_name, start, strlen(start) + 1);
                    }
                }
            } else if (strncmp(line, "processor", 9) == 0) {
                cores++;
            } else if (strncmp(line, "cpu MHz", 7) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    mhz = atof(colon + 1);
                }
            }
        }

        fclose(fp);

        rv_object_set(cpu_obj, "model", rv_new_string(model_name));
        rv_object_set(cpu_obj, "cores", rv_new_number(cores));
        rv_object_set(cpu_obj, "mhz", rv_new_number(mhz));
    } else {
        rv_object_set(cpu_obj, "model", rv_new_string("Unknown"));
        rv_object_set(cpu_obj, "cores", rv_new_number(0));
        rv_object_set(cpu_obj, "mhz", rv_new_number(0));
    }

    return cpu_obj;
}

RuntimeValue *system_hardware_memory(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    RuntimeValue *memory_obj = rv_new_object();

    // Get memory information from /proc/meminfo
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp != NULL) {
        char line[256];
        long total_kb = 0, free_kb = 0, available_kb = 0, buffers_kb = 0, cached_kb = 0;

        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                sscanf(line, "MemTotal: %ld kB", &total_kb);
            } else if (strncmp(line, "MemFree:", 8) == 0) {
                sscanf(line, "MemFree: %ld kB", &free_kb);
            } else if (strncmp(line, "MemAvailable:", 13) == 0) {
                sscanf(line, "MemAvailable: %ld kB", &available_kb);
            } else if (strncmp(line, "Buffers:", 8) == 0) {
                sscanf(line, "Buffers: %ld kB", &buffers_kb);
            } else if (strncmp(line, "Cached:", 7) == 0) {
                sscanf(line, "Cached: %ld kB", &cached_kb);
            }
        }

        fclose(fp);

        // Convert to bytes and set values
        rv_object_set(memory_obj, "totalBytes", rv_new_number(total_kb * 1024));
        rv_object_set(memory_obj, "freeBytes", rv_new_number(free_kb * 1024));
        rv_object_set(memory_obj, "availableBytes", rv_new_number(available_kb * 1024));
        rv_object_set(memory_obj, "usedBytes", rv_new_number((total_kb - free_kb) * 1024));
        rv_object_set(memory_obj, "buffersBytes", rv_new_number(buffers_kb * 1024));
        rv_object_set(memory_obj, "cachedBytes", rv_new_number(cached_kb * 1024));

        // Calculate percentage used
        if (total_kb > 0) {
            double used_percent = ((double)(total_kb - available_kb) / total_kb) * 100.0;
            rv_object_set(memory_obj, "usedPercent", rv_new_number(used_percent));
        } else {
            rv_object_set(memory_obj, "usedPercent", rv_new_number(0));
        }
    } else {
        rv_object_set(memory_obj, "totalBytes", rv_new_number(0));
        rv_object_set(memory_obj, "freeBytes", rv_new_number(0));
        rv_object_set(memory_obj, "availableBytes", rv_new_number(0));
        rv_object_set(memory_obj, "usedBytes", rv_new_number(0));
        rv_object_set(memory_obj, "buffersBytes", rv_new_number(0));
        rv_object_set(memory_obj, "cachedBytes", rv_new_number(0));
        rv_object_set(memory_obj, "usedPercent", rv_new_number(0));
    }

    return memory_obj;
}