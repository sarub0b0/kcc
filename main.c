#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <string.h>

#include "kcc.h"

#define MAX_LEN 256
#define STANDARD_INCLUDE_PATH 5

struct token *tk;
int verbose;
char **include_paths;

char *standard_include_path[STANDARD_INCLUDE_PATH] = {
    "/usr/include",
    "/usr/include/x86_64-linux-gnu",
    "/usr/include/x86_64-linux-gnu/9/include",
    "/usr/local/include",
    "/usr/include/linux",
};

struct config {
  bool is_ast_dump;
  bool is_dump_tokens;
  bool is_list_func;

  // ast-dumpのフィルタ
  char *funcname;

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
  fprintf(stderr, "  --include, -I\n");
  fprintf(stderr, "\n");
}

struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"ast-dump", optional_argument, NULL, 1},
    {"dump-tokens", no_argument, NULL, 2},
    {"list-function", no_argument, NULL, 3},
    {"include", required_argument, NULL, 'I'},
    {0, 0, 0, 0},
};

char short_options[] = "hI:";
void configure(int argc, char **argv, struct config *cfg) {
  if (argc <= 1) {
    fprintf(stderr, "引数の個数が正しくありません。\n");
    usage();
    exit(1);
  }

  include_paths = calloc(argc, sizeof(char *) * STANDARD_INCLUDE_PATH);

  int npahts = 0;
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
      case 'I':
        include_paths[npahts] = calloc(MAX_LEN, sizeof(char));
        snprintf(include_paths[npahts++], MAX_LEN, "%s", optarg);
        break;
      default:
        usage();
        exit(1);
        break;
    }
  }

  if (optind == argc) {
    usage();
    exit(1);
  }

  for (int i = 0; i < STANDARD_INCLUDE_PATH; i++) {
    include_paths[npahts++] = standard_include_path[i];
  }

  cfg->filename = argv[optind];
}

int main(int argc, char *argv[]) {
  struct config cfg = {};
  configure(argc, argv, &cfg);

  tk = tokenize_file(cfg.filename);
  tk = preprocess(tk);

  if (cfg.is_dump_tokens) {
    print_tokens(tk);
    return 0;
  }

  struct program *pr = parse(tk);
  pr->filename = cfg.filename;

  if (cfg.is_ast_dump) {
    print_ast(pr, cfg.funcname);
    return 0;
  }

  if (cfg.is_list_func) {
    print_function(pr);
    return 0;
  }

  if (!cfg.is_ast_dump && !cfg.is_dump_tokens && !cfg.is_list_func)
    gen_code(pr);

  return 0;
}