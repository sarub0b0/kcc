/*
 * Comment test
 *
 *
 *
 *
 */

#include <stdbool.h>

#include "include.h"
#include "extern.h"

typedef int Int32;

Int32 number = 0;
Int32 success = 0;
Int32 failed = 0;
Int32 g0 = 1;
Int32 g1, g2;
Int32 g3 = 0, g4 = 0;
Int32 g5[5];

char *g6 = "abc";
char g7[] = "abc";

int *g8 = &g1;
int *g9;

int g10[10] = {0, 1, 2, 3, 4, 5};
char *g11[] = {"abc", "def", "ghi"};
char *g12[5] = {"abc", "def", "ghi"};

typedef struct {
  short a;
  int b;
  long c;
  char d;
  int e[3];
  int *f;
  struct {
    int a;
    long b;
    short c;
  } g;
} Struct;

enum enum_a {
  A,
  B,
  C = 10,
  D,
};

enum enum_b {
  A1,
  B1,
  C1 = 20,
  D1,
};

Struct g13;

struct {
  char a;
  int b;
} g14 = {1, 2};

struct {
  char a;
  int b;
} g15[2] = {{1, 2}, {3, 4}};

union {
  char a;
  int b;
  long c;
  union {
    int a;
    long b;
  } d;

  char e[3];
} g16;

extern int extern_a;

#define TRUE 1
#define FALSE 0
#define MAX_LEN 128

#define STRING "hoge"
#define OK "\x1b[32mOK\x1b[0m"
#define FAILED "\x1b[31mFAILED\x1b[0m"

#define DEBUG

#define one(x) (x)
#define equal(x, y) (x == y)
#define notequal(x, y) (x != y)
#define add_sub(x, y) ((x + x) - (y + y))

#define addadd(x, y) (one(x) + one(y))

#define p(...) printf(__VA_ARGS__)
#define p1(x, ...) printf(x, __VA_ARGS__)
#define p2(x...) printf(x)

#if 0
#else

#define def
#ifdef def
#else
#endif

#endif

#undef def

int assert(unsigned long long expected,
           unsigned long long actual,
           char *code,
           bool is_unsigned) {
  p("% 4d: ", number++);
  if (expected == actual) {

    if (is_unsigned) {
      p1("%s => %llu ... " OK "\n", code, actual);
    } else {
      p1("%s => %lld ... " OK "\n", code, actual);
    }

    success++;
  } else {
    if (is_unsigned) {
      p1("%s => %llu expected, but got %llu ... %s\n",
         code,
         expected,
         actual,
         FAILED);
    } else {
      p1("%s => %lld expected, but got %lld ... %s\n",
         code,
         expected,
         actual,
         FAILED);
    }

    failed++;
  }
  return 0;
}

int add(int, int);
int padd(int *, int *);
int add(int a, int b) {
  return a + b;
}
int padd(int *a, int *b) {
  return *a + *b;
}
int sub(int x, int y) {
  return x - y;
}
int fib(int n) {
  if (n <= 1) return 1;
  return fib(n - 1) + fib(n - 2);
}
int foo() {
  return 1;
}

int for3() {
  int a;
  for (int i = 0; i < 3; ++i) {
    a = i;
  }
  return a;
}

void void0() {
  int a, b, c;
  return;
}

void void1() {
  int a = 0, b = 1, c;
  c = (a < b) ? 1 : 2;
  return;
}

int logand() {
  if (1 && 0) return 0;

  if (1 && 1) return 5;
}

int logor() {
  if (0 || 0) return 0;

  if (0 || 1) return 5;
}
int mixed(int a, short b, long c, char d) {
  int x = a + b + c + d;

  return x;
}

