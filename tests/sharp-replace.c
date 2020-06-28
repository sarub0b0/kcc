#include <stdio.h>

#define var(i) printf("var" #i " %d\n", var##i)

#if (defined hoge && hoge - 0)

#endif

int main() {
  int var0 = 10;
  var(0);
  return 0;
}
