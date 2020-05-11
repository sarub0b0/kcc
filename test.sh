#!/bin/bash

num=1

assert() {
    expected="$1"
    input="$2"

    ./kcc "$input" > tmp.s
    cc -o tmp tmp.s
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

assert 0 'int main(){return 0;}'
assert 42 'int main(){return 42;}'
assert 21 "int main(){return 5+20-4;}"
assert 41 "int main(){return 12 + 34 - 5;}"

assert 47 'int main(){return 5+6*7;}'
assert 15 'int main(){return 5*(9-6);}'
assert 4 'int main(){return (3 + 5) / 2;}'
assert 3 'int main(){return +3;}'
assert 8 'int main(){return -(-3-5);}'
assert 15 'int main(){return -3*(-5);}'
assert 1 'int main(){return 1 == 1;}'
assert 0 'int main(){return 1 != 1;}'
assert 1 'int main(){return 2 >= 1;}'
assert 1 'int main(){return 1 <= 2;}'
assert 1 'int main(){return 2 > 1;}'
assert 1 'int main(){return 1 < 2;}'

assert 5 'int main(){int a; a=3;return a+2;}'
assert 2 'int main(){int a; int b; a=b=2; return a;}'
assert 15 'int main(){int foo; foo=3;return foo+12;}'
# assert 15 'main(){int a=10; return (a+5);}'
# assert 1 'main(){if (1==1) return 1;}'
# assert 2 'main(){if (1!=1) return 1; else return 2;}'
# assert 0 'main(){i=0;while(i<3) return 0; return 1;}'
# assert 3 'main(){i=0;while(i<3) i=i+1; return i;}'
# assert 0 'main(){i=0;while(i<3) return 0; return 1;}'
# assert 3 'main(){for(i = 0; i<3; i=i+1) i = i; return i;}'
# assert 0 'main(){for(;;) return 0;}'
# assert 3 'main(){{i=1; i= i+1; i=i+1; return i;}}'
# assert 6 'main(){for(i = 0; i<5; i=i+1){i = i +1; i= i+1;} return i;}'
# assert 10 'main(){i=0; if(i==0) { i=5; return 10; }}'

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
assert 10 'int foo(int x) {return x;} int main() { int a; a= foo(5) + 5; return a;}'
assert 1 'int main(){return sub(5, 2) -sub(5, 3);} int sub(int x,int y){return x -y;}'
assert 55 'int fib(int n){if(n<=1) return 1; return fib(n-1) + fib(n-2);} int main(){return fib(9);}'



echo OK
