#ifndef __KCC_H
#define __KCC_H

#pragma once

#include <list>
#include <string>
#include <vector>

enum token_kind {
  TK_RESERVED,
  TK_IDENT,
  TK_STR,
  TK_NUM,
  TK_EOF,
  TK_KIND_NUM, // Tokenの種類の数
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
  ND_COMMA,
  ND_NUM,
  ND_RETURN,
  ND_IF,
  ND_FOR,
  ND_LABEL,
  ND_BLOCK,
  ND_FUNCALL,
  ND_VAR,
  ND_ADDR,
  ND_DEREF,
  ND_EXPR_STMT,
};

struct var {
  var *next;
  std::string name;

  // local
  int offset;
};

struct node {
  node_kind kind;
  std::string str;

  node *lhs;
  node *rhs;

  // if, for. while
  node *cond;
  node *then;
  node *els;
  node *init;
  node *inc;

  // block statement
  node *body;
  node *next;

  // function call
  node *args;
  // var **args;
  int nargs;

  // variable
  var *var;

  int val;
};

struct function {
  std::string name;
  var *params;

  node *stmt;

  var *locals;
  int stack_size;
};

enum type_kind {
  TY_INT,
};

struct type {
  type_kind kind;
  std::string name;
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
void gen_code(function *);
node *expr();
bool consume(const char *);
token *consume_ident();
void program();

bool equal(token *, std::string const &);
void print_tokens(token *);
void print_ast(std::vector<function *> &);

extern char *user_input;
extern token *tk;
extern std::vector<node *> code;

extern var *locals;
extern std::vector<function *> functions;
extern int verbose;

#endif
