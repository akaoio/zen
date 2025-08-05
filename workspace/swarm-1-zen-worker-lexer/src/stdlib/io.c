#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Read entire file contents into a string
 * @param path The file path to read
 * @return char* The file contents (caller must free)
 */
char* io_read_file(const char* path)
{
    return NULL; /* TODO: Implement */
}

/**
 * @brief Write content to a file
 * @param path The file path to write to
 * @param content The content to write
 * @return bool True if successful, false otherwise
 */
bool io_write_file(const char* path, const char* content)
{
    return false; /* TODO: Implement */
}

/**
 * @brief Check if a file exists
 * @param path The file path to check
 * @return bool True if file exists, false otherwise
 */
bool io_file_exists(const char* path)
{
    return false; /* TODO: Implement */
}