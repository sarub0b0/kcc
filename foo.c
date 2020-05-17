#include <stdio.h>

int x;
int y[10];

int foo() {
    printf("OK\n");
    return 5;
}

int bar() {
    x    = 1;
    y[0] = 2;
    return x + y[0];
}
