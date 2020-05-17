#!/bin/bash

num=1

assert() {
    expected="$1"
    input="$2"

    echo $2 > tmp.c
    ./kcc tmp.c > tmp.s
    cc -fno-common -static -o tmp tmp.s
    ./tmp
    actual="$?"

    printf "% 2d: " $num
    if [[ "$actual" = "$expected" ]]; then
        echo "'$input' => $actual"
    else
        echo "'$input' => $expected expected, but got $actual"
        echo NG
        exit 1
    fi
    num=$(( $num + 1 ))
}

assert_link() {
    expected="$1"
    input="$2"
    link="$3"

    ./kcc "$input" > tmp.s
    cc -c ${link}.c
    cc -o tmp tmp.s ${link}.o
    ./tmp > /dev/null
    actual="$?"

    printf "% 2d: " $num
    if [[ "$actual" = "$expected" ]]; then
        echo "'$input' => $actual"
    else
        echo "'$input' => $expected expected, but got $actual"
        echo NG
        exit 1
    fi
    num=$(( $num + 1 ))
}

# assert 0 'int main(){return 0;}'
# assert 42 'int main(){return 42;}'
# assert 21 "int main(){return 5+20-4;}"
# assert 41 "int main(){return 12 + 34 - 5;}"

# assert 47 'int main(){return 5+6*7;}'
# assert 15 'int main(){return 5*(9-6);}'
# assert 4 'int main(){return (3 + 5) / 2;}'
# assert 3 'int main(){return +3;}'
# assert 8 'int main(){return -(-3-5);}'
# assert 15 'int main(){return -3*(-5);}'
# assert 1 'int main(){return 1 == 1;}'
# assert 0 'int main(){return 1 != 1;}'
# assert 1 'int main(){return 2 >= 1;}'
# assert 1 'int main(){return 1 <= 2;}'
# assert 1 'int main(){return 2 > 1;}'
# assert 1 'int main(){return 1 < 2;}'

# assert 5 'int main(){int a; a=3;return a+2;}'
# assert 2 'int main(){int a; int b; a=b=2; return a;}'
# assert 15 'int main(){int foo; foo=3;return foo+12;}'
# assert 15 'int main() {return add(5, 10);} int add(int a,int b) {return a + b;}'

assert 1 'int main(){if (1==1) return 1;}'
assert 2 'int main(){if (1!=1) return 1; else return 2;}'
assert 0 'int main(){int i; i=0;while(i<3) return 0; return 1;}'
assert 3 'int main(){int i; i=0;while(i<3) i=i+1; return i;}'
assert 0 'int main(){int i; i=0;while(i<3) return 0; return 1;}'
assert 3 'int main(){int i;for(i = 0; i<3; i=i+1) i = i; return i;}'
assert 0 'int main(){for(;;) return 0;}'
assert 3 'int main(){int i;i=1; i= i+1; i=i+1; return i;}'
assert 6 'int main(){int i;for(i = 0; i<5; i=i+1){i = i +1; i= i+1;} return i;}'
assert 5 'int main(){int i; i=0; if(i==0) { i=5; return 5; }}'
assert 5 'int main(){int i; i=0; if(i==0) { i=5; return 5; }}'

# assert_link 5 'main(){a=0;a = foo(); return a;}' foo
# assert_link 10 'main(){a=0;a =5 + foo(); return a;}' foo
# assert_link 0 'main(){a=0;bar(); return a;}' foo
# assert_link 11 'main(){a=3; a= foo2(a, 3+ 5); return a;}' foo
# assert_link 6 'main(){a= foo3( 1, 2, 3);return a;}' foo
# assert_link 8 'main(){return foo3( 1 + 2, 2, 3);}' foo
# assert_link 5 'main(){return foo();}' foo

# assert 3 'main(){x = 3; y = &x; return *y;}'
# assert 3 'main(){x = 3; y = 5; z = &y - 8; return *z;}'
# assert 5 'foo() {return 5;} main() { return foo();}'
# assert 10 'foo() {return 5;} main() { a = foo() + 5; return a;}'

# assert 10 'int foo(int x) {return x;} int main() { int a1; a1= foo(5) + 5; return a1;}'
# assert 3 'int main(){return sub(5, 2);} int sub(int x,int y){return x -y;}'
# assert 1 'int main(){return sub(5, 2) -sub(5, 3);} int sub(int x,int y){return x -y;}'
# assert 55 'int fib(int n){if(n<=1) return 1; return fib(n-1) + fib(n-2);} int main(){return fib(9);}'

