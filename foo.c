#include <stdio.h>

int foo() {
  printf("OK\n");
  return 5;
}

void bar() { return; }

int foo2(int x, int y) {
  printf("OK: %d\n", x + y);
  return x + y;
}

int foo3(int x, int y, int z) {
  int a = x + y + z;
  printf("OK: %d\n", a);
  return a;
}