int variadic(int x, ...);
int main() {
  assert(0, 0, "0", false);
  assert(42, 42, "42", false);
  assert(21, 5 + 20 - 4, "5+20-4", false);
  assert(41, 12 + 34 - 5, "12+34-5", false);
  assert(47, 5 + 6 * 7, "5+6*7", false);
  assert(15, 5 * (9 - 6), "5*(9-6)", false);
  assert(4, (3 + 5) / 2, "(3+5)/2", false);
  assert(3, +3, "+3", false);
  assert(8, -(-3 - 5), "-(-3-5)", false);
  assert(15, -3 * (-5), "-3*(-5)", false);
  assert(1, 1 == 1, "1==1", false);
  assert(0, 1 != 1, "1!=1", false);
  assert(1, 2 >= 1, "2>=1", false);
  assert(1, 1 <= 2, "1<=2", false);
  assert(1, 2 > 1, "2>1", false);
  assert(1, 1 < 2, "1<2", false);
  assert(5,
         ({
           int a;
           a = 3;
           a + 2;
         }),
         "({ int a; a=3; a+2; })",
         false);
  assert(2,
         ({
           int a;
           int b;
           a = b = 2;
           a;
         }),
         "({ int a; int b; a=b=2; a; })",
         false);
  assert(15,
         ({
           int foo;
           foo = 3;
           foo + 12;
         }),
         "({ int foo; foo=3; foo+12; })",
         false);

  assert(3,
         ({
           int a = 3;
           a;
         }),
         "({ int a=3; a; })",
         false);

  assert(15, ({ add(5, 10); }), "({ add(5, 10); })", false);
  assert(15,
         ({
           int a = add(5, 10);
           a;
         }),
         "({ int a=add(5, 10); a; })",
         false);
  assert(3, ({ sub(5, 2); }), "({ sub(5, 2); })", false);
  assert(2, ({ sub(5, 3); }), "({ sub(5, 3); })", false);
  assert(
      1, ({ sub(5, 2) - sub(5, 3); }), "({ sub(5, 2)-sub(5, 3); })", false);
  assert(55, ({ fib(9); }), "({ fib(9); })", false);
  assert(3,
         ({
           int i;
           for (i = 0; i < 3; i = i + 1) {
             i = i;
           }
           i;
         }),
         "({ int i; for(i=0; i<=3; i=i+1){ i=i; } i; })",
         false);
  assert(3,
         ({
           int a;
           for (int i = 0; i <= 3; i = i + 1) {
             a = i;
           }
           a;
         }),
         "({ int a; for(int i=0; i<=3; i=i+1){ a=i; } a; })",
         false);
  assert(3,
         ({
           int a;
           int i = 0;
           while (i < 3) {
             i = i + 1;
             a = i;
           }
           a;
         }),
         "({ int a; int i=0; while(i<=3){ i=i+1; a=i; } a; })",
         false);

  assert(5,
         ({
           int i = 0;
           do {
             i++;
           } while (i < 5);
           i;
         }),
         "({ int i=0; do{ i++; }while(i<5); i; })",
         false);

  assert(3,
         ({
           int x;
           int *y = &x;
           *y = 3;
           x;
         }),
         "({ int x; int *y; y=&x; *y=3; x; })",
         false);
  assert(3,
         ({
           int x;
           int *y = &x;
           *y = 3;
           x;
         }),
         "({ int x; int *y=&x; *y=3; x; })",
         false);
  assert(8,
         ({
           int x;
           int y;
           x = 3;
           y = 5;
           add(x, y);
         }),
         "({ int x; int y; x=3; y=5; add(x, y); })",
         false);
  assert(3,
         ({
           int x = 3;
           *&x;
         }),
         "({ int x=3; *&x; })",
         false);
  assert(5,
         ({
           int x = 3;
           int y = 5;
           *(&x + 1);
         }),
         "({ int x=3; int y=5; *(&x+1); })",
         false);
  assert(3,
         ({
           int x = 3;
           int y = 5;
           *(&y - 1);
         }),
         "({ int x=3; int y=5; *(&y-1); })",
         false);
  assert(4,
         ({
           int x;
           sizeof(x);
         }),
         "({ int x; sizeof(x); })",
         false);
  assert(8,
         ({
           int *x;
           sizeof(x);
         }),
         "({ int *x; sizeof(x); })",
         false);
  assert(4,
         ({
           int x;
           sizeof(x + 3);
         }),
         "({ int x; sizeof(x+3); })",
         false);
  assert(8,
         ({
           int *x;
           sizeof(x + 5);
         }),
         "({ int *x; sizeof(x+5); })",
         false);
  assert(4,
         ({
           int *x;
           sizeof(*x);
         }),
         "({ int *x; sizeof(*x); })",
         false);
  assert(4, ({ sizeof(1); }), "({ sizeof(1); })", false);
  assert(5, ({ sizeof(1) + 1; }), "({ sizeof(1)+1; })", false);
  assert(4, ({ sizeof(sizeof(1)); }), "({ sizeof(sizeof(1)); })", false);
  assert(3,
         ({
           int x[2];
           int *y = &x;
           *y = 3;
           *x;
         }),
         "({ int x[2]; int *y=&x; *y=3; *x; })",
         false);
  assert(3,
         ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *x;
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; })",
         false);
  assert(4,
         ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *(x + 1);
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); })",
         false);
  assert(5,
         ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); })",
         false);
  assert(1,
         ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *p;
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *p; })",
         false);
  assert(2,
         ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *(p + 1);
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *(p+1); })",
         false);
  assert(3,
         ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *p + *(p + 1);
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *p+*(p+1); })",
         false);
  assert(8,
         ({
           int a[2];
           sizeof(a);
         }),
         "({ int a[2]; sizeof(a); })",
         false);
  assert(3,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           *x;
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; *x; })",
         false);
  assert(4,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           x[1];
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; x[1]; })",
         false);
  assert(4,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 1);
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; *(x+1); })",
         false);
  assert(5,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           x[2];
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; x[2]; })",
         false);
  assert(0,
         ({
           int x[2][3];
           int *y = x;
           *y = 0;
           **x;
         }),
         "({ int x[2][3]; int *y=x; *y=0; **x; })",
         false);
  assert(1,
         ({
           int x[2][3];
           int *y = x;
           *(y + 1) = 1;
           *(*x + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); })",
         false);
  assert(2,
         ({
           int x[2][3];
           int *y = x;
           *(y + 2) = 2;
           *(*x + 2);
         }),
         "({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); })",
         false);
  assert(3,
         ({
           int x[2][3];
           int *y = x;
           *(y + 3) = 3;
           **(x + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); })",
         false);
  assert(4,
         ({
           int x[2][3];
           int *y = x;
           *(y + 4) = 4;
           *(*(x + 1) + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); })",
         false);
  assert(5,
         ({
           int x[2][3];
           int *y = x;
           *(y + 5) = 5;
           *(*(x + 1) + 2);
         }),
         "({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); })",
         false);
  assert(3,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *x;
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; })",
         false);
  assert(4,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 1);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); })",
         false);
  assert(5,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })",
         false);
  assert(5,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })",
         false);
  assert(5,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })",
         false);
  assert(0,
         ({
           int x[2][3];
           int *y = x;
           y[0] = 0;
           x[0][0];
         }),
         "({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; })",
         false);
  assert(1,
         ({
           int x[2][3];
           int *y = x;
           y[1] = 1;
           x[0][1];
         }),
         "({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; })",
         false);
  assert(2,
         ({
           int x[2][3];
           int *y = x;
           y[2] = 2;
           x[0][2];
         }),
         "({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; })",
         false);
  assert(3,
         ({
           int x[2][3];
           int *y = x;
           y[3] = 3;
           x[1][0];
         }),
         "({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; })",
         false);
  assert(4,
         ({
           int x[2][3];
           int *y = x;
           y[4] = 4;
           x[1][1];
         }),
         "({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; })",
         false);
  assert(5,
         ({
           int x[2][3];
           int *y = x;
           y[5] = 5;
           x[1][2];
         }),
         "({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; })",
         false);
  assert(1, ({ g0; }), "({ g0; })", false);
  assert(3,
         ({
           g1 = 3;
           g1;
         }),
         "({ g1=3; g1; })",
         false);
  assert(0,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[0];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[0]; })",
         false);
  assert(1,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[1];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[1]; })",
         false);
  assert(2,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[2];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[2]; })",
         false);
  assert(3,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[3];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[3]; })",
         false);
  assert(16,
         ({
           int x[4];
           sizeof(x);
         }),
         "({ int x[4]; sizeof(x); })",
         false);
  assert(1,
         ({
           char a;
           sizeof(a);
         }),
         "({ char a; sizeof(a); })",
         false);
  assert(10,
         ({
           char a[10];
           sizeof(a);
         }),
         "({ char a[10]; sizeof(a); })",
         false);
  assert(3,
         ({
           char a = 1;
           char b = 2;
           a + b;
         }),
         "({ char a=1; char b=2; a+b; })",
         false);
  assert(3,
         ({
           char x[3];
           x[0] = -1;
           x[1] = 2;
           int y = 4;
           x[0] + y;
         }),
         "({ char x[3]; x[0]=-1; x[1]=2; int y; y=4; x[0]+y; })",
         false);
  assert(97, ({ "abc"[0]; }), "({ \"abc\"[0]; })", false);
  assert(98, ({ "abc"[1]; }), "({ \"abc\"[1]; })", false);
  assert(97,
         ({
           char *a = "abc";
           a[0];
         }),
         "({ char *a=\"abc\"; a[0]; })",
         false);
  assert(98,
         ({
           char *a = "abc";
           a[1];
         }),
         "({ char *a=\"abc\"; a[1]; })",
         false);

  assert(101,
         ({
           char *a = "abc", *b = "def";
           b[1];
         }),
         "({ char *a=\"abc\",*b=\"def\"; b[1]; })",
         false);
  assert(3,
         ({
           int x, y;
           x = 3;
           x;
         }),
         "({ int x,y; x=3; x; })",
         false);

  assert(3,
         ({
           int x = 1, y = 2;
           x + y;
         }),
         "({ int x=1, y=2; x+y; })",
         false);

  assert(1,
         ({
           int a[] = {1};
           a[0];
         }),
         "({ int a[]={1}; a[0]; })",
         false);
  assert(2,
         ({
           int a[] = {1, 2};
           a[1];
         }),
         "({ int a[]={1,2}; a[1]; })",
         false);
  assert(3,
         ({
           int a[3] = {1, 2, 3};
           a[2];
         }),
         "({ int a[3]={1,2,3}; a[2]; })",
         false);
  assert(1,
         ({
           int a[] = {1, 2, foo()};
           a[2];
         }),
         "({ int a[]={1,2,foo()}; a[2]; })",
         false);

  assert(98,
         ({
           char a[] = "abc";
           a[1];
         }),
         "({ char a[]=\"abc\"; a[1]; })",
         false);

  assert(98,
         ({
           char a[4] = "abc";
           a[1];
         }),
         "({ char a[4]=\"abc\"; a[1]; })",
         false);

  assert(97, ({ g6[0]; }), "({ g6[0]; })", false);
  assert(97, ({ g7[0]; }), "({ g7[0]; })", false);
  assert(3,
         ({
           g1 = 3;
           *g8;
         }),
         "({ g1=3; *g8; })",
         false);

  assert(3, ({ g10[3]; }), "({ g10[3]; })", false);
  assert(97, ({ g11[0][0]; }), "({ g11[0][0]; })", false);

  assert(1,
         ({
           int a = 1;
           void0();
           a;
         }),
         "({ int a=1; void0(); a; })",
         false);

  assert(2,
         ({
           int a = 1;
           short b = 2;
           long c = 2;
           c;
         }),
         "({ int a=1; short b=2; long c=2; c; })",
         false);

  assert(2,
         ({
           int a = 0;
           ++a;
           ++a;
         }),
         "({ int a=0; ++a; ++a; })",
         false);

  assert(1,
         ({
           int a = 3;
           --a;
           --a;
         }),
         "({ int a=3; --a; --a; })",
         false);

  assert(2,
         ({
           int a = 0;
           a++;
           a++;
         }),
         "({ int a=0; a++; a++; })",
         false);

  assert(1,
         ({
           int a = 3;
           a--;
           a--;
         }),
         "({ int a=3; a--; a--; })",
         false);

  assert(4,
         ({
           int a = 0;
           a += 2;
           a += 2;
         }),
         "({ int a=0; a+=2; a+=2; })",
         false);

  assert(1,
         ({
           int a = 5;
           a -= 2;
           a -= 2;
         }),
         "({ int a=5; a-=2; a-=2; })",
         false);
  assert(98,
         ({
           char *a = "abc";
           *a++;
         }),
         "({ char *a=\"abc\"; *a++; })",
         false);
  assert(99,
         ({
           char *a = "abc";
           *a += 2;
         }),
         "({ char *a=\"abc\"; *a+=2; })",
         false);

  assert(6,
         ({
           int a = 2;
           a *= 3;
         }),
         "({ int a=2; a*=3; })",
         false);

  assert(2,
         ({
           int a = 6;
           a /= 3;
         }),
         "({ int a=6; a/=3; })",
         false);

  assert(1,
         ({
           int a = 1;
           a = a == 1 ? a : 2;
           a;
         }),
         "({ int a=1; a = a==1 ? a:2; })",
         false);

  assert(2,
         ({
           int a = 2;
           a = a == 1 ? a : 2;
           a;
         }),
         "({ int a=2; a = a==1 ? a:2; })",
         false);

  assert(1,
         ({
           int a = 0;
           int b = 1;
           a | b;
         }),
         "({ int a=0; int b=1; a|b; })",
         false);
  assert(0,
         ({
           int a = 0;
           int b = 0;
           a | b | b;
         }),
         "({ int a=0; int b=0; a|b|b; })",
         false);
  assert(0,
         ({
           int a = 0;
           int b = 1;
           int c = a & b;
           c;
         }),
         "({ int a=0; int b=1; int c=a&b; c; })",
         false);
  assert(1,
         ({
           int a = 1;
           int b = 1;
           int c = a & b;
           c;
         }),
         "({ int a=1; int b=1; int c=a&b; c; })",
         false);

  assert(0,
         ({
           int a = 1;
           int b = 1;
           int c = a ^ b;
           c;
         }),
         "({ int a=1; int b=1; int c=a^b; c; })",
         false);

  assert(1,
         ({
           int a = 0;
           int b = 1;
           int c = a ^ b;
           c;
         }),
         "({ int a=0; int b=1; int c=a^b; c; })",
         false);

  assert(1,
         ({
           char a = 1;
           int b = (int) a;
           b;
         }),
         "({ char a=1; int b=(int)a; b;})",
         false);

  assert(3,
         ({
           int a = 3;
           int *b = (int *) &a;
           *b;
         }),
         "({ char a=3; int *b=(int *)&a; *b;})",
         false);

  assert(-1, ({ ~0; }), "({ ~0; })", false);

  assert(5, ({ logor(); }), "({ logor(); })", false);
  assert(5, ({ logand(); }), "({ logand(); })", false);

  assert(1,
         ({
           bool a;
           a = 1;
           a;
         }),
         "({ bool a=0; a=1; a; })",
         false);

  assert(3,
         ({
           Struct a;
           Struct b;
           a.a = 3;
           a.b = 2;
           a.a;
         }),
         "({ Struct a; a.a=3; a.b=2; a.a; })",
         false);

  assert(5,
         ({
           Struct a = {3, 2};
           a.b + a.a;
         }),
         "({ Struct a={3,2}; a.b+a.a; })",
         false);

  assert(3,
         ({
           Struct a;
           a.e[0] = 1;
           a.e[1] = 2;
           a.e[0] + a.e[1];
         }),
         "({ Struct a; a.e[0]=1; a.e[1]=2; a.e[0]+a.e[1]; })",
         false);

  assert(10,
         ({
           Struct a = {10};
           Struct *p = &a;
           p->a;
         }),
         "({ Struct a={10}; Struct *p=&a; p->a; })",
         false);

  assert(10,
         ({
           Struct a;
           Struct *p = &a;
           p->b = 10;
           p->b;
         }),
         "({ Struct a; Struct *p=&a; p->b=10; p->b; })",
         false);

  assert(3,
         ({
           Struct a = {1, 2, 3, 4}, *p;
           p = &a;
           p->e[0] = 1;
           p->e[1] = 2;
           p->e[0] + p->e[1];
         }),
         "({ Struct a,*p; p=&a; p->e[0]=1; p->e[1]=2; "
         "p->e[0]+p->e[1]; })",
         false);

  assert(5,
         ({
           g13.a = 5;
           g13.a;
         }),
         "({ g13.a=5; g13.a; })",
         false);

  assert(10, ({ mixed(1, 2, 3, 4); }), "({ mixed(1,2,3,4); })", false);

  assert(0, ({ A; }), "({ A; })", false);
  assert(1, ({ B; }), "({ B; })", false);
  assert(10, ({ C; }), "({ C; })", false);
  assert(11, ({ D; }), "({ D; })", false);

  assert(0,
         ({
           int a = A1;
           a;
         }),
         "({ int a=A1; a; })",
         false);
  assert(1,
         ({
           int a = B1;
           a;
         }),
         "({ int a=B1; a; })",
         false);
  assert(20,
         ({
           int a = C1;
           a;
         }),
         "({ int a=C1; a; })",
         false);
  assert(21,
         ({
           int a = D1;
           a;
         }),
         "({ int a=D1; a; })",
         false);

  assert(5,
         ({
           int a = 5;
           { int b = 10; }
           int b = a;
           a;
         }),
         "({ int a=5; { int b = 10; } int b=a; a; })",
         false);

  assert(5,
         ({
           int a = 5;
           int b = a;
           { int b = 10; }
           b;
         }),
         "({ int a=5; int b=a; { int b = 10; } b; })",
         false);

  assert(2,
         ({
           enum a { X, Y, Z };
           Z;
         }),
         "({ enum a{X,Y,Z}; Z; })",
         false);

  assert(5,
         ({
           struct a {
             int a;
             int b;
           };
           struct a a;
           a.a = 5;
           a.a;
         }),
         "({ struct a { int a; int b; }; struct a a; a.a=5; a.a; })",
         false);

  assert(1, ({ TRUE; }), "({ TRUE; })", false);
  assert(128, ({ MAX_LEN; }), "({ MAX_LEN; })", false);

  assert(5, ({ one(5); }), "({ one(5); })", false);
  assert(10, ({ one(5) + one(5); }), "({ one(5)+one(5); })", false);
  assert(1, ({ equal(1, 1); }), "({ equal(1, 1); })", false);
  assert(4, ({ add_sub(3, 1); }), "({ add_sub(3,1); })", false);
  assert(5, ({ addadd(2, 3); }), "({ addadd(2,3); })", false);
  assert(10,
         ({ addadd((1 + 2), (3 + 4)); }),
         "({ addadd((1+2),(3+4)); })",
         false);
  assert(10,
         ({ add((one(1) + one(2)), (3 + 4)); }),
         "({ add((one(1)+one(2)),(3+4)); })",
         false);
  assert(15,
         ({
           int x = 5;
           int y = 10;
           padd(&x, &y);
         }),
         "({ int x=5; int y=10; padd(&x, &y); })",
         false);

  assert(97,
         ({
           char *c = "abc\n";
           c[0];
         }),
         "({ char *c=\"abc\\n\"; c[0]; })",
         false);

  assert(
      48 + 50 + 52,
      ({
        char *c[] = {"01", "23", "45"};
        (int) c[0][0] + c[1][0] + c[2][0];
      }),
      "({ char *c[]={\"01\",\"23\",\"45\"}; (int)c[0][0]+c[1][0]+c[2][0]; })",
      false);

  assert(3,
         ({
           int a[] = {0, 1, 2};
           a[0] + a[1] + a[2];
         }),
         "({ int a[]={0,1,2}; a[0]+a[1]+a[2]; })",
         false);
  assert(9,
         ({
           int d[][2] = {{1, 2}, {3, 4}, {5, 6}};
           d[0][0] + d[1][0] + d[2][0];
         }),
         "({ int d[][2]={{1,2},{3,4},{5,6}}; d[0][0]+d[1][0]+d[2][0]; })",
         false);

  assert(7,
         ({
           Struct a = {1, 2, 3, 4, {5, 6, 7}, 0, {9, 10, 11}};
           a.e[2];
         }),
         "({ Struct a={1,2,3,4,{5,6,7},0,{9,10,11}}; a.e[2];})",
         false);

  assert(11,
         ({
           Struct a = {1, 2, 3, 4, {5, 6, 7}, 0, {9, 10, 11}};
           a.g.c;
         }),
         "({ Struct a={1,2,3,4,{5,6,7},0,{9,10,11}}; a.g.c;})",
         false);

  assert(4,
         ({
           Struct a;
           Struct b = {1, 2, 3, 4};
           a = b;
           a.d;
         }),
         "({ Struct a; Struct b={1,2,3,4}; a=b; a.d; })",
         false);

  assert(3, ({ g14.a + g14.b; }), "({ g14.a+g14.b; })", false);
  assert(5, ({ g15[0].a + g15[1].b; }), "({ g15[0].a+g15[1].b; })", false);

  assert(11, ({ extern_a; }), "({ extern_a; })", false);

  assert(8,
         ({
           int a = 1;
           a << 3;
         }),
         "({ int a=1; a<<3; })",
         false);
  assert(1,
         ({
           int a = 8;
           a >> 3;
         }),
         "({ int a=8; a>>3; })",
         false);

  assert(-1,
         ({
           char a = 255;
           a;
         }),
         "({ char a=255; a; })",
         false);
  assert(-1,
         ({
           signed char a = 255;
           a;
         }),
         "({ signed char a=255; a; })",
         false);
  assert(255,
         ({
           unsigned char a = 255;
           a;
         }),
         "({ unsigned char a=255; a; })",
         false);

  assert(-1,
         ({
           short a = 65535;
           a;
         }),
         "({ short a=65535; a; })",
         false);
  assert(-1,
         ({
           signed short a = 65535;
           a;
         }),
         "({ signed short a=65535; a; })",
         false);
  assert(65535,
         ({
           unsigned short a = 65535;
           a;
         }),
         "({ unsigned short a=65535; a; })",
         false);

  assert(1, ({ sizeof(char); }), "({ sizeof(char); })", false);
  assert(1, ({ sizeof(signed char); }), "({ sizeof(signed char); })", false);
  assert(
      1, ({ sizeof(unsigned char); }), "({ sizeof(unsigned char); })", false);

  assert(2, ({ sizeof(short); }), "({ sizeof(short); })", false);
  assert(2, ({ sizeof(short int); }), "({ sizeof(short int); })", false);
  assert(
      2, ({ sizeof(signed short); }), "({ sizeof(signed short); })", false);
  assert(2,
         ({ sizeof(signed short int); }),
         "({ sizeof(signed short int); })",
         false);
  assert(2,
         ({ sizeof(unsigned short); }),
         "({ sizeof(unsigned short); })",
         false);
  assert(2,
         ({ sizeof(unsigned short int); }),
         "({ sizeof(unsigned short int); })",
         false);

  assert(4, ({ sizeof(int); }), "({ sizeof(int); })", false);
  assert(4, ({ sizeof(signed); }), "({ sizeof(signed); })", false);
  assert(4, ({ sizeof(unsigned); }), "({ sizeof(unsigned); })", false);
  assert(4, ({ sizeof(signed int); }), "({ sizeof(signed int); })", false);
  assert(
      4, ({ sizeof(unsigned int); }), "({ sizeof(unsigned int); })", false);

  assert(8, ({ sizeof(long); }), "({ sizeof(long); })", false);
  assert(8, ({ sizeof(long int); }), "({ sizeof(long int); })", false);
  assert(8, ({ sizeof(long long); }), "({ sizeof(long long); })", false);
  assert(
      8, ({ sizeof(long long int); }), "({ sizeof(long long int); })", false);

  assert(8, ({ sizeof(signed long); }), "({ sizeof(signed long); })", false);
  assert(8,
         ({ sizeof(signed long int); }),
         "({ sizeof(signed long int); })",
         false);
  assert(8,
         ({ sizeof(signed long long); }),
         "({ sizeof(signed long long); })",
         false);
  assert(8,
         ({ sizeof(signed long long int); }),
         "({ sizeof(signed long long int); })",
         false);

  assert(
      8, ({ sizeof(unsigned long); }), "({ sizeof(unsigned long); })", false);
  assert(8,
         ({ sizeof(unsigned long int); }),
         "({ sizeof(unsigned long int); })",
         false);
  assert(8,
         ({ sizeof(unsigned long long); }),
         "({ sizeof(unsigned long long); })",
         false);
  assert(8,
         ({ sizeof(unsigned long long int); }),
         "({ sizeof(unsigned long long int); })",
         false);

  assert(1,
         ({
           g16.a = 1;
           g16.a;
         }),
         "({ g16.a=1; g16.a; })",
         false);
  assert(1,
         ({
           g16.d.a = 1;
           g16.d.a;
         }),
         "({ g16.d.a=1; g16.d.a; })",
         false);

  assert(3,
         ({
           const int a = 3;
           a;
         }),
         "({ const int a=3; a; })",
         false);

  assert(3,
         ({
           int a = 3;
           const int *p = &a;
           *p;
         }),
         "({ int a=3; const int *p=&a; *p; })",
         false);
  assert(3,
         ({
           int a = 3;
           int const *p = &a;
           *p;
         }),
         "({ int a=3; int const *p=&a; *p; })",
         false);

  assert(3,
         ({
           int a = 3;
           const int *const p = &a;
           *p;
         }),
         "({ int a=3; const int *const p=&a; *p; })",
         false);

  assert(3,
         ({
           int a = 3;
           const int *const restrict x = &a;
           *x;
         }),
         "({ int a=3; const int *const restrict x=&a; *x; })",
         false);

  assert(4294967295, ({ 0xffffffff; }), "({ 0xffffffff; })", false);
  assert(4294967295U, ({ 0xffffffffU; }), "({ 0xffffffffU; })", true);
  assert(-1, ({ 0xffffffffffffffff; }), "({ 0xffffffffffffffff; })", false);
  assert(
      -1, ({ 0xffffffffffffffffLL; }), "({ 0xffffffffffffffffLL; })", false);
  assert(18446744073709551615LLU,
         ({ 0xffffffffffffffffLLU; }),
         "({ 0xffffffffffffffffLLU; })",
         true);

  if (success == number)
    p2("result: \x1b[32mOK\x1b[0m, ");
  else
    p2("result: \x1b[31mFAILED\x1b[0m, ");

  p2("%d success; %d failed; %d tests;\n", success, failed, number);
  return 0;
}
