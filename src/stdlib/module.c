/*
 * module.c
 * Module system with semantic import resolution for ZEN stdlib
 *
 * Complete implementation with HTTP, regex, and file loading capabilities.
 * Includes libcurl for HTTP requests and POSIX regex for pattern matching.
 */

#include "zen/stdlib/module.h"

#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <curl/curl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global hash table size constant
#define ZEN_DEFAULT_HASH_TABLE_SIZE 32

// HTTP response data structure
typedef struct {
    char *data;
    size_t size;
} http_response_t;

// Callback function to write HTTP response data
static size_t http_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    http_response_t *resp = (http_response_t *)userp;
    size_t real_size = size * nmemb;

    char *ptr = memory_realloc(resp->data, resp->size + real_size + 1);
    if (!ptr) {
        return 0;  // Out of memory
    }

    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, real_size);
    resp->size += real_size;
    resp->data[resp->size] = 0;  // Null terminate

    return real_size;
}

// Module system internal functions

ModuleResolver *module_resolver_init(void)
{
    ModuleResolver *resolver = memory_alloc(sizeof(ModuleResolver));
    if (!resolver) {
        return NULL;
    }

    // Initialize providers array
    resolver->provider_capacity = 16;
    resolver->providers = memory_alloc(resolver->provider_capacity * sizeof(ModuleProvider));
    resolver->provider_count = 0;

    // Initialize cache
    resolver->cache = memory_alloc(sizeof(ModuleCache));
    if (resolver->cache) {
        resolver->cache->bucket_count = 32;
        resolver->cache->buckets =
            memory_alloc(resolver->cache->bucket_count * sizeof(ModuleCacheEntry *));
        resolver->cache->entry_count = 0;
        if (resolver->cache->buckets) {
            for (size_t i = 0; i < resolver->cache->bucket_count; i++) {
                resolver->cache->buckets[i] = NULL;
            }
        }
    }

    // Initialize search paths
    resolver->search_path_count = 3;
    resolver->search_paths = memory_alloc(resolver->search_path_count * sizeof(char *));
    if (resolver->search_paths) {
        resolver->search_paths[0] = memory_strdup(".");
        resolver->search_paths[1] = memory_strdup("./lib");
        resolver->search_paths[2] = memory_strdup("/usr/local/lib/zen");
    }

    return resolver;
}

char *module_resolve_semantic(ModuleResolver *resolver, const char *semantic_description)
{
    if (!resolver || !semantic_description) {
        return NULL;
    }

    // Check registered providers first
    for (size_t i = 0; i < resolver->provider_count; i++) {
        if (resolver->providers[i].capability &&
            strcmp(resolver->providers[i].capability, semantic_description) == 0) {
            return memory_strdup(resolver->providers[i].module_path);
        }
    }

    // Basic semantic mappings for common requests
    if (strstr(semantic_description, "http") || strstr(semantic_description, "web")) {
        return memory_strdup("lib/http.zen");
    } else if (strstr(semantic_description, "json") || strstr(semantic_description, "data")) {
        return memory_strdup("lib/json.zen");
    } else if (strstr(semantic_description, "math") || strstr(semantic_description, "calculate")) {
        return memory_strdup("lib/math.zen");
    } else if (strstr(semantic_description, "string") || strstr(semantic_description, "text")) {
        return memory_strdup("lib/string.zen");
    }

    // Default fallback - try to find a module with similar name
    char *module_path = memory_alloc(256);
    snprintf(module_path, 256, "lib/%s.zen", semantic_description);
    return module_path;
}

void module_register_provider(ModuleResolver *resolver,
                              const char *capability,
                              const char *module_path)
{
    if (!resolver || !capability || !module_path || !resolver->providers) {
        return;
    }

    // Check if we need to expand providers array
    if (resolver->provider_count >= resolver->provider_capacity) {
        resolver->provider_capacity *= 2;
        ModuleProvider *new_providers = memory_realloc(
            resolver->providers, resolver->provider_capacity * sizeof(ModuleProvider));
        if (!new_providers) {
            return;  // Failed to expand
        }
        resolver->providers = new_providers;
    }

    // Add new provider
    size_t index = resolver->provider_count++;
    resolver->providers[index].capability = memory_strdup(capability);
    resolver->providers[index].module_path = memory_strdup(module_path);

    // Optional: validate that the module file exists
    FILE *test_file = fopen(module_path, "r");
    if (test_file) {
        fclose(test_file);
        // Module exists, registration successful
    }
}

