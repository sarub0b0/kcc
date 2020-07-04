#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

#include "kcc.h"

struct string *strings;
char *current_userinput;
char *current_filename;

const char *token_kind_str[TK_KIND_NUM] = {
    "reserved",
    "identifier",
    "string",
    "number",
    "eof",
    "sizeof",
};

void print_tokens(struct token *token) {

  for (struct token *t = token; t; t = t->next) {
    fprintf(stderr,
            "%s:%d %*s %s\n",
            t->filename,
            t->line_num,
            12,
            token_kind_str[t->kind],
            t->str);
  }
}

int starts_with(const char *p, const char *q) {
  return strncmp(p, q, strlen(q)) == 0;
}

bool at_eof(struct token *tk) {
  return tk->kind == TK_EOF;
}

bool equal(struct token *token, char *op) {
  return token->len == strlen(op) && strncmp(token->str, op, strlen(op)) == 0;
}

bool consume(struct token **ret, struct token *tk, char *op) {
  if (equal(tk, op)) {
    *ret = tk->next;
    return true;
  }
  *ret = tk;
  return false;
}

struct token *consume_ident(struct token **ret, struct token *tk) {
  struct token *tok = tk;
  if (tk->kind == TK_IDENT) {
    *ret = tk->next;
    return tok;
  }
  *ret = tk;
  return NULL;
}

struct token *consume_num(struct token **ret, struct token *tk) {
  struct token *tok = tk;
  if (tk->kind == TK_IDENT) {
    *ret = tk->next;
    return tok;
  }
  *ret = tk;
  return NULL;
}

struct token *new_token(enum token_kind kind,
                        struct token *cur,
                        char *loc,
                        int len) {
  struct token *t = calloc(1, sizeof(struct token));
  t->kind = kind;
  t->len = len;
  t->str = strndup(loc, len);
  t->loc = loc;
  t->input = current_userinput;
  t->filename = current_filename;
  cur->next = t;
  return t;
}

struct token *string_literal_token(struct token *cur, char *start) {
  char *p = start + 1;

  char *end = p;

  while (*end != '"') {
    if (*end == '\\') {
      end++;
    }
    end++;
  }

  char *buf = malloc(end - p + 1);
  int len = 0;

  while (*p != '"') {
    if (*p == '\\') buf[len++] = *p++;
    buf[len++] = *p++;
  }

  buf[len++] = '\0';

  struct token *ret = new_token(TK_STR, cur, start, p - start + 1);
  ret->str_len = len;
  ret->str_literal = buf;

  return ret;
}

int is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

int is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

bool is_keyword(struct token *tok) {
  char *keyword[] = {
      "return", "if",       "else",   "for",      "while",   "sizeof",
      "int",    "void",     "char",   "short",    "long",    "_Bool",
      "signed", "unsigned", "struct", "enum",     "typedef", "static",
      "extern", "union",    "const",  "restrict",
  };

  for (int i = 0; i < sizeof(keyword) / sizeof(*keyword); i++) {
    if (equal(tok, keyword[i])) {
      return true;
    }
  }

  return false;
}

void convert_ident_to_reserved(struct token *token) {
  for (struct token *tok = token; tok; tok = tok->next) {
    if (tok->kind == TK_IDENT && is_keyword(tok)) {
      tok->kind = TK_RESERVED;
      if (equal(tok, "sizeof")) {
        tok->kind = TK_SIZEOF;
      }
      continue;
    }
  }
}

void add_line_info(struct token *tk) {
  char *p = current_userinput;
  bool at_bol = true;
  bool has_space = false;
  int line_num = 1;

  do {
    if (p == tk->loc) {
      tk->at_bol = at_bol;
      tk->has_space = has_space;
      tk->line_num = line_num;
      tk = tk->next;
    }

    if (*p == '\n') {
      line_num++;
      at_bol = true;
    } else if (isspace(*p)) {
      has_space = true;
    } else {
      has_space = false;
      at_bol = false;
    }
  } while (*p++);
}

struct token *tokenize(char *filename, char *input) {
  strings = NULL;
  current_filename = filename;
  current_userinput = input;

  struct token head = {};
  struct token *cur = &head;
  int string_idx = 0;

  char *p = input;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (starts_with(p, "//")) {
      while (*p != '\n') p++;
      continue;
    }

    if (starts_with(p, "/*")) {
      char *q = strstr(p + 2, "*/");
      if (!q) error_at(p, "コメントが閉じられていません");
      p = q + 2;
      continue;
    }

    if (starts_with(p, "...")) {
      cur = new_token(TK_RESERVED, cur, p, 3);
      p += 3;
    }

    if (starts_with(p, "==") || starts_with(p, "!=") ||
        starts_with(p, ">=") || starts_with(p, "<=") ||
        starts_with(p, "++") || starts_with(p, "--") ||
        starts_with(p, "+=") || starts_with(p, "-=") ||
        starts_with(p, "*=") || starts_with(p, "/=") ||
        starts_with(p, "&&") || starts_with(p, "||") ||
        starts_with(p, "<<") || starts_with(p, ">>") ||
        starts_with(p, "->") || starts_with(p, "##")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (is_alpha(*p)) {
      char *q = p;
      while (is_alnum(*p)) {
        p++;
      }
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    if (*p == '"') {
      cur = string_literal_token(cur, p);
      p += cur->len;
      continue;
    }

    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isdigit(*p)) {
      char *prev = p;

      cur = new_token(TK_NUM, cur, p, 1);

      cur->val = strtol(p, &p, 10);
      cur->len = p - prev;
      cur->str = strndup(prev, cur->len);

      if (*p == 'L' || *p == 'l') {
        p++;
      }

      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  convert_ident_to_reserved(head.next);
  add_line_info(head.next);

  return head.next;
}

char *readfile(char *path) {
  size_t size;
  FILE *f = fopen(path, "r");
  if (!f) {
    error("cannot open %s: %s", path, strerror(errno));
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

void remove_backslash(char *input) {
  char *q = input;
  char *p = input;
  int cnt = 0;
  while (*p) {
    if (starts_with(p, "\\\n")) {
      p += 2;
      cnt++;
    } else if (*p == '\n') {
      *q++ = *p++;
      for (; cnt > 0; cnt--) {
        *q++ = '\n';
      }
    } else {
      *q++ = *p++;
    }
  }

  *q = '\0';
}

struct token *tokenize_file(char *file) {
  char *input = readfile(file);

  remove_backslash(input);

  struct token *token = tokenize(file, input);
  return token;
}
