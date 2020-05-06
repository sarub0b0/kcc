#include <stdio.h>

int foo() {
  printf("OK\n");
  return 5;
}

void bar() { return; }

int foo2() {
  int a = 0;
  a = foo();
  return 0;
}
