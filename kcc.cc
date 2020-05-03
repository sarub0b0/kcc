#include <cctype>
#include <cstdio>
#include <fstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>

struct token;

token *tk;
char *user_input;

typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
} token_kind;

struct token {
  token_kind kind;
  token *next;
  int val;
  char *str;
};

void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, const char *fmt, ...) {
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

bool consume(char op) {
  if (tk->kind != TK_RESERVED || tk->str[0] != op) {
    return false;
  }
  tk = tk->next;
  return true;
}

void expect(char op) {
  if (tk->kind != TK_RESERVED || tk->str[0] != op) {
    error_at(tk->str, "'%c'ではありません", op);
  }
  tk = tk->next;
}

int expect_number() {
  if (tk->kind != TK_NUM) {
    error_at(tk->str, "数ではありません");
  }
  int val = tk->val;
  tk = tk->next;
  return val;
}

bool at_eof() { return tk->kind == TK_EOF; }

token *new_token(token_kind kind, token *cur, char *str) {
  token *t = new token[1];
  t->kind = kind;
  t->str = str;
  cur->next = t;
  return t;
}

token *tokenize(char *p) {
  token head;
  head.next = nullptr;
  token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = std::strtol(p, &p, 10);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません。\n");
    return 1;
  }

  user_input = argv[1];
  tk = tokenize(argv[1]);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  printf("  mov rax, %d\n", expect_number());

  while (!at_eof()) {
    if (consume('+')) {
      printf("  add rax, %d\n", expect_number());
      continue;
    }

    expect('-');
    printf("  sub rax, %d\n", expect_number());
  }

  printf("  ret\n");

  return 0;
}
