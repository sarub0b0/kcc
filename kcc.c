#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "kcc.h"

struct token *tk;
char *user_input;
int verbose;

void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(const char *loc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    verbose = 0;

    if (argc <= 1) {
        fprintf(stderr, "引数の個数が正しくありません。\n");
        return 1;
    }

    locals = NULL;

    user_input = argv[1];
    tk         = tokenize(argv[1]);

    // print_tokens(tk);

    program();

    // print_ast(functions);

    gen_code(functions);

    return 0;
}
