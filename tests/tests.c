/*
 * Comment test
 *
 *
 *
 *
 */

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

#if 0
#else

#define def
#ifdef def
#else
#endif

#endif

#undef def

int assert(int expected, int actual, char *code) {
  p("% 4d: ", number++);
  if (expected == actual) {
    p("%s => %d ... " OK "\n", code, actual);
    success++;
  } else {
    p("%s => %d expected, but got %d ... %s\n",
      code,
      expected,
      actual,
      FAILED);

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
int main() {
  assert(0, 0, "0");
  assert(42, 42, "42");
  assert(21, 5 + 20 - 4, "5+20-4");
  assert(41, 12 + 34 - 5, "12+34-5");
  assert(47, 5 + 6 * 7, "5+6*7");
  assert(15, 5 * (9 - 6), "5*(9-6)");
  assert(4, (3 + 5) / 2, "(3+5)/2");
  assert(3, +3, "+3");
  assert(8, -(-3 - 5), "-(-3-5)");
  assert(15, -3 * (-5), "-3*(-5)");
  assert(1, 1 == 1, "1==1");
  assert(0, 1 != 1, "1!=1");
  assert(1, 2 >= 1, "2>=1");
  assert(1, 1 <= 2, "1<=2");
  assert(1, 2 > 1, "2>1");
  assert(1, 1 < 2, "1<2");
  assert(5,
         ({
           int a;
           a = 3;
           a + 2;
         }),
         "({ int a; a=3; a+2; })");
  assert(2,
         ({
           int a;
           int b;
           a = b = 2;
           a;
         }),
         "({ int a; int b; a=b=2; a; })");
  assert(15,
         ({
           int foo;
           foo = 3;
           foo + 12;
         }),
         "({ int foo; foo=3; foo+12; })");

  assert(3,
         ({
           int a = 3;
           a;
         }),
         "({ int a=3; a; })");

  assert(15, ({ add(5, 10); }), "({ add(5, 10); })");
  assert(15,
         ({
           int a = add(5, 10);
           a;
         }),
         "({ int a=add(5, 10); a; })");
  assert(3, ({ sub(5, 2); }), "({ sub(5, 2); })");
  assert(2, ({ sub(5, 3); }), "({ sub(5, 3); })");
  assert(1, ({ sub(5, 2) - sub(5, 3); }), "({ sub(5, 2)-sub(5, 3); })");
  assert(55, ({ fib(9); }), "({ fib(9); })");
  assert(3,
         ({
           int i;
           for (i = 0; i < 3; i = i + 1) {
             i = i;
           }
           i;
         }),
         "({ int i; for(i=0; i<=3; i=i+1){ i=i; } i; })");
  assert(3,
         ({
           int a;
           for (int i = 0; i <= 3; i = i + 1) {
             a = i;
           }
           a;
         }),
         "({ int a; for(int i=0; i<=3; i=i+1){ a=i; } a; })");
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
         "({ int a; int i=0; while(i<=3){ i=i+1; a=i; } a; })");
  assert(3,
         ({
           int x;
           int *y = &x;
           *y = 3;
           x;
         }),
         "({ int x; int *y; y=&x; *y=3; x; })");
  assert(3,
         ({
           int x;
           int *y = &x;
           *y = 3;
           x;
         }),
         "({ int x; int *y=&x; *y=3; x; })");
  assert(8,
         ({
           int x;
           int y;
           x = 3;
           y = 5;
           add(x, y);
         }),
         "({ int x; int y; x=3; y=5; add(x, y); })");
  assert(3,
         ({
           int x = 3;
           *&x;
         }),
         "({ int x=3; *&x; })");
  assert(5,
         ({
           int x = 3;
           int y = 5;
           *(&x + 1);
         }),
         "({ int x=3; int y=5; *(&x+1); })");
  assert(3,
         ({
           int x = 3;
           int y = 5;
           *(&y - 1);
         }),
         "({ int x=3; int y=5; *(&y-1); })");
  assert(4,
         ({
           int x;
           sizeof(x);
         }),
         "({ int x; sizeof(x); })");
  assert(8,
         ({
           int *x;
           sizeof(x);
         }),
         "({ int *x; sizeof(x); })");
  assert(4,
         ({
           int x;
           sizeof(x + 3);
         }),
         "({ int x; sizeof(x+3); })");
  assert(8,
         ({
           int *x;
           sizeof(x + 5);
         }),
         "({ int *x; sizeof(x+5); })");
  assert(4,
         ({
           int *x;
           sizeof(*x);
         }),
         "({ int *x; sizeof(*x); })");
  assert(4, ({ sizeof(1); }), "({ sizeof(1); })");
  assert(5, ({ sizeof(1) + 1; }), "({ sizeof(1)+1; })");
  assert(4, ({ sizeof(sizeof(1)); }), "({ sizeof(sizeof(1)); })");
  assert(3,
         ({
           int x[2];
           int *y = &x;
           *y = 3;
           *x;
         }),
         "({ int x[2]; int *y=&x; *y=3; *x; })");
  assert(3,
         ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *x;
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; })");
  assert(4,
         ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *(x + 1);
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); })");
  assert(5,
         ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); })");
  assert(1,
         ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *p;
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *p; })");
  assert(2,
         ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *(p + 1);
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *(p+1); })");
  assert(3,
         ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *p + *(p + 1);
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *p+*(p+1); })");
  assert(8,
         ({
           int a[2];
           sizeof(a);
         }),
         "({ int a[2]; sizeof(a); })");
  assert(3,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           *x;
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; *x; })");
  assert(4,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           x[1];
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; x[1]; })");
  assert(4,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 1);
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; *(x+1); })");
  assert(5,
         ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           x[2];
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; x[2]; })");
  assert(0,
         ({
           int x[2][3];
           int *y = x;
           *y = 0;
           **x;
         }),
         "({ int x[2][3]; int *y=x; *y=0; **x; })");
  assert(1,
         ({
           int x[2][3];
           int *y = x;
           *(y + 1) = 1;
           *(*x + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); })");
  assert(2,
         ({
           int x[2][3];
           int *y = x;
           *(y + 2) = 2;
           *(*x + 2);
         }),
         "({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); })");
  assert(3,
         ({
           int x[2][3];
           int *y = x;
           *(y + 3) = 3;
           **(x + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); })");
  assert(4,
         ({
           int x[2][3];
           int *y = x;
           *(y + 4) = 4;
           *(*(x + 1) + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); })");
  assert(5,
         ({
           int x[2][3];
           int *y = x;
           *(y + 5) = 5;
           *(*(x + 1) + 2);
         }),
         "({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); })");
  assert(3,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *x;
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; })");
  assert(4,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 1);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); })");
  assert(5,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
  assert(5,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
  assert(5,
         ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
  assert(0,
         ({
           int x[2][3];
           int *y = x;
           y[0] = 0;
           x[0][0];
         }),
         "({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; })");
  assert(1,
         ({
           int x[2][3];
           int *y = x;
           y[1] = 1;
           x[0][1];
         }),
         "({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; })");
  assert(2,
         ({
           int x[2][3];
           int *y = x;
           y[2] = 2;
           x[0][2];
         }),
         "({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; })");
  assert(3,
         ({
           int x[2][3];
           int *y = x;
           y[3] = 3;
           x[1][0];
         }),
         "({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; })");
  assert(4,
         ({
           int x[2][3];
           int *y = x;
           y[4] = 4;
           x[1][1];
         }),
         "({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; })");
  assert(5,
         ({
           int x[2][3];
           int *y = x;
           y[5] = 5;
           x[1][2];
         }),
         "({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; })");
  assert(1, ({ g0; }), "({ g0; })");
  assert(3,
         ({
           g1 = 3;
           g1;
         }),
         "({ g1=3; g1; })");
  assert(0,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[0];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[0]; })");
  assert(1,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[1];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[1]; })");
  assert(2,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[2];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[2]; })");
  assert(3,
         ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[3];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[3]; })");
  assert(16,
         ({
           int x[4];
           sizeof(x);
         }),
         "({ int x[4]; sizeof(x); })");
  assert(1,
         ({
           char a;
           sizeof(a);
         }),
         "({ char a; sizeof(a); })");
  assert(10,
         ({
           char a[10];
           sizeof(a);
         }),
         "({ char a[10]; sizeof(a); })");
  assert(3,
         ({
           char a = 1;
           char b = 2;
           a + b;
         }),
         "({ char a=1; char b=2; a+b; })");
  assert(3,
         ({
           char x[3];
           x[0] = -1;
           x[1] = 2;
           int y = 4;
           x[0] + y;
         }),
         "({ char x[3]; x[0]=-1; x[1]=2; int y; y=4; x[0]+y; })");
  assert(97, ({ "abc"[0]; }), "({ \"abc\"[0]; })");
  assert(98, ({ "abc"[1]; }), "({ \"abc\"[1]; })");
  assert(97,
         ({
           char *a = "abc";
           a[0];
         }),
         "({ char *a=\"abc\"; a[0]; })");
  assert(98,
         ({
           char *a = "abc";
           a[1];
         }),
         "({ char *a=\"abc\"; a[1]; })");

  assert(101,
         ({
           char *a = "abc", *b = "def";
           b[1];
         }),
         "({ char *a=\"abc\",*b=\"def\"; b[1]; })");
  assert(3,
         ({
           int x, y;
           x = 3;
           x;
         }),
         "({ int x,y; x=3; x; })");

  assert(3,
         ({
           int x = 1, y = 2;
           x + y;
         }),
         "({ int x=1, y=2; x+y; })");

  assert(1,
         ({
           int a[] = {1};
           a[0];
         }),
         "({ int a[]={1}; a[0]; })");
  assert(2,
         ({
           int a[] = {1, 2};
           a[1];
         }),
         "({ int a[]={1,2}; a[1]; })");
  assert(3,
         ({
           int a[3] = {1, 2, 3};
           a[2];
         }),
         "({ int a[3]={1,2,3}; a[2]; })");
  assert(1,
         ({
           int a[] = {1, 2, foo()};
           a[2];
         }),
         "({ int a[]={1,2,foo()}; a[2]; })");

  assert(98,
         ({
           char a[] = "abc";
           a[1];
         }),
         "({ char a[]=\"abc\"; a[1]; })");

  assert(98,
         ({
           char a[4] = "abc";
           a[1];
         }),
         "({ char a[4]=\"abc\"; a[1]; })");

  assert(97, ({ g6[0]; }), "({ g6[0]; })");
  assert(97, ({ g7[0]; }), "({ g7[0]; })");
  assert(3,
         ({
           g1 = 3;
           *g8;
         }),
         "({ g1=3; *g8; })");

  assert(3, ({ g10[3]; }), "({ g10[3]; })");
  assert(97, ({ g11[0][0]; }), "({ g11[0][0]; })");

  assert(1,
         ({
           int a = 1;
           void0();
           a;
         }),
         "({ int a=1; void0(); a; })");

  assert(2,
         ({
           int a = 1;
           short b = 2;
           long c = 2;
           c;
         }),
         "({ int a=1; short b=2; long c=2; c; })");

  assert(2,
         ({
           int a = 0;
           ++a;
           ++a;
         }),
         "({ int a=0; ++a; ++a; })");

  assert(1,
         ({
           int a = 3;
           --a;
           --a;
         }),
         "({ int a=3; --a; --a; })");

  assert(2,
         ({
           int a = 0;
           a++;
           a++;
         }),
         "({ int a=0; a++; a++; })");

  assert(1,
         ({
           int a = 3;
           a--;
           a--;
         }),
         "({ int a=3; a--; a--; })");

  assert(4,
         ({
           int a = 0;
           a += 2;
           a += 2;
         }),
         "({ int a=0; a+=2; a+=2; })");

  assert(1,
         ({
           int a = 5;
           a -= 2;
           a -= 2;
         }),
         "({ int a=5; a-=2; a-=2; })");
  assert(98,
         ({
           char *a = "abc";
           *a++;
         }),
         "({ char *a=\"abc\"; *a++; })");
  assert(99,
         ({
           char *a = "abc";
           *a += 2;
         }),
         "({ char *a=\"abc\"; *a+=2; })");

  assert(6,
         ({
           int a = 2;
           a *= 3;
         }),
         "({ int a=2; a*=3; })");

  assert(2,
         ({
           int a = 6;
           a /= 3;
         }),
         "({ int a=6; a/=3; })");

  assert(1,
         ({
           int a = 1;
           a = a == 1 ? a : 2;
           a;
         }),
         "({ int a=1; a = a==1 ? a:2; })");

  assert(2,
         ({
           int a = 2;
           a = a == 1 ? a : 2;
           a;
         }),
         "({ int a=2; a = a==1 ? a:2; })");

  assert(1,
         ({
           int a = 0;
           int b = 1;
           a | b;
         }),
         "({ int a=0; int b=1; a|b; })");
  assert(0,
         ({
           int a = 0;
           int b = 0;
           a | b | b;
         }),
         "({ int a=0; int b=0; a|b|b; })");
  assert(0,
         ({
           int a = 0;
           int b = 1;
           int c = a & b;
           c;
         }),
         "({ int a=0; int b=1; int c=a&b; c; })");
  assert(1,
         ({
           int a = 1;
           int b = 1;
           int c = a & b;
           c;
         }),
         "({ int a=1; int b=1; int c=a&b; c; })");

  assert(0,
         ({
           int a = 1;
           int b = 1;
           int c = a ^ b;
           c;
         }),
         "({ int a=1; int b=1; int c=a^b; c; })");

  assert(1,
         ({
           int a = 0;
           int b = 1;
           int c = a ^ b;
           c;
         }),
         "({ int a=0; int b=1; int c=a^b; c; })");

  assert(1,
         ({
           char a = 1;
           int b = (int) a;
           b;
         }),
         "({ char a=1; int b=(int)a; b;})");

  assert(3,
         ({
           int a = 3;
           int *b = (int *) &a;
           *b;
         }),
         "({ char a=3; int *b=(int *)&a; *b;})");

  assert(-1, ({ ~0; }), "({ ~0; })");

  assert(5, ({ logor(); }), "({ logor(); })");
  assert(5, ({ logand(); }), "({ logand(); })");

  assert(1,
         ({
           bool a;
           a = 1;
           a;
         }),
         "({ bool a=0; a=1; a; })");

  assert(3,
         ({
           Struct a;
           Struct b;
           a.a = 3;
           a.b = 2;
           a.a;
         }),
         "({ Struct a; a.a=3; a.b=2; a.a; })");

  assert(5,
         ({
           Struct a = {3, 2};
           a.b + a.a;
         }),
         "({ Struct a={3,2}; a.b+a.a; })");

  assert(3,
         ({
           Struct a;
           a.e[0] = 1;
           a.e[1] = 2;
           a.e[0] + a.e[1];
         }),
         "({ Struct a; a.e[0]=1; a.e[1]=2; a.e[0]+a.e[1]; })");

  assert(10,
         ({
           Struct a = {10};
           Struct *p = &a;
           p->a;
         }),
         "({ Struct a={10}; Struct *p=&a; p->a; })");

  assert(10,
         ({
           Struct a;
           Struct *p = &a;
           p->b = 10;
           p->b;
         }),
         "({ Struct a; Struct *p=&a; p->b=10; p->b; })");

  assert(3,
         ({
           Struct a = {1, 2, 3, 4}, *p;
           p = &a;
           p->e[0] = 1;
           p->e[1] = 2;
           p->e[0] + p->e[1];
         }),
         "({ Struct a,*p; p=&a; p->e[0]=1; p->e[1]=2; "
         "p->e[0]+p->e[1]; })");

  assert(5,
         ({
           g13.a = 5;
           g13.a;
         }),
         "({ g13.a=5; g13.a; })");

  assert(10, ({ mixed(1, 2, 3, 4); }), "({ mixed(1,2,3,4); })");

  assert(0, ({ A; }), "({ A; })");
  assert(1, ({ B; }), "({ B; })");
  assert(10, ({ C; }), "({ C; })");
  assert(11, ({ D; }), "({ D; })");

  assert(0,
         ({
           int a = A1;
           a;
         }),
         "({ int a=A1; a; })");
  assert(1,
         ({
           int a = B1;
           a;
         }),
         "({ int a=B1; a; })");
  assert(20,
         ({
           int a = C1;
           a;
         }),
         "({ int a=C1; a; })");
  assert(21,
         ({
           int a = D1;
           a;
         }),
         "({ int a=D1; a; })");

  assert(5,
         ({
           int a = 5;
           { int b = 10; }
           int b = a;
           a;
         }),
         "({ int a=5; { int b = 10; } int b=a; a; })");

  assert(5,
         ({
           int a = 5;
           int b = a;
           { int b = 10; }
           b;
         }),
         "({ int a=5; int b=a; { int b = 10; } b; })");

  assert(2,
         ({
           enum a { X, Y, Z };
           Z;
         }),
         "({ enum a{X,Y,Z}; Z; })");

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
         "({ struct a { int a; int b; }; struct a a; a.a=5; a.a; })");

  assert(1, ({ TRUE; }), "({ TRUE; })");
  assert(128, ({ MAX_LEN; }), "({ MAX_LEN; })");

  assert(5, ({ one(5); }), "({ one(5); })");
  assert(10, ({ one(5) + one(5); }), "({ one(5)+one(5); })");
  assert(1, ({ equal(1, 1); }), "({ equal(1, 1); })");
  assert(4, ({ add_sub(3, 1); }), "({ add_sub(3,1); })");
  assert(5, ({ addadd(2, 3); }), "({ addadd(2,3); })");
  assert(10, ({ addadd((1 + 2), (3 + 4)); }), "({ addadd((1+2),(3+4)); })");
  assert(10,
         ({ add((one(1) + one(2)), (3 + 4)); }),
         "({ add((one(1)+one(2)),(3+4)); })");
  assert(15,
         ({
           int x = 5;
           int y = 10;
           padd(&x, &y);
         }),
         "({ int x=5; int y=10; padd(&x, &y); })");

  assert(97,
         ({
           char *c = "abc\n";
           c[0];
         }),
         "({ char *c=\"abc\\n\"; c[0]; })");

  assert(48 + 50 + 52,
         ({
           char *c[] = {"01", "23", "45"};
           c[0][0] + c[1][0] + c[2][0];
         }),
         "({ char *c[]={\"01\",\"23\",\"45\"}; c[0][0]+c[1][0]+c[2][0]; })");

  assert(3,
         ({
           int a[] = {0, 1, 2};
           a[0] + a[1] + a[2];
         }),
         "({ int a[]={0,1,2}; a[0]+a[1]+a[2]; })");
  assert(9,
         ({
           int d[][2] = {{1, 2}, {3, 4}, {5, 6}};
           d[0][0] + d[1][0] + d[2][0];
         }),
         "({ int d[][2]={{1,2},{3,4},{5,6}}; d[0][0]+d[1][0]+d[2][0]; })");

  assert(7,
         ({
           Struct a = {1, 2, 3, 4, {5, 6, 7}, 0, {9, 10, 11}};
           a.e[2];
         }),
         "({ Struct a={1,2,3,4,{5,6,7},0,{9,10,11}}; a.e[2];})");

  assert(11,
         ({
           Struct a = {1, 2, 3, 4, {5, 6, 7}, 0, {9, 10, 11}};
           a.g.c;
         }),
         "({ Struct a={1,2,3,4,{5,6,7},0,{9,10,11}}; a.g.c;})");

  assert(4,
         ({
           Struct a;
           Struct b = {1, 2, 3, 4};
           a = b;
           a.d;
         }),
         "({ Struct a; Struct b={1,2,3,4}; a=b; a.d; })");

  assert(3, ({ g14.a + g14.b; }), "({ g14.a+g14.b; })");
  assert(5, ({ g15[0].a + g15[1].b; }), "({ g15[0].a+g15[1].b; })");

  assert(11, ({ extern_a; }), "({ extern_a; })");

  assert(8,
         ({
           int a = 1;
           a << 3;
         }),
         "({ int a=1; a<<3; })");
  assert(1,
         ({
           int a = 8;
           a >> 3;
         }),
         "({ int a=8; a>>3; })");

  if (success == number)
    printf("result: \x1b[32mOK\x1b[0m, ");
  else
    printf("result: \x1b[31mFAILED\x1b[0m, ");

  printf("%d success; %d failed; %d tests;\n", success, failed, number);
  return 0;
}
