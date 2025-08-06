#include "zen/types/value.h"
#include "zen/stdlib/string.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Test zen_string_upper directly
    Value* test_str = value_new_string("Hello World");
    printf("Original string: %s\n", test_str->as.string->data);
    
    Value* upper_result = zen_string_upper(test_str);
    printf("Upper result type: %d\n", upper_result->type);
    
    if (upper_result->type == VALUE_STRING && upper_result->as.string) {
        printf("Upper string: %s\n", upper_result->as.string->data);
    } else {
        printf("Upper function failed or returned wrong type\n");
    }
    
    value_unref(test_str);
    value_unref(upper_result);
    return 0;
}