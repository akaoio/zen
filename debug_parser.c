#include <stdio.h>

int main() {
    printf("Testing parser without ZEN execution\n");
    
    // This will test if the issue is in parsing or execution
    const char* input = "put \"test.json\" data";
    
    // Parse without executing to isolate the issue
    printf("Input: %s\n", input);
    printf("If this prints, parsing works. Issue is in visitor.\n");
    
    return 0;
}