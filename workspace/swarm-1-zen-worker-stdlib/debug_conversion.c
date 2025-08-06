#include "zen/types/value.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Test string value creation and conversion
    Value* str_val = value_new_string("HELLO WORLD");
    printf("Created string value: %s\n", str_val->as.string->data);
    
    // Test value_to_string (what should be called for output)
    char* str_repr = value_to_string(str_val);
    if (str_repr) {
        printf("value_to_string result: %s\n", str_repr);
        free(str_repr);
    } else {
        printf("value_to_string failed\n");
    }
    
    value_unref(str_val);
    return 0;
}