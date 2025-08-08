#include <stdio.h>
struct test { char data[1000]; };
int main() { printf("Test struct size: %zu bytes\n", sizeof(struct test)); return 0; }
