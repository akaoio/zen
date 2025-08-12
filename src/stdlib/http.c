/*
 * http.c
 * ZEN HTTP Client Library with libcurl integration
 *
 * Provides natural language HTTP operations for ZEN:
 * - http get "https://api.example.com"
 * - http post "https://api.example.com" data headers
 * - http timeout 30
 *
 * Features:
 * - libcurl integration for HTTP operations
 * - <100ms response time target for typical requests
 * - JSON request/response handling
 * - Memory management with reference counting
 * - Comprehensive error handling
 */

#define _GNU_SOURCE  // For strdup
#include "zen/stdlib/http.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"
#include "zen/stdlib/convert.h"
#include "zen/stdlib/json.h"
// Array and object functions are in runtime_value.h

#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global configuration
static long global_timeout = 30;  // Default 30 seconds
static bool curl_initialized = false;

/**
 * @brief Structure for collecting HTTP response data
 */
struct HttpResponse {
    char *data;       ///< Response body data
    size_t size;      ///< Current size of response data
    size_t capacity;  ///< Allocated capacity
};

/**
 * @brief Structure for collecting HTTP headers
 */
struct HttpHeaderCollector {
    HttpHeaders *headers;  ///< Headers structure to populate
};

/**
 * @brief Callback function for curl to write response data
 * @param contents Data received from server
 * @param size Size multiplier
 * @param nmemb Number of items
 * @param userp Pointer to HttpResponse structure
 * @return Number of bytes processed
 */
static size_t http_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct HttpResponse *response = (struct HttpResponse *)userp;

    // Ensure we have enough capacity
    if (response->size + realsize + 1 > response->capacity) {
        size_t new_capacity = (response->capacity == 0) ? 1024 : response->capacity * 2;
        while (new_capacity < response->size + realsize + 1) {
            new_capacity *= 2;
        }

        char *new_data = memory_realloc(response->data, new_capacity);
        if (!new_data) {
            return 0;  // Out of memory
        }
        response->data = new_data;
        response->capacity = new_capacity;
    }

    // Copy the new data
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = '\0';  // Null terminate

    return realsize;
}

/**
 * @brief Callback function for curl to collect response headers
 * @param buffer Header line from server
 * @param size Size multiplier
 * @param nitems Number of items
 * @param userp Pointer to HttpHeaderCollector structure
 * @return Number of bytes processed
 */
static size_t http_header_callback(char *buffer, size_t size, size_t nitems, void *userp)
{
    size_t realsize = size * nitems;
    struct HttpHeaderCollector *collector = (struct HttpHeaderCollector *)userp;

    // Skip status line and empty lines
    if (realsize < 3 || strncmp(buffer, "HTTP/", 5) == 0) {
        return realsize;
    }

    // Find colon separator
    char *colon = strchr(buffer, ':');
    if (!colon) {
        return realsize;  // Not a valid header
    }

    // Extract key and value
    size_t key_len = colon - buffer;
    char *key = memory_alloc(key_len + 1);
    if (!key)
        return 0;

    strncpy(key, buffer, key_len);
    key[key_len] = '\0';

    // Skip colon and whitespace
    char *value_start = colon + 1;
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++;
    }

    // Find end of value (remove \r\n)
    char *value_end = value_start + strlen(value_start);
    while (value_end > value_start && (value_end[-1] == '\r' || value_end[-1] == '\n')) {
        value_end--;
    }

    size_t value_len = value_end - value_start;
    char *value = memory_alloc(value_len + 1);
    if (!value) {
        memory_free(key);
        return 0;
    }

    strncpy(value, value_start, value_len);
    value[value_len] = '\0';

    // Add to headers collection
    http_headers_add(collector->headers, key, value);

    memory_free(key);
    memory_free(value);

    return realsize;
}

/**
 * @brief Initialize libcurl if not already done
 * @return true on success, false on failure
 */
static bool http_init_curl(void)
{
    if (curl_initialized) {
        return true;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return false;
    }

    curl_initialized = true;
    return true;
}

/**
 * @brief Cleanup libcurl resources
 */
static void http_cleanup_curl(void) __attribute__((unused));
static void http_cleanup_curl(void)
{
    if (curl_initialized) {
        curl_global_cleanup();
        curl_initialized = false;
    }
}

// HttpHeaders implementation

/**
 * @brief Initialize HTTP headers structure
 * @return Pointer to initialized HttpHeaders structure
 */
