

CC=kcc
TMP=build

rm -rf $TMP
mkdir -p $TMP

function kcc () {
    asm=${1%.c}.s
    obj=${1%.c}.o
    ./$CC $1 > $TMP/$asm
    gcc -c -o $TMP/$obj $TMP/$asm
}

function cc () {
    asm=${1%.c}.s
    obj=${1%.c}.o
    # ./$CC $1 > $TMP/$asm
    gcc -c -o $TMP/$obj $1
}

cc main.c
cc message.c
cc tokenize.c
cc preprocess.c
cc parse.c
cc codegen.c
kcc type.c


gcc -static -o $TMP/kcc $TMP/*.o

