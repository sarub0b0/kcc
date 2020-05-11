#include <cctype>
#include <cstdlib>
#include <cstring>

#include <string>
#include <unordered_set>

#include "kcc.h"

using uset_str = std::unordered_set<std::string>;

const char *token_kind_str[TK_KIND_NUM] = {
    "reserved", "identifier", "string", "number", "eof",
};

void print_tokens(token *token) {

  for (struct token *t = token; t->kind != TK_EOF; t = t->next) {
    printf("%*s %s\n", 12, token_kind_str[t->kind], t->str.c_str());
  }
}

int starts_with(const char *p, const char *q) {
  return std::strncmp(p, q, std::strlen(q)) == 0;
}

bool equal(token *tk, std::string const &op) {
  return tk->str.compare(op) == 0;
}

bool consume(const char *op) {
  if (tk->kind != TK_RESERVED || tk->str.size() != std::strlen(op) ||
      tk->str.compare(op)) {
    return false;
  }
  tk = tk->next;
  return true;
}

token *consume_ident() {
  token *tok = tk;
  if (tk->kind == TK_IDENT) {
    tk = tk->next;
    return tok;
  }
  return nullptr;
}

token *consume_num() {
  token *tok = tk;
  if (tk->kind == TK_IDENT) {
    tk = tk->next;
    return tok;
  }
  return nullptr;
}

token *new_token(token_kind kind, token *cur, char *str, int len) {
  token *t = new token;
  t->kind = kind;
  t->str = std::string(str, len);
  t->pos = str;
  cur->next = t;
  return t;
}

int is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

int is_alnum(char c) { return is_alpha(c) || ('0' <= c && c <= '9'); }

bool is_keyword(std::string &str) {
  std::unordered_set<std::string> keyword{
      "return", "if", "else", "for", "while",
  };

  return keyword.find(str) != keyword.end();
}

void convert_ident_to_reserved(token *token) {
  while (token) {
    if (token->kind == TK_IDENT) {
      if (is_keyword(token->str)) {
        token->kind = TK_RESERVED;
        continue;
      }
    }
  }
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

    // if (starts_with(p, "return") && !is_alnum(p[6])) {
    //   cur = new_token(TK_RESERVED, cur, p, 6);
    //   p += 6;
    //   continue;
    // }

    if (starts_with(p, "==") || starts_with(p, "!=") || starts_with(p, ">=") ||
        starts_with(p, "<=")) {
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

  new_token(TK_EOF, cur, p, 1);
  return head.next;
}