HttpHeaders *http_headers_new(void)
{
    HttpHeaders *headers = memory_alloc(sizeof(HttpHeaders));
    if (!headers)
        return NULL;

    headers->keys = NULL;
    headers->values = NULL;
    headers->count = 0;
    headers->capacity = 0;

    return headers;
}

/**
 * @brief Add header to HttpHeaders structure
 * @param headers Pointer to HttpHeaders structure
 * @param key Header key
 * @param value Header value
 * @return true on success, false on failure
 */
bool http_headers_add(HttpHeaders *headers, const char *key, const char *value)
{
    if (!headers || !key || !value) {
        return false;
    }

    // Expand arrays if needed
    if (headers->count >= headers->capacity) {
        size_t new_capacity = headers->capacity == 0 ? 8 : headers->capacity * 2;

        char **new_keys = memory_realloc(headers->keys, new_capacity * sizeof(char *));
        char **new_values = memory_realloc(headers->values, new_capacity * sizeof(char *));

        if (!new_keys || !new_values) {
            return false;
        }

        headers->keys = new_keys;
        headers->values = new_values;
        headers->capacity = new_capacity;
    }

    // Duplicate strings
    headers->keys[headers->count] = memory_strdup(key);
    headers->values[headers->count] = memory_strdup(value);

    if (!headers->keys[headers->count] || !headers->values[headers->count]) {
        if (headers->keys[headers->count])
            memory_free(headers->keys[headers->count]);
        if (headers->values[headers->count])
            memory_free(headers->values[headers->count]);
        return false;
    }

    headers->count++;
    return true;
}

/**
 * @brief Convert ZEN object value to HttpHeaders structure
 * @param headers_value ZEN object containing headers
 * @return Pointer to HttpHeaders structure or NULL on error
 */
HttpHeaders *http_headers_from_value(const RuntimeValue *headers_value)
{
    if (!headers_value || headers_value->type != RV_OBJECT) {
        return NULL;
    }

    HttpHeaders *headers = http_headers_new();
    if (!headers)
        return NULL;

    // Iterate through object properties
    // Iterate through object properties
    for (size_t i = 0; i < rv_object_size((RuntimeValue *)headers_value); i++) {
        const char *key = rv_object_get_key_at((RuntimeValue *)headers_value, i);
        RuntimeValue *value = rv_object_get((RuntimeValue *)headers_value, key);
        if (key && value) {
            // Convert value to string
            char *value_str = rv_to_string(value);
            if (value_str) {
                if (!http_headers_add(headers, key, value_str)) {
                    memory_free(value_str);
                    http_headers_free(headers);
                    return NULL;
                }
                memory_free(value_str);
            }
        }
    }

    return headers;
}

/**
 * @brief Convert HttpHeaders structure to ZEN object value
 * @param headers Pointer to HttpHeaders structure
 * @return ZEN object value containing headers
 */
RuntimeValue *http_headers_to_value(const HttpHeaders *headers)
{
    if (!headers) {
        return rv_new_object();
    }

    RuntimeValue *obj = rv_new_object();
    if (!obj)
        return NULL;

    for (size_t i = 0; i < headers->count; i++) {
        RuntimeValue *value = rv_new_string(headers->values[i]);
        if (value) {
            rv_object_set(obj, headers->keys[i], value);
            rv_unref(value);  // rv_object_set adds its own reference
        }
    }

    return obj;
}

/**
 * @brief Free HttpHeaders structure and its contents
 * @param headers Pointer to HttpHeaders structure
 */
void http_headers_free(HttpHeaders *headers)
{
    if (!headers)
        return;

    for (size_t i = 0; i < headers->count; i++) {
        memory_free(headers->keys[i]);
        memory_free(headers->values[i]);
    }

    memory_free(headers->keys);
    memory_free(headers->values);
    memory_free(headers);
}

/**
 * @brief Create HTTP response object
 * @param status HTTP status code
 * @param body Response body as string
 * @param headers Response headers
 * @return ZEN object value containing response data
 */
RuntimeValue *http_create_response_object(long status, const char *body, const HttpHeaders *headers)
{
    RuntimeValue *response = rv_new_object();
    if (!response)
        return NULL;

    // Add status
    RuntimeValue *status_val = rv_new_number((double)status);
    if (status_val) {
        rv_object_set(response, "status", status_val);
        rv_unref(status_val);
    }

    // Add body - try to parse as JSON first
    RuntimeValue *body_val = NULL;
    if (body && strlen(body) > 0) {
        // Try JSON parsing first
        body_val = json_parse(body);
        if (!body_val || body_val->type == RV_ERROR) {
            // If JSON parsing fails, use as string
            if (body_val)
                rv_unref(body_val);
            body_val = rv_new_string(body);
        }
    } else {
        body_val = rv_new_string("");
    }

    if (body_val) {
        rv_object_set(response, "body", body_val);
        rv_unref(body_val);
    }

    // Add headers
    RuntimeValue *headers_val = http_headers_to_value(headers);
    if (headers_val) {
        rv_object_set(response, "headers", headers_val);
        rv_unref(headers_val);
    }

    return response;
}

