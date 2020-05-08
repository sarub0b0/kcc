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

assert 0 '0;'
assert 42 '42;'
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5; "

assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3 + 5) / 2;'
assert 3 '+3;'
assert 8 '-(-3-5);'
assert 15 '-3*(-5);'
assert 1 '1 == 1;'
assert 0 '1 != 1;'
assert 1 '2 >= 1;'
assert 1 '1 <= 2;'
assert 1 '2 > 1;'
assert 1 '1 < 2;'

assert 2 'a=b=2;'
assert 5 'a=3;b=a+2;'
assert 5 'a=3;a=a+2;'
assert 15 'foo=3;bar=foo+12;'
assert 15 'foo=3;foo=foo+12;'
assert 5 'return 5;'
assert 15 'a=10; return (a+5);'
assert 1 'if (1==1) return 1;'
assert 2 'if (1!=1) return 1; else return 2;'
assert 0 'i=0;while(i<3) return 0; return 1;'
assert 3 'i=0;while(i<3) i=i+1; return i;'
assert 0 'i=0;while(i<3) return 0; return 1;'
assert 3 'for(i = 0; i<3; i=i+1) 0; return i;'
assert 0 'for(;;) return 0;'
assert 3 '{i=1; i= i+1; i=i+1; return i;}'
assert 6 'for(i = 0; i<5; i=i+1){i = i +1; i= i+1;} return i;'
assert 10 'i=0; if(i==0) { i=5; return 10; }'

assert_link 5 'a=0;a = foo(); return a;' foo
assert_link 0 'a=0;bar(); return a;' foo
assert_link 11 'a=3; a= foo2(a, 3+ 5); return a;' foo
assert_link 6 'a= foo3( 1, 2, 3);return a;' foo
assert_link 6 'return foo3( 1, 2, 3);' foo

assert 3 'x = 3; y = &x; return *y;'
assert 3 'x = 3; y = 5; z = &y + 8; return *z;'

echo OK
