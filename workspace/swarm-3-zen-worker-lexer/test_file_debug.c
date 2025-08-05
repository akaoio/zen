#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zen/stdlib/io.h"

int main() {
    printf("Testing file reading for 'test_variable.zen'...\n");
    
    char* contents = get_file_contents("test_variable.zen");
    if (!contents) {
        printf("ERROR: Failed to read file\n");
        return 1;
    }
    
    printf("File contents: '%s'\n", contents);
    printf("File length: %zu\n", strlen(contents));
    printf("Last character code: %d\n", (int)contents[strlen(contents)-1]);
    
    // Let's see if there are any issues with the content
    for (size_t i = 0; i < strlen(contents); i++) {
        printf("Char %zu: '%c' (code %d)\n", i, contents[i], (int)contents[i]);
    }
    
    free(contents);
    return 0;
}