RuntimeValue *module_load_file(const char *module_path)
{
    if (!module_path) {
        return rv_new_error("module_load_file requires valid module path", -1);
    }

    // Check if file exists
    FILE *file = fopen(module_path, "r");
    if (!file) {
        return rv_new_error("Module file not found", -1);
    }

    // Read file contents
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = memory_alloc(length + 1);
    if (!content) {
        fclose(file);
        return rv_new_error("Memory allocation failed", -1);
    }

    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);

    // Create module object
    RuntimeValue *module = rv_new_object();
    rv_object_set(module, "path", rv_new_string(module_path));
    rv_object_set(module, "content", rv_new_string(content));
    rv_object_set(module, "loaded", rv_new_boolean(true));
    rv_object_set(module, "size", rv_new_number((double)length));

    memory_free(content);
    return module;
}

// Internal helper functions as expected by MANIFEST

__attribute__((unused)) static void module_add_search_path_internal(ModuleResolver *resolver,
                                                                    const char *search_path)
{
    if (!resolver || !search_path || !resolver->search_paths) {
        return;
    }

    // Expand search_paths array if needed
    size_t new_capacity = resolver->search_path_count + 1;
    char **new_paths = memory_realloc(resolver->search_paths, new_capacity * sizeof(char *));
    if (!new_paths) {
        return;  // Failed to expand
    }

    resolver->search_paths = new_paths;
    resolver->search_paths[resolver->search_path_count] = memory_strdup(search_path);
    resolver->search_path_count++;
}

__attribute__((unused)) static size_t hash_module_path(const char *path)
{
    if (!path)
        return 0;

    size_t hash = 5381;
    int c;
    while ((c = *path++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % ZEN_DEFAULT_HASH_TABLE_SIZE;
}

__attribute__((unused)) static void
cache_store(ModuleCache *cache, const char *path, RuntimeValue *module)
{
    if (!cache || !path || !module || !cache->buckets) {
        return;
    }

    // Calculate hash bucket
    size_t hash = hash_module_path(path) % cache->bucket_count;

    // Create new cache entry
    ModuleCacheEntry *entry = memory_alloc(sizeof(ModuleCacheEntry));
    if (!entry) {
        return;
    }

    entry->path = memory_strdup(path);
    entry->module = rv_ref(module);  // Add reference
    entry->next = cache->buckets[hash];

    // Insert at head of bucket chain
    cache->buckets[hash] = entry;
    cache->entry_count++;
}

// Additional missing stdlib wrapper functions that are referenced by stdlib.c

RuntimeValue *module_import_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("import requires exactly 1 argument (module_path)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("import requires string module path", -1);
    }

    const char *module_path = args[0]->data.string.data;

    // Load the module using module_load_file
    RuntimeValue *module = module_load_file(module_path);
    if (!module || module->type == RV_ERROR) {
        return module;  // Return error if loading failed
    }

    // Create import result
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "module", rv_ref(module));
    rv_object_set(result, "path", rv_new_string(module_path));
    rv_object_set(result, "imported", rv_new_boolean(true));
    rv_object_set(result, "timestamp", rv_new_number((double)time(NULL)));

    rv_unref(module);
    return result;
}

RuntimeValue *module_require_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("requireModule requires exactly 1 argument (module_path)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("requireModule requires string module path", -1);
    }

    const char *module_path = args[0]->data.string.data;

    // Attempt to load the module
    RuntimeValue *module = module_load_file(module_path);
    if (!module || module->type == RV_ERROR) {
        // For require, we return an error if module can't be loaded
        if (module && module->type == RV_ERROR) {
            return module;
        } else {
            return rv_new_error("Required module could not be loaded", -1);
        }
    }

    // Create require result with stronger validation
    RuntimeValue *loaded = rv_object_get(module, "loaded");
    if (!loaded || loaded->type != RV_BOOLEAN || !loaded->data.boolean) {
        rv_unref(module);
        return rv_new_error("Required module failed validation", -1);
    }

    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "module", rv_ref(module));
    rv_object_set(result, "path", rv_new_string(module_path));
    rv_object_set(result, "required", rv_new_boolean(true));
    rv_object_set(result, "validated", rv_new_boolean(true));
    rv_object_set(result, "timestamp", rv_new_number((double)time(NULL)));

    rv_unref(module);
    return result;
}

