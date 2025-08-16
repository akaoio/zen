/**
 * @file stream.c
 * @brief Streaming file operations for handling GB-sized files
 * 
 * This module provides streaming and memory-mapped file operations
 * to handle files of any size without loading them entirely into memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/stream.h"

typedef struct {
    int fd;                    // File descriptor
    char *mapped;              // Memory-mapped region
    size_t size;               // File size
    size_t position;           // Current read position
    bool is_mapped;            // Whether using mmap or streaming
    FILE *file;                // For streaming mode
    char *buffer;              // Read buffer for streaming
    size_t buffer_size;        // Buffer size
    size_t buffer_pos;         // Position in buffer
    size_t buffer_len;         // Valid data in buffer
} FileStream;

typedef struct {
    FileStream *stream;        // Underlying file stream
    size_t *index;            // Array of object/array positions
    size_t index_size;        // Number of indexed positions
    size_t index_capacity;    // Capacity of index array
    bool indexed;             // Whether index has been built
} JsonStream;

// Create a new file stream
FileStream *stream_open(const char *filename) {
    FileStream *stream = memory_alloc(sizeof(FileStream));
    if (!stream) return NULL;
    
    memset(stream, 0, sizeof(FileStream));
    
    // Try to open file
    stream->fd = open(filename, O_RDONLY);
    if (stream->fd < 0) {
        memory_free(stream);
        return NULL;
    }
    
    // Get file size
    struct stat st;
    if (fstat(stream->fd, &st) < 0) {
        close(stream->fd);
        memory_free(stream);
        return NULL;
    }
    
    stream->size = st.st_size;
    
    // For files > 100MB, use streaming instead of mmap
    if (stream->size > 100 * 1024 * 1024) {
        stream->is_mapped = false;
        stream->file = fdopen(stream->fd, "r");
        stream->buffer_size = 64 * 1024; // 64KB buffer
        stream->buffer = memory_alloc(stream->buffer_size);
        stream->buffer_pos = 0;
        stream->buffer_len = 0;
    } else {
        // Use memory mapping for smaller files
        stream->mapped = mmap(NULL, stream->size, PROT_READ, MAP_PRIVATE, stream->fd, 0);
        if (stream->mapped == MAP_FAILED) {
            close(stream->fd);
            memory_free(stream);
            return NULL;
        }
        stream->is_mapped = true;
    }
    
    stream->position = 0;
    return stream;
}

// Close file stream
void stream_close(FileStream *stream) {
    if (!stream) return;
    
    if (stream->is_mapped && stream->mapped) {
        munmap(stream->mapped, stream->size);
    }
    
    if (stream->buffer) {
        memory_free(stream->buffer);
    }
    
    if (stream->file) {
        fclose(stream->file);
    } else if (stream->fd >= 0) {
        close(stream->fd);
    }
    
    memory_free(stream);
}

// Read next character from stream
int stream_getc(FileStream *stream) {
    if (!stream) return EOF;
    
    if (stream->position >= stream->size) {
        return EOF;
    }
    
    if (stream->is_mapped) {
        return stream->mapped[stream->position++];
    } else {
        // Streaming mode - use buffered reading
        if (stream->buffer_pos >= stream->buffer_len) {
            // Refill buffer
            stream->buffer_len = fread(stream->buffer, 1, stream->buffer_size, stream->file);
            stream->buffer_pos = 0;
            if (stream->buffer_len == 0) {
                return EOF;
            }
        }
        stream->position++;
        return stream->buffer[stream->buffer_pos++];
    }
}

// Peek at next character without advancing
int stream_peek(FileStream *stream) {
    if (!stream || stream->position >= stream->size) {
        return EOF;
    }
    
    if (stream->is_mapped) {
        return stream->mapped[stream->position];
    } else {
        if (stream->buffer_pos >= stream->buffer_len) {
            // Need to refill buffer
            stream->buffer_len = fread(stream->buffer, 1, stream->buffer_size, stream->file);
            stream->buffer_pos = 0;
            if (stream->buffer_len == 0) {
                return EOF;
            }
        }
        return stream->buffer[stream->buffer_pos];
    }
}

// Skip whitespace
void stream_skip_whitespace(FileStream *stream) {
    int c;
    while ((c = stream_peek(stream)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
        stream_getc(stream);
    }
}

// Parse a JSON value lazily from stream
RuntimeValue *stream_parse_json_value(FileStream *stream);

// Parse JSON string from stream
RuntimeValue *stream_parse_json_string(FileStream *stream) {
    if (stream_getc(stream) != '"') return NULL;
    
    size_t capacity = 256;
    size_t len = 0;
    char *str = memory_alloc(capacity);
    
    int c;
    while ((c = stream_getc(stream)) != EOF && c != '"') {
        if (c == '\\') {
            c = stream_getc(stream);
            switch (c) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                default: break;
            }
        }
        
        if (len + 1 >= capacity) {
            capacity *= 2;
            char *new_str = memory_realloc(str, capacity);
            if (!new_str) {
                memory_free(str);
                return NULL;
            }
            str = new_str;
        }
        
        str[len++] = c;
    }
    
    str[len] = '\0';
    RuntimeValue *result = rv_new_string(str);
    memory_free(str);
    return result;
}

// Parse JSON number from stream
RuntimeValue *stream_parse_json_number(FileStream *stream) {
    char buffer[64];
    size_t len = 0;
    int c;
    
    while ((c = stream_peek(stream)) != EOF && len < 63) {
        if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
            buffer[len++] = stream_getc(stream);
        } else {
            break;
        }
    }
    
    buffer[len] = '\0';
    return rv_new_number(atof(buffer));
}

// Parse JSON object from stream (lazy - only parse requested keys)
RuntimeValue *stream_parse_json_object_lazy(FileStream *stream, const char *target_key) {
    if (stream_getc(stream) != '{') return NULL;
    
    RuntimeValue *result = NULL;
    
    // If no target key, parse entire object
    if (!target_key) {
        result = rv_new_object();
    }
    
    stream_skip_whitespace(stream);
    
    while (stream_peek(stream) != '}' && stream_peek(stream) != EOF) {
        // Parse key
        if (stream_peek(stream) != '"') {
            if (result) rv_unref(result);
            return NULL;
        }
        RuntimeValue *key = stream_parse_json_string(stream);
        if (!key) {
            if (result) rv_unref(result);
            return NULL;
        }
        
        stream_skip_whitespace(stream);
        if (stream_getc(stream) != ':') {
            rv_unref(key);
            if (result) rv_unref(result);
            return NULL;
        }
        
        stream_skip_whitespace(stream);
        
        const char *key_str = rv_get_string(key);
        
        // Check if this is the key we're looking for
        if (target_key && strcmp(key_str, target_key) == 0) {
            rv_unref(key);
            // Parse and return this value
            return stream_parse_json_value(stream);
        }
        
        // If parsing full object, parse the value
        if (!target_key) {
            RuntimeValue *value = stream_parse_json_value(stream);
            if (value) {
                rv_object_set(result, key_str, value);
                rv_unref(value);
            }
        } else {
            // Skip this value since it's not what we want
            int depth = 0;
            int in_string = 0;
            int escape = 0;
            int c;
            
            c = stream_peek(stream);
            if (c == '"') {
                // String value - parse it properly
                stream_parse_json_string(stream);
            } else if (c == '{' || c == '[') {
                // Object or array - skip with depth tracking
                depth = 1;
                stream_getc(stream); // consume opening bracket
                
                while ((c = stream_getc(stream)) != EOF && depth > 0) {
                    if (!in_string) {
                        if (c == '"') in_string = 1;
                        else if (c == '{' || c == '[') depth++;
                        else if (c == '}' || c == ']') depth--;
                    } else {
                        if (c == '\\' && !escape) {
                            escape = 1;
                        } else if (c == '"' && !escape) {
                            in_string = 0;
                        } else {
                            escape = 0;
                        }
                    }
                }
            } else {
                // Number, boolean, or null - skip until delimiter
                while ((c = stream_peek(stream)) != EOF && 
                       c != ',' && c != '}' && c != ']' && 
                       c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    stream_getc(stream);
                }
            }
        }
        
        rv_unref(key);
        
        stream_skip_whitespace(stream);
        if (stream_peek(stream) == ',') {
            stream_getc(stream);
            stream_skip_whitespace(stream);
        }
    }
    
    // Consume closing brace if we parsed the full object
    if (!target_key && stream_peek(stream) == '}') {
        stream_getc(stream);
    }
    
    return target_key ? NULL : result; // Return NULL if key not found, or the full object
}

// Parse any JSON value from stream
RuntimeValue *stream_parse_json_value(FileStream *stream) {
    stream_skip_whitespace(stream);
    
    int c = stream_peek(stream);
    if (c == '"') {
        return stream_parse_json_string(stream);
    } else if (c == '{') {
        // For now, parse full object (could be optimized)
        // In a real implementation, would return a lazy object proxy
        return stream_parse_json_object_lazy(stream, NULL);
    } else if (c == '[') {
        // Skip array parsing for now
        return rv_new_array();
    } else if ((c >= '0' && c <= '9') || c == '-') {
        return stream_parse_json_number(stream);
    } else if (c == 't' || c == 'f') {
        // Boolean
        char buffer[6];
        size_t i = 0;
        while (i < 5 && (c = stream_getc(stream)) != EOF && c >= 'a' && c <= 'z') {
            buffer[i++] = c;
        }
        buffer[i] = '\0';
        if (strcmp(buffer, "true") == 0) return rv_new_boolean(true);
        if (strcmp(buffer, "false") == 0) return rv_new_boolean(false);
    } else if (c == 'n') {
        // null
        char buffer[5];
        for (int i = 0; i < 4; i++) {
            buffer[i] = stream_getc(stream);
        }
        buffer[4] = '\0';
        if (strcmp(buffer, "null") == 0) return rv_new_null();
    }
    
    return NULL;
}

// High-level function: Get a value from a JSON file without loading it all
RuntimeValue *stream_json_get(const char *filename, const char *path) {
    FileStream *stream = stream_open(filename);
    if (!stream) return NULL;
    
    // Skip to start of JSON
    stream_skip_whitespace(stream);
    
    // Parse path (e.g., "users.alice.name")
    char *path_copy = memory_strdup(path);
    char *token = strtok(path_copy, ".");
    
    RuntimeValue *result = NULL;
    
    // For now, simple implementation - just get first level
    if (stream_peek(stream) == '{') {
        result = stream_parse_json_object_lazy(stream, token);
    }
    
    memory_free(path_copy);
    stream_close(stream);
    
    return result ? result : rv_new_null();
}

// Streaming put operation (append to file or modify in place)
bool stream_json_put(const char *filename, const char *path, RuntimeValue *value) {
    // This is complex - would need to either:
    // 1. Rewrite the file with the modification
    // 2. Use a write-ahead log
    // 3. Use a database-like structure
    
    // Avoid unused parameter warnings
    (void)filename;
    (void)path;
    (void)value;
    
    // For now, return false (not implemented)
    return false;
}

// Internal API functions used by get/put methods
RuntimeValue *stream_get_internal(const char *filename, const char *path) {
    return stream_json_get(filename, path);
}

RuntimeValue *stream_put_internal(const char *filename, const char *path, RuntimeValue *value) {
    bool success = stream_json_put(filename, path, value);
    return rv_new_boolean(success);
}