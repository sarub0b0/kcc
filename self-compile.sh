#!/bin/bash

CC=kcc
TMP=$1

rm -rf $TMP
mkdir -p $TMP

function kcc () {
    asm=${1%.c}.s
    obj=${1%.c}.o

    ./$CC $1 > $TMP/$asm

    clang -c -o $TMP/$obj $TMP/$asm
}

function cc () {
    clang -c -o $TMP/${1%.c}.o $1
}

cc main.c
cc message.c
cc tokenize.c
cc preprocess.c
cc parse.c
cc codegen.c
cc type.c

clang -static -o $TMP/kcc $TMP/*.o
