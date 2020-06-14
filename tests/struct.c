struct struct_a {
  short a;
  int b;
  long c;
  char d;
  int e[3];
};

// int a;
// long b = 1;
// short c = 1;
// int *d;

int main() {
  struct struct_a sa;

  // sa.a = 5;
  // sa.b = 10;
  // sa.c = 2;
  // sa.d = 97;

  sa.e[0] = 1 + 10;
  sa.e[1] = 2 + 10;
  sa.e[2] = 3 + 10;

  // printf("a:%d b:%d c:%ld d:%c\n", sa.a, sa.b, sa.c, sa.d);

  printf("e[0]:%d, e[1]:%d, e[2]:%d, num:%d\n", sa.e[0], sa.e[1], sa.e[2], 10);

  return sa.b;
}