/**
 * @brief Serialize ZEN value to request body string
 * @param data_value ZEN value to serialize
 * @param content_type Output parameter for determined content type
 * @return Serialized string (caller must free) or NULL on error
 */
char *http_serialize_request_data(const RuntimeValue *data_value, char **content_type)
{
    if (!data_value || !content_type) {
        return NULL;
    }

    *content_type = NULL;

    switch (data_value->type) {
    case RV_STRING: {
        const char *str = rv_get_string((RuntimeValue *)data_value);
        if (str) {
            *content_type = memory_strdup("text/plain");
            return memory_strdup(str);
        }
    } break;

    case RV_OBJECT:
    case RV_ARRAY: {
        char *json_str = json_stringify(data_value);
        if (json_str) {
            *content_type = memory_strdup("application/json");
            return json_str;
        }
        break;
    }

    default: {
        // Convert other types to string
        char *str = rv_to_string((RuntimeValue *)data_value);
        if (str) {
            *content_type = memory_strdup("text/plain");
            return str;
        }
        break;
    }
    }

    return NULL;
}

/**
 * @brief Perform HTTP request with libcurl
 * @param method HTTP method (GET, POST, PUT, DELETE)
 * @param url_value URL to request
 * @param data_value Request data (can be NULL)
 * @param headers_value Request headers (can be NULL)
 * @return Response object or error value
 */
static RuntimeValue *http_perform_request(const char *method,
                                          const RuntimeValue *url_value,
                                          const RuntimeValue *data_value,
                                          const RuntimeValue *headers_value)
{
    // Validate URL
    if (!url_value || url_value->type != RV_STRING) {
        return rv_new_error("http_request requires valid URL string", -1);
    }

    // Initialize curl if needed
    if (!http_init_curl()) {
        return rv_new_error("Failed to initialize HTTP library", -1);
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return rv_new_error("Failed to create HTTP request", -1);
    }

    // Initialize response structure
    struct HttpResponse response = {0};
    response.data = memory_alloc(1);
    if (!response.data) {
        curl_easy_cleanup(curl);
        return rv_new_error("Memory allocation failed", -1);
    }
    response.data[0] = '\0';
    response.capacity = 1;

    // Initialize header collector
    HttpHeaders *response_headers = http_headers_new();
    struct HttpHeaderCollector header_collector = {response_headers};

    RuntimeValue *result = NULL;
    struct curl_slist *curl_headers = NULL;
    char *request_body = NULL;

    // Configure basic curl options
    curl_easy_setopt(curl, CURLOPT_URL, rv_get_string((RuntimeValue *)url_value));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, http_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_collector);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, global_timeout);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ZEN-HTTP/1.0");

    // Configure method-specific options
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        if (data_value) {
            char *content_type = NULL;
            request_body = http_serialize_request_data(data_value, &content_type);

            if (request_body) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request_body));

                if (content_type) {
                    char header_line[512];
                    snprintf(header_line, sizeof(header_line), "Content-Type: %s", content_type);
                    curl_headers = curl_slist_append(curl_headers, header_line);
                    memory_free(content_type);
                }
            }
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else if (strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

        if (data_value) {
            char *content_type = NULL;
            request_body = http_serialize_request_data(data_value, &content_type);

            if (request_body) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request_body));

                if (content_type) {
                    char header_line[512];
                    snprintf(header_line, sizeof(header_line), "Content-Type: %s", content_type);
                    curl_headers = curl_slist_append(curl_headers, header_line);
                    memory_free(content_type);
                }
            }
        }
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    // Add custom headers
    if (headers_value) {
        HttpHeaders *req_headers = http_headers_from_value(headers_value);
        if (req_headers) {
            for (size_t i = 0; i < req_headers->count; i++) {
                char header_line[1024];
                snprintf(header_line,
                         sizeof(header_line),
                         "%s: %s",
                         req_headers->keys[i],
                         req_headers->values[i]);
                curl_headers = curl_slist_append(curl_headers, header_line);
            }
            http_headers_free(req_headers);
        }
    }

    if (curl_headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    }

    // Perform the request
    CURLcode curl_result = curl_easy_perform(curl);

    if (curl_result == CURLE_OK) {
        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        result = http_create_response_object(status_code, response.data, response_headers);
    } else {
        char error_msg[512];
        snprintf(error_msg,
                 sizeof(error_msg),
                 "HTTP request failed: %s",
                 curl_easy_strerror(curl_result));
        result = rv_new_error(error_msg, (int)curl_result);
    }

    // Cleanup
    if (curl_headers) {
        curl_slist_free_all(curl_headers);
    }
    if (request_body) {
        memory_free(request_body);
    }
    memory_free(response.data);
    http_headers_free(response_headers);
    curl_easy_cleanup(curl);

    return result;
}

