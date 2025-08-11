/*
 * io_minimal.c
 * Minimal IO functions for RuntimeValue system
 * Only includes essential functions needed for basic ZEN operation
 */

#define _GNU_SOURCE
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Read entire file to string
 * @param filepath Path to the file to read
 * @return Newly allocated string containing file contents, or NULL on error
 */
char *io_read_file_internal(const char *filepath)
{
    if (!filepath) {
        return NULL;
    }

    char *buffer = NULL;
    long length;

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return NULL;
    }

    // Get file size
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    length = ftell(f);
    if (length < 0) {
        fclose(f);
        return NULL;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    // Allocate buffer
    buffer = memory_alloc(length + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    // Read file
    size_t read_size = fread(buffer, 1, length, f);
    if (read_size != (size_t)length) {
        memory_free(buffer);
        fclose(f);
        return NULL;
    }

    buffer[length] = '\0';
    fclose(f);

    return buffer;
}

/**
 * @brief Print RuntimeValue to stdout with newline
 * @param value RuntimeValue to print
 */
void io_print_internal(const RuntimeValue *value)
{
    if (!value) {
        printf("null\n");
        return;
    }

    char *str = rv_to_string((RuntimeValue *)value);
    if (str) {
        printf("%s\n", str);
        memory_free(str);
    } else {
        printf("null\n");
    }
}

/**
 * @brief Print RuntimeValue to stdout without newline
 * @param value RuntimeValue to print
 */
void io_print_no_newline_internal(const RuntimeValue *value)
{
    if (!value) {
        printf("null");
        return;
    }

    char *str = rv_to_string((RuntimeValue *)value);
    if (str) {
        printf("%s", str);
        memory_free(str);
    } else {
        printf("null");
    }
}

/**
 * @brief Write string to file
 * @param filepath Path to the file to write
 * @param content Content to write
 * @return true on success, false on error
 */
bool io_write_file_internal(const char *filepath, const char *content)
{
    if (!filepath || !content) {
        return false;
    }

    FILE *f = fopen(filepath, "w");
    if (!f) {
        return false;
    }

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);

    return written == len;
}