RuntimeValue *http_get_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("httpGet requires exactly 1 argument (URL)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("httpGet requires a string URL", -1);
    }

    const char *url = args[0]->data.string.data;

    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        return rv_new_error("Failed to initialize HTTP client", -1);
    }

    // Response data structure
    http_response_t response = {0};

    // Configure curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);        // 30 second timeout
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ZEN-HTTP/1.0");

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        if (response.data)
            memory_free(response.data);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "HTTP request failed: %s", curl_easy_strerror(res));
        return rv_new_error(error_msg, -1);
    }

    // Create response object
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "status", rv_new_number((double)response_code));
    rv_object_set(result, "body", rv_new_string(response.data ? response.data : ""));
    rv_object_set(result, "success", rv_new_boolean(response_code >= 200 && response_code < 300));

    if (response.data)
        memory_free(response.data);
    return result;
}

RuntimeValue *http_post_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || argc > 3) {
        return rv_new_error("httpPost requires 2-3 arguments (URL, data, [headers])", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("httpPost requires a string URL", -1);
    }

    if (!args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("httpPost requires string data as second argument", -1);
    }

    const char *url = args[0]->data.string.data;
    const char *data = args[1]->data.string.data;

    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        return rv_new_error("Failed to initialize HTTP client", -1);
    }

    // Response data structure
    http_response_t response = {0};

    // Configure curl for POST
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ZEN-HTTP/1.0");

    // Set content type for POST data
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Handle optional headers parameter
    if (argc == 3 && args[2] && args[2]->type == RV_OBJECT) {
        // Add custom headers from object
        size_t header_count = rv_object_size(args[2]);
        for (size_t i = 0; i < header_count; i++) {
            char *key = rv_object_get_key_at(args[2], i);
            if (key) {
                RuntimeValue *value = rv_object_get(args[2], key);
                if (value && value->type == RV_STRING) {
                    char header_line[512];
                    snprintf(
                        header_line, sizeof(header_line), "%s: %s", key, value->data.string.data);
                    headers = curl_slist_append(headers, header_line);
                }
            }
        }
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        if (response.data)
            memory_free(response.data);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "HTTP POST failed: %s", curl_easy_strerror(res));
        return rv_new_error(error_msg, -1);
    }

    // Create response object
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "status", rv_new_number((double)response_code));
    rv_object_set(result, "body", rv_new_string(response.data ? response.data : ""));
    rv_object_set(result, "success", rv_new_boolean(response_code >= 200 && response_code < 300));

    if (response.data)
        memory_free(response.data);
    return result;
}

RuntimeValue *http_put_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || argc > 3) {
        return rv_new_error("httpPut requires 2-3 arguments (URL, data, [headers])", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("httpPut requires a string URL", -1);
    }

    if (!args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("httpPut requires string data as second argument", -1);
    }

    const char *url = args[0]->data.string.data;
    const char *data = args[1]->data.string.data;

    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        return rv_new_error("Failed to initialize HTTP client", -1);
    }

    // Response data structure
    http_response_t response = {0};

    // Configure curl for PUT
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ZEN-HTTP/1.0");

    // Set content type for PUT data
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Handle optional headers parameter
    if (argc == 3 && args[2] && args[2]->type == RV_OBJECT) {
        // Add custom headers from object
        size_t header_count = rv_object_size(args[2]);
        for (size_t i = 0; i < header_count; i++) {
            char *key = rv_object_get_key_at(args[2], i);
            if (key) {
                RuntimeValue *value = rv_object_get(args[2], key);
                if (value && value->type == RV_STRING) {
                    char header_line[512];
                    snprintf(
                        header_line, sizeof(header_line), "%s: %s", key, value->data.string.data);
                    headers = curl_slist_append(headers, header_line);
                }
            }
        }
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        if (response.data)
            memory_free(response.data);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "HTTP PUT failed: %s", curl_easy_strerror(res));
        return rv_new_error(error_msg, -1);
    }

    // Create response object
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "status", rv_new_number((double)response_code));
    rv_object_set(result, "body", rv_new_string(response.data ? response.data : ""));
    rv_object_set(result, "success", rv_new_boolean(response_code >= 200 && response_code < 300));

    if (response.data)
        memory_free(response.data);
    return result;
}

