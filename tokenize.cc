#include <cctype>
#include <cstdlib>
#include <cstring>

#include <string>

#include "kcc.h"

int starts_with(const char *p, const char *q) {
  return std::strncmp(p, q, std::strlen(q)) == 0;
}

bool consume(const char *op) {
  if (tk->kind != TK_RESERVED || tk->str.size() != std::strlen(op) ||
      tk->str.compare(op)) {
    return false;
  }
  tk = tk->next;
  return true;
}

token *new_token(token_kind kind, token *cur, char *str, int len) {
  token *t = new token[1];
  t->kind = kind;
  t->str = std::string(str, len);
  cur->next = t;
  return t;
}

token *tokenize(char *p) {
  token head;
  head.next = nullptr;
  token *cur = &head;

  while (*p) {
    if (std::isspace(*p)) {
      p++;
      continue;
    }

    if (starts_with(p, "==") || starts_with(p, "!=") || starts_with(p, ">=") ||
        starts_with(p, "<=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (std::ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isdigit(*p)) {
      char *prev = p;
      cur = new_token(TK_NUM, cur, p, 1);
      cur->val = std::strtol(p, &p, 10);
      cur->str = std::string(prev, p - prev);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
