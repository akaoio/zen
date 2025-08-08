#ifndef ZEN_HTTP_H
#define ZEN_HTTP_H

#include "zen/types/value.h"

#include <stddef.h>

/**
 * @file http.h
 * @brief HTTP client library for ZEN language with natural language interface
 * @note Part of Phase 2A implementation - HTTP, modules, advanced data structures
 * @note Uses libcurl for HTTP operations with <100ms response time target
 * @note Supports natural ZEN syntax: http get "https://api.example.com"
 */

/**
 * @brief Structure for HTTP headers
 * Used for managing HTTP request and response headers
 */
typedef struct HttpHeaders {
    char **keys;      ///< Array of header keys
    char **values;    ///< Array of header values
    size_t count;     ///< Number of headers
    size_t capacity;  ///< Allocated capacity
} HttpHeaders;

// HTTP Request Functions (MANIFEST.json signatures)

/**
 * @brief Perform HTTP GET request with optional headers
 * @param url_value String value containing URL to request
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 * @note Natural ZEN syntax: http get "https://api.example.com"
 * @note Target response time: <100ms for typical requests
 * @note Supports JSON responses with automatic parsing
 */
Value *http_get(const Value *url_value, const Value *headers_value);

/**
 * @brief Perform HTTP POST request with data and headers
 * @param url_value String value containing URL to request
 * @param data_value Value containing POST data (string, object, or array)
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 * @note Natural ZEN syntax: http post "https://api.example.com" data headers
 * @note Automatically sets Content-Type based on data type
 * @note JSON data is automatically serialized
 */
Value *http_post(const Value *url_value, const Value *data_value, const Value *headers_value);

/**
 * @brief Perform HTTP PUT request with data and headers
 * @param url_value String value containing URL to request
 * @param data_value Value containing PUT data (string, object, or array)
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 * @note Natural ZEN syntax: http put "https://api.example.com" data headers
 * @note Automatically sets Content-Type based on data type
 * @note JSON data is automatically serialized
 */
Value *http_put(const Value *url_value, const Value *data_value, const Value *headers_value);

/**
 * @brief Perform HTTP DELETE request with optional headers
 * @param url_value String value containing URL to request
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 * @note Natural ZEN syntax: http delete "https://api.example.com" headers
 * @note Target response time: <100ms for typical requests
 */
Value *http_delete(const Value *url_value, const Value *headers_value);

/**
 * @brief Configure global HTTP request timeout
 * @param timeout_seconds Timeout in seconds (long value)
 * @return void
 * @note Natural ZEN syntax: http timeout 30
 * @note Default timeout is 30 seconds
 * @note Applies to all subsequent HTTP requests
 */
void http_configure_timeout(long timeout_seconds);

// Stdlib wrapper functions for ZEN runtime integration

/**
 * @brief HTTP GET wrapper for stdlib integration
 * @param args Arguments array: [url_string, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
Value *stdlib_http_get(Value **args, size_t argc);

/**
 * @brief HTTP POST wrapper for stdlib integration
 * @param args Arguments array: [url_string, data_value, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
Value *stdlib_http_post(Value **args, size_t argc);

/**
 * @brief HTTP PUT wrapper for stdlib integration
 * @param args Arguments array: [url_string, data_value, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
Value *stdlib_http_put(Value **args, size_t argc);

/**
 * @brief HTTP DELETE wrapper for stdlib integration
 * @param args Arguments array: [url_string, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
Value *stdlib_http_delete(Value **args, size_t argc);

/**
 * @brief HTTP timeout configuration wrapper for stdlib integration
 * @param args Arguments array: [timeout_seconds]
 * @param argc Number of arguments
 * @return Null value
 */
Value *stdlib_http_timeout(Value **args, size_t argc);

// Internal helper functions

/**
 * @brief Initialize HTTP headers structure
 * @return Pointer to initialized HttpHeaders structure
 */
HttpHeaders *http_headers_new(void);

/**
 * @brief Add header to HttpHeaders structure
 * @param headers Pointer to HttpHeaders structure
 * @param key Header key
 * @param value Header value
 * @return true on success, false on failure
 */
bool http_headers_add(HttpHeaders *headers, const char *key, const char *value);

/**
 * @brief Convert ZEN object value to HttpHeaders structure
 * @param headers_value ZEN object containing headers
 * @return Pointer to HttpHeaders structure or NULL on error
 */
HttpHeaders *http_headers_from_value(const Value *headers_value);

/**
 * @brief Convert HttpHeaders structure to ZEN object value
 * @param headers Pointer to HttpHeaders structure
 * @return ZEN object value containing headers
 */
Value *http_headers_to_value(const HttpHeaders *headers);

/**
 * @brief Free HttpHeaders structure and its contents
 * @param headers Pointer to HttpHeaders structure
 */
void http_headers_free(HttpHeaders *headers);

/**
 * @brief Create HTTP response object
 * @param status HTTP status code
 * @param body Response body as string
 * @param headers Response headers
 * @return ZEN object value containing response data
 */
Value *http_create_response_object(long status, const char *body, const HttpHeaders *headers);

/**
 * @brief Serialize ZEN value to request body string
 * @param data_value ZEN value to serialize
 * @param content_type Output parameter for determined content type
 * @return Serialized string (caller must free) or NULL on error
 */
char *http_serialize_request_data(const Value *data_value, char **content_type);

#endif /* ZEN_HTTP_H */