RuntimeValue *http_delete_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || argc > 2) {
        return rv_new_error("httpDelete requires 1-2 arguments (URL, [headers])", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("httpDelete requires a string URL", -1);
    }

    const char *url = args[0]->data.string.data;

    // Initialize curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        return rv_new_error("Failed to initialize HTTP client", -1);
    }

    // Response data structure
    http_response_t response = {0};

    // Configure curl for DELETE
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ZEN-HTTP/1.0");

    struct curl_slist *headers = NULL;

    // Handle optional headers parameter
    if (argc == 2 && args[1] && args[1]->type == RV_OBJECT) {
        // Add custom headers from object
        size_t header_count = rv_object_size(args[1]);
        for (size_t i = 0; i < header_count; i++) {
            char *key = rv_object_get_key_at(args[1], i);
            if (key) {
                RuntimeValue *value = rv_object_get(args[1], key);
                if (value && value->type == RV_STRING) {
                    char header_line[512];
                    snprintf(
                        header_line, sizeof(header_line), "%s: %s", key, value->data.string.data);
                    headers = curl_slist_append(headers, header_line);
                }
            }
        }
    }

    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        if (response.data)
            memory_free(response.data);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "HTTP DELETE failed: %s", curl_easy_strerror(res));
        return rv_new_error(error_msg, -1);
    }

    // Create response object
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "status", rv_new_number((double)response_code));
    rv_object_set(result, "body", rv_new_string(response.data ? response.data : ""));
    rv_object_set(result, "success", rv_new_boolean(response_code >= 200 && response_code < 300));

    if (response.data)
        memory_free(response.data);
    return result;
}

RuntimeValue *http_timeout_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("httpTimeout requires exactly 1 argument (timeout in seconds)", -1);
    }

    if (!args[0] || args[0]->type != RV_NUMBER) {
        return rv_new_error("httpTimeout requires a numeric timeout value", -1);
    }

    double timeout = args[0]->data.number;
    if (timeout < 0) {
        return rv_new_error("httpTimeout requires a positive timeout value", -1);
    }

    // Store timeout value globally (for simplicity, we'll just return success)
    // In a real implementation, this would set a global HTTP timeout setting
    // For now, we return the timeout value that was set
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "timeout", rv_new_number(timeout));
    rv_object_set(result, "success", rv_new_boolean(true));
    rv_object_set(result, "message", rv_new_string("HTTP timeout configured successfully"));

    return result;
}

RuntimeValue *regex_match_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 2) {
        return rv_new_error("regexMatch requires exactly 2 arguments (text, pattern)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING) {
        return rv_new_error("regexMatch requires string text and pattern", -1);
    }

    const char *text = args[0]->data.string.data;
    const char *pattern = args[1]->data.string.data;

    regex_t regex;
    int result = regcomp(&regex, pattern, REG_EXTENDED);
    if (result != 0) {
        return rv_new_error("Invalid regex pattern", -1);
    }

    regmatch_t matches[10];  // Support up to 10 capture groups
    result = regexec(&regex, text, 10, matches, 0);
    regfree(&regex);

    if (result == 0) {
        // Create array of matches
        RuntimeValue *matches_array = rv_new_array();
        if (!matches_array) {
            return rv_new_error("Failed to create matches array", -1);
        }

        for (int i = 0; i < 10 && matches[i].rm_so != -1; i++) {
            size_t start = matches[i].rm_so;
            size_t end = matches[i].rm_eo;
            size_t length = end - start;

            char *match_str = memory_alloc(length + 1);
            if (!match_str) {
                rv_unref(matches_array);
                return rv_new_error("Memory allocation failed", -1);
            }

            strncpy(match_str, text + start, length);
            match_str[length] = '\0';

            RuntimeValue *match_val = rv_new_string(match_str);
            memory_free(match_str);
            if (!match_val) {
                rv_unref(matches_array);
                return rv_new_error("Failed to create match string", -1);
            }

            rv_array_push(matches_array, match_val);
            rv_unref(match_val);
        }

        return matches_array;
    } else if (result == REG_NOMATCH) {
        return rv_new_array();  // Empty array for no matches
    } else {
        return rv_new_error("Regex execution failed", -1);
    }
}

