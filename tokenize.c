#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>

#include "kcc.h"

struct string *strings;
char *current_user_input;
char *current_filename;

const char *token_kind_str[TK_KIND_NUM] = {
    "reserved", "identifier", "string", "number", "eof", "sizeof",
};

void print_tokens(struct token *token) {

  for (struct token *t = token; t->kind != TK_EOF; t = t->next) {
    printf("%*s %s\n", 12, token_kind_str[t->kind], t->str);
  }
}

int starts_with(const char *p, const char *q) {
  return strncmp(p, q, strlen(q)) == 0;
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

struct token *new_token(enum token_kind kind, struct token *cur, char *loc,
                        int len) {
  struct token *t = calloc(1, sizeof(struct token));
  t->kind = kind;
  t->len = len;
  t->str = strndup(loc, len);
  t->loc = loc;
  t->input = current_user_input;
  t->file = current_filename;
  cur->next = t;
  return t;
}

int is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

int is_alnum(char c) { return is_alpha(c) || ('0' <= c && c <= '9'); }

bool is_keyword(struct token *tok) {
  char *keyword[] = {"return", "if",   "else",   "for",  "while",
                     "sizeof", "int",  "void",   "char", "short",
                     "long",   "bool", "struct", "enum"};

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

struct token *tokenize(char *filename, char *p) {
  strings = NULL;
  current_filename = filename;
  current_user_input = p;

  struct token head = {};
  struct token *cur = &head;
  int string_idx = 0;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (starts_with(p, "//")) {
      while (*p != '\n')
        p++;
      continue;
    }

    if (starts_with(p, "/*")) {
      char *q = strstr(p + 2, "*/");
      if (!q)
        error_at(p, "コメントが閉じられていません");
      p = q + 2;
      continue;
    }

    if (starts_with(p, "==") || starts_with(p, "!=") || starts_with(p, ">=") ||
        starts_with(p, "<=") || starts_with(p, "++") || starts_with(p, "--") ||
        starts_with(p, "+=") || starts_with(p, "-=") || starts_with(p, "*=") ||
        starts_with(p, "/=") || starts_with(p, "&&") || starts_with(p, "||") ||
        starts_with(p, "->")) {
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

    // if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
    //   cur = new_token(TK_RESERVED, cur, p++, 1);
    //   continue;
    // }

    if (*p == '"') {
      p++;
      char *q = p;
      while (*p != '"') {
        if (*p == '\\' && *(p + 1) == 0x22) {
          p++;
        }
        p++;
      }
      cur = new_token(TK_STR, cur, q, p - q);

      p++;
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
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  convert_ident_to_reserved(head.next);
  return head.next;
}