// MANIFEST.json signature implementations

/**
 * @brief Perform HTTP GET request with optional headers
 * @param url_value String value containing URL to request
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 */
RuntimeValue *http_get(const RuntimeValue *url_value, const RuntimeValue *headers_value)
{
    return http_perform_request("GET", url_value, NULL, headers_value);
}

/**
 * @brief Perform HTTP POST request with data and headers
 * @param url_value String value containing URL to request
 * @param data_value Value containing POST data (string, object, or array)
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 */
RuntimeValue *http_post(const RuntimeValue *url_value,
                        const RuntimeValue *data_value,
                        const RuntimeValue *headers_value)
{
    return http_perform_request("POST", url_value, data_value, headers_value);
}

/**
 * @brief Perform HTTP PUT request with data and headers
 * @param url_value String value containing URL to request
 * @param data_value Value containing PUT data (string, object, or array)
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 */
RuntimeValue *http_put(const RuntimeValue *url_value,
                       const RuntimeValue *data_value,
                       const RuntimeValue *headers_value)
{
    return http_perform_request("PUT", url_value, data_value, headers_value);
}

/**
 * @brief Perform HTTP DELETE request with optional headers
 * @param url_value String value containing URL to request
 * @param headers_value Object value containing headers (optional, can be null)
 * @return Object value with response data (status, body, headers) or error value
 */
RuntimeValue *http_delete(const RuntimeValue *url_value, const RuntimeValue *headers_value)
{
    return http_perform_request("DELETE", url_value, NULL, headers_value);
}

/**
 * @brief Configure global HTTP request timeout
 * @param timeout_seconds Timeout in seconds (long value)
 */
void http_configure_timeout(long timeout_seconds)
{
    if (timeout_seconds > 0) {
        global_timeout = timeout_seconds;
    }
}

// Stdlib wrapper functions for ZEN runtime integration

/**
 * @brief HTTP GET wrapper for stdlib integration
 * @param args Arguments array: [url_string, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
RuntimeValue *http_get_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("httpGet requires URL string", -1);
    }

    const RuntimeValue *headers = (argc >= 2) ? args[1] : NULL;
    return http_get(args[0], headers);
}

/**
 * @brief HTTP POST wrapper for stdlib integration
 * @param args Arguments array: [url_string, data_value, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
RuntimeValue *http_post_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 2) {
        return rv_new_error("httpPost requires URL and data", -1);
    }

    const RuntimeValue *headers = (argc >= 3) ? args[2] : NULL;
    return http_post(args[0], args[1], headers);
}

/**
 * @brief HTTP PUT wrapper for stdlib integration
 * @param args Arguments array: [url_string, data_value, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
RuntimeValue *http_put_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 2) {
        return rv_new_error("httpPut requires URL and data", -1);
    }

    const RuntimeValue *headers = (argc >= 3) ? args[2] : NULL;
    return http_put(args[0], args[1], headers);
}

/**
 * @brief HTTP DELETE wrapper for stdlib integration
 * @param args Arguments array: [url_string, headers_object(optional)]
 * @param argc Number of arguments
 * @return Response object or error value
 */
RuntimeValue *http_delete_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("httpDelete requires URL string", -1);
    }

    const RuntimeValue *headers = (argc >= 2) ? args[1] : NULL;
    return http_delete(args[0], headers);
}

/**
 * @brief HTTP timeout configuration wrapper for stdlib integration
 * @param args Arguments array: [timeout_seconds]
 * @param argc Number of arguments
 * @return Null value
 */
RuntimeValue *http_timeout_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc >= 1 && args[0]->type == RV_NUMBER) {
        http_configure_timeout((long)args[0]->data.number);
    }
    return rv_new_null();
}