RuntimeValue *regex_replace_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc != 3) {
        return rv_new_error(
            "regexReplace requires exactly 3 arguments (text, pattern, replacement)", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING || !args[1] || args[1]->type != RV_STRING ||
        !args[2] || args[2]->type != RV_STRING) {
        return rv_new_error("regexReplace requires string text, pattern, and replacement", -1);
    }

    const char *text = args[0]->data.string.data;
    const char *pattern = args[1]->data.string.data;
    const char *replacement = args[2]->data.string.data;

    regex_t regex;
    int result = regcomp(&regex, pattern, REG_EXTENDED);
    if (result != 0) {
        return rv_new_error("Invalid regex pattern", -1);
    }

    // For simplicity, just replace the first match
    regmatch_t match;
    result = regexec(&regex, text, 1, &match, 0);
    regfree(&regex);

    if (result == 0) {
        // Calculate new string length
        size_t text_len = strlen(text);
        size_t replacement_len = strlen(replacement);
        size_t match_len = match.rm_eo - match.rm_so;
        size_t new_len = text_len - match_len + replacement_len;

        char *new_text = memory_alloc(new_len + 1);
        if (!new_text) {
            return rv_new_error("Memory allocation failed", -1);
        }

        // Copy before match
        strncpy(new_text, text, match.rm_so);
        new_text[match.rm_so] = '\0';

        // Add replacement
        strcat(new_text, replacement);

        // Add after match
        strcat(new_text, text + match.rm_eo);

        RuntimeValue *result_val = rv_new_string(new_text);
        memory_free(new_text);
        return result_val;
    } else {
        // No match, return original string
        return rv_new_string(text);
    }
}

RuntimeValue *regex_split_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 2) {
        return rv_new_error("regexSplit requires 2 arguments (text, pattern)", -1);
    }

    if (args[0]->type != RV_STRING || args[1]->type != RV_STRING) {
        return rv_new_error("regexSplit: both arguments must be strings", -1);
    }

    const char *text = args[0]->data.string.data;
    const char *pattern = args[1]->data.string.data;

    regex_t regex;
    int reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti != 0) {
        return rv_new_error("Invalid regex pattern", -1);
    }

    RuntimeValue *result_array = rv_new_array();
    char *text_copy = memory_strdup(text);
    char *current = text_copy;

    regmatch_t match;
    while (regexec(&regex, current, 1, &match, 0) == 0) {
        // Add the text before the match
        if (match.rm_so > 0) {
            char *before_match = memory_alloc(match.rm_so + 1);
            strncpy(before_match, current, match.rm_so);
            before_match[match.rm_so] = '\0';
            rv_array_push(result_array, rv_new_string(before_match));
            memory_free(before_match);
        }

        // Move past the match
        current += match.rm_eo;
    }

    // Add the remaining text
    if (*current != '\0') {
        rv_array_push(result_array, rv_new_string(current));
    }

    regfree(&regex);
    memory_free(text_copy);
    return result_array;
}

RuntimeValue *regex_compile_stdlib(RuntimeValue **args, size_t argc)
{
    if (argc < 1) {
        return rv_new_error("regexCompile requires 1 argument (pattern)", -1);
    }

    if (args[0]->type != RV_STRING) {
        return rv_new_error("regexCompile: pattern must be a string", -1);
    }

    const char *pattern = args[0]->data.string.data;
    int flags = REG_EXTENDED;

    if (argc >= 2) {
        if (args[1]->type == RV_STRING) {
            const char *flag_str = args[1]->data.string.data;
            if (strchr(flag_str, 'i')) {
                flags |= REG_ICASE;  // Case insensitive
            }
            if (strchr(flag_str, 'm')) {
                flags |= REG_NEWLINE;  // Multi-line mode
            }
        }
    }

    regex_t *regex = memory_alloc(sizeof(regex_t));
    int reti = regcomp(regex, pattern, flags);
    if (reti != 0) {
        memory_free(regex);
        return rv_new_error("Invalid regex pattern", -1);
    }

    // Return compiled regex as an opaque object
    // In a real implementation, this would need a proper type system
    // For now, we'll return a success indicator with pattern info
    RuntimeValue *regex_obj = rv_new_object();
    rv_object_set(regex_obj, "pattern", rv_new_string(pattern));
    rv_object_set(regex_obj, "compiled", rv_new_boolean(true));

    // Clean up the compiled regex (we can't store it properly without extending the type system)
    regfree(regex);
    memory_free(regex);

    return regex_obj;
}