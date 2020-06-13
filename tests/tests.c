/*
 * Comment test
 *
 *
 *
 *
 */

int number = 0;
int g0 = 1;
int g1, g2;
int g3 = 0, g4 = 0;
int g5[5];

char *g6 = "abc";
char g7[] = "abc";

int *g8 = &g1;
int *g9;

int g10[10] = {0, 1, 2, 3, 4, 5};
char *g11[] = {"abc", "def", "ghi"};
char *g12[5] = {"abc", "def", "ghi"};

// typedef int Int;

int assert(int expected, int actual, char *code) {
  printf("% 3d: ", number++);
  // printf("%s => %d/%d\n", code, actual, expected);
  if (expected == actual) {
    printf("%s => %d\n", code, actual);
  } else {
    printf("%s => %d expected, but got %d\n", code, expected, actual);
    exit(1);
  }
  return 0;
}
int add(int a, int b) { return a + b; }
int sub(int x, int y) { return x - y; }
int fib(int n) {
  if (n <= 1)
    return 1;
  return fib(n - 1) + fib(n - 2);
}
int foo() { return 1; }

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
  assert(5, ({
           int a;
           a = 3;
           a + 2;
         }),
         "({ int a; a=3; a+2; })");
  assert(2, ({
           int a;
           int b;
           a = b = 2;
           a;
         }),
         "({ int a; int b; a=b=2; a; })");
  assert(15, ({
           int foo;
           foo = 3;
           foo + 12;
         }),
         "({ int foo; foo=3; foo+12; })");

  assert(3, ({
           int a = 3;
           a;
         }),
         "({ int a=3; a; })");

  assert(15, ({ add(5, 10); }), "({ add(5, 10); })");
  assert(15, ({
           int a = add(5, 10);
           a;
         }),
         "({ int a=add(5, 10); a; })");
  assert(3, ({ sub(5, 2); }), "({ sub(5, 2); })");
  assert(2, ({ sub(5, 3); }), "({ sub(5, 3); })");
  assert(1, ({ sub(5, 2) - sub(5, 3); }), "({ sub(5, 2)-sub(5, 3); })");
  assert(55, ({ fib(9); }), "({ fib(9); })");
  assert(3, ({
           int i;
           for (i = 0; i < 3; i = i + 1) {
             i = i;
           }
           i;
         }),
         "({ int i; for(i=0; i<=3; i=i+1){ i=i; } i; })");
  // assert(3, ({
  //          int a;
  //          for (int i = 0; i <= 3; i = i + 1) {
  //            a = i;
  //          }
  //          a;
  //        }),
  //        "({ int a; for(int i=0; i<=3; i=i+1){ a=i; } a; })");
  // assert(3, ({
  //          int a;
  //          int i = 0;
  //          while (i < 3) {
  //            i = i + 1;
  //            a = i;
  //          }
  //          a;
  //        }),
  //        "({ int a; int i=0; while(i<=3){ i=i+1; a=i; } a; })");
  assert(3, ({
           int x;
           int *y = &x;
           *y = 3;
           x;
         }),
         "({ int x; int *y; y=&x; *y=3; x; })");
  assert(3, ({
           int x;
           int *y = &x;
           *y = 3;
           x;
         }),
         "({ int x; int *y=&x; *y=3; x; })");
  assert(8, ({
           int x;
           int y;
           x = 3;
           y = 5;
           add(x, y);
         }),
         "({ int x; int y; x=3; y=5; add(x, y); })");
  assert(3, ({
           int x = 3;
           *&x;
         }),
         "({ int x=3; *&x; })");
  assert(5, ({
           int x = 3;
           int y = 5;
           *(&x + 1);
         }),
         "({ int x=3; int y=5; *(&x+1); })");
  assert(3, ({
           int x = 3;
           int y = 5;
           *(&y - 1);
         }),
         "({ int x=3; int y=5; *(&y-1); })");
  assert(4, ({
           int x;
           sizeof(x);
         }),
         "({ int x; sizeof(x); })");
  assert(8, ({
           int *x;
           sizeof(x);
         }),
         "({ int *x; sizeof(x); })");
  assert(4, ({
           int x;
           sizeof(x + 3);
         }),
         "({ int x; sizeof(x+3); })");
  assert(8, ({
           int *x;
           sizeof(x + 5);
         }),
         "({ int *x; sizeof(x+5); })");
  assert(4, ({
           int *x;
           sizeof(*x);
         }),
         "({ int *x; sizeof(*x); })");
  assert(4, ({ sizeof(1); }), "({ sizeof(1); })");
  assert(5, ({ sizeof(1) + 1; }), "({ sizeof(1)+1; })");
  assert(4, ({ sizeof(sizeof(1)); }), "({ sizeof(sizeof(1)); })");
  assert(3, ({
           int x[2];
           int *y = &x;
           *y = 3;
           *x;
         }),
         "({ int x[2]; int *y=&x; *y=3; *x; })");
  assert(3, ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *x;
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; })");
  assert(4, ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *(x + 1);
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); })");
  assert(5, ({
           int x[3];
           *x = 3;
           *(x + 1) = 4;
           *(x + 2) = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); })");
  assert(1, ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *p;
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *p; })");
  assert(2, ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *(p + 1);
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *(p+1); })");
  assert(3, ({
           int a[2];
           *a = 1;
           *(a + 1) = 2;
           int *p = a;
           *p + *(p + 1);
         }),
         "({ int a[2]; *a=1; *(a+1)=2; int *p=a; *p+*(p+1); })");
  assert(8, ({
           int a[2];
           sizeof(a);
         }),
         "({ int a[2]; sizeof(a); })");
  assert(3, ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           *x;
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; *x; })");
  assert(4, ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           x[1];
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; x[1]; })");
  assert(4, ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 1);
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; *(x+1); })");
  assert(5, ({
           int x[3];
           x[0] = 3;
           x[1] = 4;
           x[2] = 5;
           x[2];
         }),
         "({ int x[3]; x[0]=3; x[1]=4; x[2]=5; x[2]; })");
  assert(0, ({
           int x[2][3];
           int *y = x;
           *y = 0;
           **x;
         }),
         "({ int x[2][3]; int *y=x; *y=0; **x; })");
  assert(1, ({
           int x[2][3];
           int *y = x;
           *(y + 1) = 1;
           *(*x + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); })");
  assert(2, ({
           int x[2][3];
           int *y = x;
           *(y + 2) = 2;
           *(*x + 2);
         }),
         "({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); })");
  assert(3, ({
           int x[2][3];
           int *y = x;
           *(y + 3) = 3;
           **(x + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); })");
  assert(4, ({
           int x[2][3];
           int *y = x;
           *(y + 4) = 4;
           *(*(x + 1) + 1);
         }),
         "({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); })");
  assert(5, ({
           int x[2][3];
           int *y = x;
           *(y + 5) = 5;
           *(*(x + 1) + 2);
         }),
         "({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); })");
  assert(3, ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *x;
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; })");
  assert(4, ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 1);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); })");
  assert(5, ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
  assert(5, ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
  assert(5, ({
           int x[3];
           *x = 3;
           x[1] = 4;
           x[2] = 5;
           *(x + 2);
         }),
         "({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); })");
  assert(0, ({
           int x[2][3];
           int *y = x;
           y[0] = 0;
           x[0][0];
         }),
         "({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; })");
  assert(1, ({
           int x[2][3];
           int *y = x;
           y[1] = 1;
           x[0][1];
         }),
         "({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; })");
  assert(2, ({
           int x[2][3];
           int *y = x;
           y[2] = 2;
           x[0][2];
         }),
         "({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; })");
  assert(3, ({
           int x[2][3];
           int *y = x;
           y[3] = 3;
           x[1][0];
         }),
         "({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; })");
  assert(4, ({
           int x[2][3];
           int *y = x;
           y[4] = 4;
           x[1][1];
         }),
         "({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; })");
  assert(5, ({
           int x[2][3];
           int *y = x;
           y[5] = 5;
           x[1][2];
         }),
         "({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; })");
  assert(1, ({ g0; }), "({ g0; })");
  assert(3, ({
           g1 = 3;
           g1;
         }),
         "({ g1=3; g1; })");
  assert(0, ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[0];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[0]; })");
  assert(1, ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[1];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[1]; })");
  assert(2, ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[2];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[2]; })");
  assert(3, ({
           int x[4];
           x[0] = 0;
           x[1] = 1;
           x[2] = 2;
           x[3] = 3;
           x[3];
         }),
         "({ int x[4]; x[0]=0; x[1]=1; x[2]=2; x[3]=3; x[3]; })");
  assert(16, ({
           int x[4];
           sizeof(x);
         }),
         "({ int x[4]; sizeof(x); })");
  assert(1, ({
           char a;
           sizeof(a);
         }),
         "({ char a; sizeof(a); })");
  assert(10, ({
           char a[10];
           sizeof(a);
         }),
         "({ char a[10]; sizeof(a); })");
  assert(3, ({
           char a = 1;
           char b = 2;
           a + b;
         }),
         "({ char a=1; char b=2; a+b; })");
  assert(3, ({
           char x[3];
           x[0] = -1;
           x[1] = 2;
           int y = 4;
           x[0] + y;
         }),
         "({ char x[3]; x[0]=-1; x[1]=2; int y; y=4; x[0]+y; })");
  assert(97, ({ "abc"[0]; }), "({ \"abc\"[0]; })");
  assert(98, ({ "abc"[1]; }), "({ \"abc\"[1]; })");
  assert(97, ({
           char *a = "abc";
           a[0];
         }),
         "({ char *a=\"abc\"; a[0]; })");
  assert(98, ({
           char *a = "abc";
           a[1];
         }),
         "({ char *a=\"abc\"; a[1]; })");

  assert(101, ({
           char *a = "abc", *b = "def";
           b[1];
         }),
         "({ char *a=\"abc\",*b=\"def\"; b[1]; })");
  assert(3, ({
           int x, y;
           x = 3;
           x;
         }),
         "({ int x,y; x=3; x; })");

  assert(3, ({
           int x = 1, y = 2;
           x + y;
         }),
         "({ int x=1, y=2; x+y; })");

  assert(1, ({
           int a[] = {1};
           a[0];
         }),
         "({ int a[]={1}; a[0]; })");
  assert(2, ({
           int a[] = {1, 2};
           a[1];
         }),
         "({ int a[]={1,2}; a[1]; })");
  assert(3, ({
           int a[3] = {1, 2, 3};
           a[2];
         }),
         "({ int a[3]={1,2,3}; a[2]; })");
  assert(1, ({
           int a[] = {1, 2, foo()};
           a[2];
         }),
         "({ int a[]={1,2,foo()}; a[2]; })");

  assert(98, ({
           char a[] = "abc";
           a[1];
         }),
         "({ char a[]=\"abc\"; a[1]; })");

  assert(98, ({
           char a[4] = "abc";
           a[1];
         }),
         "({ char a[4]=\"abc\"; a[1]; })");

  assert(97, ({ g6[0]; }), "({ g6[0]; })");
  assert(97, ({ g7[0]; }), "({ g7[0]; })");
  assert(3, ({
           g1 = 3;
           *g8;
         }),
         "({ g1=3; *g8; })");

  assert(3, ({ g10[3]; }), "({ g10[3]; })");
  assert(97, ({ g11[0][0]; }), "({ g11[0][0]; })");

  assert(1, ({
           int a = 1;
           void0();
           a;
         }),
         "({ int a=1; void0(); a; })");

  assert(2, ({
           int a = 1;
           short b = 2;
           long c = 2;
           c;
         }),
         "({ int a=1; short b=2; long c=2; c; })");

  assert(2, ({
           int a = 0;
           ++a;
           ++a;
         }),
         "({ int a=0; ++a; ++a; })");

  assert(1, ({
           int a = 3;
           --a;
           --a;
         }),
         "({ int a=3; --a; --a; })");

  assert(2, ({
           int a = 0;
           a++;
           a++;
         }),
         "({ int a=0; a++; a++; })");

  assert(1, ({
           int a = 3;
           a--;
           a--;
         }),
         "({ int a=3; a--; a--; })");

  assert(4, ({
           int a = 0;
           a += 2;
           a += 2;
         }),
         "({ int a=0; a+=2; a+=2; })");

  assert(1, ({
           int a = 5;
           a -= 2;
           a -= 2;
         }),
         "({ int a=5; a-=2; a-=2; })");
  assert(98, ({
           char *a = "abc";
           *a++;
         }),
         "({ char *a=\"abc\"; *a++; })");
  assert(99, ({
           char *a = "abc";
           *a += 2;
         }),
         "({ char *a=\"abc\"; *a+=2; })");

  assert(6, ({
           int a = 2;
           a *= 3;
         }),
         "({ int a=2; a*=3; })");

  assert(2, ({
           int a = 6;
           a /= 3;
         }),
         "({ int a=6; a/=3; })");

  assert(1, ({
           int a = 1;
           a = a == 1 ? a : 2;
           a;
         }),
         "({ int a=1; a = a==1 ? a:2; })");

  assert(2, ({
           int a = 2;
           a = a == 1 ? a : 2;
           a;
         }),
         "({ int a=2; a = a==1 ? a:2; })");

  assert(1, ({
           int a = 0;
           int b = 1;
           a | b;
         }),
         "({ int a=0; int b=1; a|b; })");
  assert(0, ({
           int a = 0;
           int b = 0;
           a | b | b;
         }),
         "({ int a=0; int b=0; a|b|b; })");
  assert(0, ({
           int a = 0;
           int b = 1;
           int c = a & b;
           c;
         }),
         "({ int a=0; int b=1; int c=a&b; c; })");
  assert(1, ({
           int a = 1;
           int b = 1;
           int c = a & b;
           c;
         }),
         "({ int a=1; int b=1; int c=a&b; c; })");

  assert(0, ({
           int a = 1;
           int b = 1;
           int c = a ^ b;
           c;
         }),
         "({ int a=1; int b=1; int c=a^b; c; })");

  assert(1, ({
           int a = 0;
           int b = 1;
           int c = a ^ b;
           c;
         }),
         "({ int a=0; int b=1; int c=a^b; c; })");

  assert(1, ({
           char a = 1;
           int b = (int)a;
           b;
         }),
         "({ char a=1; int b=(int)a; b;})");

  assert(3, ({
           int a = 3;
           int *b = (int *)&a;
           *b;
         }),
         "({ char a=3; int *b=(int *)&a; *b;})");

  return 0;
}
