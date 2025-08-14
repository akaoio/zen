#include <stdio.h>
#include "src/include/zen/core/lexer.h"
#include "src/include/zen/core/parser.h"
#include "src/include/zen/core/visitor.h"
#include "src/include/zen/core/scope.h"
#include "src/include/zen/core/memory.h"
#include "src/include/zen/core/runtime_value.h"

int main() {
    memory_debug_enable(true);
    memory_reset_stats();
    
    // Create a scope and add variables with negative numbers
    scope_T* scope = scope_new();
    
    printf("Creating negative values...\n");
    RuntimeValue* val1 = rv_new_number(-42);
    RuntimeValue* val2 = rv_new_number(-3.14);
    
    printf("Setting variables in scope...\n");
    scope_set_variable(scope, "negative_int", val1);
    scope_set_variable(scope, "negative_float", val2);
    
    // The scope now owns references to these values
    rv_unref(val1);
    rv_unref(val2);
    
    printf("Freeing scope...\n");
    scope_free(scope);
    
    MemoryStats stats;
    memory_get_stats(&stats);
    printf("Current allocated: %zu bytes\n", stats.current_allocated);
    printf("Outstanding allocs: %zu\n", stats.allocation_count - stats.free_count);
    
    memory_print_leak_report();
    
    return 0;
}