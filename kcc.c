#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
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
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

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

  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';

  buf[size] = '\0';
  fclose(f);

  return buf;
}

struct config {
  bool is_ast_dump;
  bool is_dump_tokens;
  bool is_list_func;

  // ast-dumpのフィルタ
  char *funcname;

  //
  char *filename;
};

void usage(void) {
  fprintf(stderr, "Usage: kcc [options] file\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "General options:\n");
  fprintf(stderr, "  -h, --help\n");
  fprintf(stderr, "  --ast-dump\n");
  fprintf(stderr, "  --dump-tokens\n");
  fprintf(stderr, "  --list-function\n");
  fprintf(stderr, "\n");
}

struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"ast-dump", optional_argument, NULL, 1},
    {"dump-tokens", no_argument, NULL, 2},
    {"list-function", no_argument, NULL, 3},
    {0, 0, 0, 0},
};

char short_options[] = "h";
void configure(int argc, char **argv, struct config *cfg) {
  if (argc <= 1) {
    fprintf(stderr, "引数の個数が正しくありません。\n");
    usage();
    exit(1);
  }

  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) !=
         -1) {
    switch (c) {
    case 1:
      cfg->is_ast_dump = true;
      if (optarg) {
        cfg->funcname = strdup(optarg);
      }
      break;
    case 2:
      cfg->is_dump_tokens = true;
      break;
    case 3:
      cfg->is_list_func = true;
      break;
    case 'h':
      usage();
      exit(1);
    default:
      break;
    }
  }

  if (optind == argc) {
    usage();
    exit(1);
  }

  cfg->filename = argv[optind];
}

int main(int argc, char *argv[]) {
  struct config cfg = {};

  configure(argc, argv, &cfg);

  filename = cfg.filename;

  user_input = readfile(cfg.filename);
  tk = tokenize(cfg.filename, user_input);
  tk = preprocess(tk);

  if (cfg.is_dump_tokens)
    print_tokens(tk);

  struct program *pr = parse(tk);

  if (cfg.is_ast_dump)
    print_ast(pr, cfg.funcname);

  if (cfg.is_list_func)
    print_function(pr);

  if (!cfg.is_ast_dump && !cfg.is_dump_tokens && !cfg.is_list_func)
    gen_code(pr);

  return 0;
}
