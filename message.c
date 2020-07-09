
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "kcc.h"

char *current_userinput;
char *current_filename;

#define red(x)                   \
  do {                           \
    fprintf(stderr, "\x1b[31m"); \
    x;                           \
    fprintf(stderr, "\x1b[0m");  \
  } while (0);

#define yellow(x)                \
  do {                           \
    fprintf(stderr, "\x1b[33m"); \
    x;                           \
    fprintf(stderr, "\x1b[0m");  \
  } while (0);

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void verror_at(char *loc,
               char *input,
               char *filename,
               int line_num,
               char *fmt,
               va_list ap) {
  char *line = loc;
  while (input < line && line[-1] != '\n') line--;

  char *end = loc;
  while (*end != '\n') end++;

  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int) (end - line), line);

  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

void error_at(char *loc, char *fmt, ...) {
  int line_num = 1;
  for (char *p = current_userinput; p < loc; p++)
    if (*p == '\n') line_num++;

  va_list ap;
  va_start(ap, fmt);

  verror_at(loc, current_userinput, current_filename, line_num, fmt, ap);

  exit(1);
}

void error_tok(struct token *tk, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  red(verror_at(tk->loc, tk->input, tk->filename, tk->line_num, fmt, ap));

  exit(1);
}
void warn_tok(struct token *tk, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  yellow(verror_at(tk->loc, tk->input, tk->filename, tk->line_num, fmt, ap));
}

void info_tok(struct token *tk, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tk->loc, tk->input, tk->filename, tk->line_num, fmt, ap);
}