# assert 3 'int main(){int x; int *y; y = &x; *y = 3; return x;}'
# assert 3 'int main(){int x; int *y; y = &x; *y = 3; return x;}'
# assert 8 'int main(){int x; int y; x = 3; y = 5; return add(x, y);} int add(int x,int y){return x + y;}'
# assert 3 'int main(){int x; x = 3; return *&x;}'

# assert 5 'int main(){ int x; x=3; int y;y=5; return *(&x+1); }'
# assert 3 'int main(){ int x; x=3; int y;y=5; return *(&y-1); }'
# assert 8 'int main(){ int x; return sizeof(x); }'
# assert 8 'int main(){ int *x; return sizeof(x); }'
# assert 8 'int main(){ int x; return sizeof(x + 3); }'
# assert 8 'int main(){ int *x; return sizeof(x + 5); }'
# assert 8 'int main(){ int *x; return sizeof(*x); }'
# assert 8 'int main(){ return sizeof(1); }'
# assert 9 'int main(){ return sizeof(1) + 1; }'
# assert 8 'int main(){ return sizeof(sizeof(1)); }'


# assert 3 'int main(){ int x[2]; int *y; y=&x; *y=3; return *x; }'

# assert 3 'int main(){ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
# assert 4 'int main(){ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
# assert 5 'int main(){ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

# assert 1 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p; }'
# assert 2 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *(p + 1); }'
# assert 3 'int main(){ int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }'
# assert 16 'int main(){int a[2]; return sizeof(a);}'

# assert 3 'int main(){ int x[3]; x[0]=3; x[1]=4; x[2]=5; return *x; }'
# assert 4 'int main(){ int x[3]; x[0]=3; x[1]=4; x[2]=5; return x[1]; }'
# assert 4 'int main(){ int x[3]; x[0]=3; x[1]=4; x[2]=5; return *(x + 1); }'
# assert 5 'int main(){ int x[3]; x[0]=3; x[1]=4; x[2]=5; return x[2]; }'

# assert 0 'int main() { int x[2][3]; int *y; y=x; *y=0; return **x; }'
# assert 1 'int main() { int x[2][3]; int *y; y=x; *(y+1)=1; return *(*x+1); }'
# assert 2 'int main() { int x[2][3]; int *y; y=x; *(y+2)=2; return *(*x+2); }'
# assert 3 'int main() { int x[2][3]; int *y; y=x; *(y+3)=3; return **(x+1); }'
# assert 4 'int main() { int x[2][3]; int *y; y=x; *(y+4)=4; return *(*(x+1)+1); }'
# assert 5 'int main() { int x[2][3]; int *y; y=x; *(y+5)=5; return *(*(x+1)+2); }'

# assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
# assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
# assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
# assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
# # assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'

# assert 0 'int main() { int x[2][3]; int *y; y=x; y[0]=0; return x[0][0]; }'
# assert 1 'int main() { int x[2][3]; int *y; y=x; y[1]=1; return x[0][1]; }'
# assert 2 'int main() { int x[2][3]; int *y; y=x; y[2]=2; return x[0][2]; }'
# assert 3 'int main() { int x[2][3]; int *y; y=x; y[3]=3; return x[1][0]; }'
# assert 4 'int main() { int x[2][3]; int *y; y=x; y[4]=4; return x[1][1]; }'
# assert 5 'int main() { int x[2][3]; int *y; y=x; y[5]=5; return x[1][2]; }'

# assert 0 'int x; int main() { return x; }'
# assert 3 'int x; int main() { x=3; return x; }'
# assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
# assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
# assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
# assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
# assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
# assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

# assert 8 'int x; int main() { return sizeof(x); }'
# assert 32 'int x[4]; int main() { return sizeof(x); }'

# assert 1 'int main(){ char a; return sizeof(a);}'
# assert 10 'int main(){ char a[10]; return sizeof(a);}'
# assert 3 'int main(){ char a; a = 1; char b; b = 2; return a + b;}'
# assert 3 'int main(){ char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y;}'

assert 97 'int main(){{char *a; a = "abc"; return a[0];}}'
assert 98 'char *a; int main(){a = "abc"; return a[1];}'
assert 97 'int main(){return "abc"[0];}'
assert 98 'int main(){return "abc"[1];}'

echo OK
