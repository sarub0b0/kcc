#ifndef __KCC_H
#define __KCC_H

#pragma once

#include <string>

enum token_kind {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
};

struct token {
  token_kind kind;
  token *next;
  int val;
  std::string str;
};

enum node_kind {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NUM,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
  ND_GT,
  ND_GE,
};

struct node {
  node_kind kind;
  node *lhs;
  node *rhs;
  int val;
  std::string str;
};

struct trunk {
  trunk *prev;
  std::string str;
  trunk(trunk *prev, std::string str) {
    this->prev = prev;
    this->str = str;
  }
};

void error(const char *fmt, ...);
void error_at(const char *, const char *, ...);

token *tokenize(char *);
void gen(node *);
node *expr();
bool consume(const char *);

extern char *user_input;
extern token *tk;

#endif
