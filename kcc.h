#ifndef __KCC_H
#define __KCC_H

#pragma once

#include <string>
#include <vector>

struct lvar {
  lvar *next;
  std::string name;
  int offset;
};

enum token_kind {
  TK_RESERVED,
  TK_IDENT,
  TK_STR,
  TK_NUM,
  TK_EOF,
};

struct token {
  token_kind kind;
  token *next;
  int val;
  std::string str;
  char *pos;
};

enum node_kind {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
  ND_GT,
  ND_GE,
  ND_ASSIGN,
  ND_LVAR,
  ND_COMMA,
  ND_NUM,
  ND_RETURN,
  ND_IF,
  ND_FOR,
  ND_LABEL,
  ND_BLOCK,
  ND_FUNC,
  ND_VAR,
};

struct var {
  var *next;
  std::string name;
  bool is_local;
};

struct node {
  node_kind kind;
  node *lhs;
  node *rhs;

  // if, for
  node *cond;
  node *then;
  node *els;
  node *init;
  node *inc;

  // block statement
  node *body;
  node *next;

  // function call
  std::vector<node *> args;
  int nargs;

  int val;
  std::string str;
  int offset;
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
void gen_code(node *);
node *expr();
bool consume(const char *);
token *consume_ident();
void program();

bool equal(token *, std::string const &);

extern char *user_input;
extern token *tk;
extern std::vector<node *> code;

extern lvar *locals;

#endif
