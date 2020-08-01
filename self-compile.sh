#!/bin/bash

CC=$1
TMP=$2

rm -rf $TMP
mkdir -p $TMP

function kcc () {
    asm=${1%.c}.s
    obj=${1%.c}.o

    echo "./$CC $1 > $TMP/$asm"
    ./$CC $1 > $TMP/$asm

    echo "gcc -static -g -O0 -c -o $TMP/$obj $TMP/$asm"
    gcc -static -g -O0 -c -o $TMP/$obj $TMP/$asm
}

function cc () {
    echo "gcc -static -g -O0 -c -o $TMP/${1%.c}.o $1"
    gcc -static -g -O0 -c -o $TMP/${1%.c}.o $1
}

kcc main.c
kcc message.c
kcc type.c
kcc tokenize.c
kcc preprocess.c
kcc parse.c
kcc codegen.c


echo "gcc -static -o $TMP/kcc $TMP/*.o"
gcc -static -g -O0 -o $TMP/kcc $TMP/*.o
