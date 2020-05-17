#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "kcc.h"

struct token *tk;
char *user_input;
int verbose;

char *filename;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {

    char *line = loc;
    while (user_input < line && line[-1] != '\n') line--;

    char *end = loc;
    while (*end != '\n') end++;

    int line_num = 1;
    for (char *p = user_input; p < line; p++)
        if (*p == '\n') line_num++;

    int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int) (end - line), line);

    va_list ap;
    va_start(ap, fmt);

    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

char *readfile(char *path) {

    size_t size;
    FILE *f = fopen(path, "r");
    if (!f) {
        error("cacnot open %s: %s", path, strerror(errno));
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        error("%s: fseek: %s", path, strerror(errno));
    }

    size = ftell(f);

    if (fseek(f, 0, SEEK_SET) != 0) {
        error("%s: fseek: %s", path, strerror(errno));
    }

    char *buf = calloc(1, size + 2);

    fread(buf, size, 1, f);

    if (size == 0 || buf[size - 1] != '\n') buf[size++] = '\n';

    buf[size] = '\0';
    fclose(f);

    return buf;
}

int main(int argc, char *argv[]) {
    verbose = 0;

    if (argc <= 1) {
        fprintf(stderr, "引数の個数が正しくありません。\n");
        return 1;
    }

    filename = argv[1];

    user_input = readfile(argv[1]);
    tk         = tokenize(user_input);

    // print_tokens(tk);

    struct program *pr = parse();

    // print_ast(pr);

    gen_code(pr);

    return 0;
}
