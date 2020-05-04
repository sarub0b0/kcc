#include <cstdio>

#include <string>

#include "kcc.h"

node *expr();
node *equality();
node *relational();
node *add();
node *mul();
node *unary();
node *primary();

void expect(char op) {
  if (tk->kind != TK_RESERVED || tk->str[0] != op) {
    error_at(tk->str.c_str(), "'%c'ではありません", op);
  }
  tk = tk->next;
}

int expect_number() {
  if (tk->kind != TK_NUM) {
    error_at(tk->str.c_str(), "数ではありません");
  }
  int val = tk->val;
  tk = tk->next;
  return val;
}

bool at_eof() { return tk->kind == TK_EOF; }

node *new_node(node_kind kind, node *lhs, node *rhs) {
  node *n = new node[1];
  n->kind = kind;
  n->lhs = lhs;
  n->rhs = rhs;
  switch (kind) {
  case ND_ADD:
    n->str = "+";
    break;
  case ND_SUB:
    n->str = "-";
    break;
  case ND_MUL:
    n->str = "*";
    break;
  case ND_DIV:
    n->str = "/";
    break;
  case ND_EQ:
    n->str = "==";
    break;
  case ND_NE:
    n->str = "!=";
    break;
  case ND_LE:
    n->str = "<=";
    break;
  case ND_LT:
    n->str = "<";
    break;
  case ND_GE:
    n->str = ">=";
    break;
  case ND_GT:
    n->str = ">";
    break;
  case ND_NUM:
    break;
  }
  return n;
}

node *new_node_num(int val) {
  node *n = new node[1];
  n->kind = ND_NUM;
  n->val = val;
  n->str = std::to_string(val);
  return n;
}

node *expr() { return equality(); }

node *equality() {
  node *n = relational();
  while (true) {
    if (consume("==")) {
      n = new_node(ND_EQ, n, relational());
    } else if (consume("!=")) {
      n = new_node(ND_NE, n, relational());
    } else {
      return n;
    }
  }
}

node *relational() {
  node *n = add();

  while (true) {
    if (consume("<")) {
      n = new_node(ND_LT, n, add());
    } else if (consume("<=")) {
      n = new_node(ND_LE, n, add());
    } else if (consume(">")) {
      n = new_node(ND_GT, n, add());
    } else if (consume(">=")) {
      n = new_node(ND_GE, n, add());
    } else {
      return n;
    }
  }
}

node *add() {
  node *n = mul();
  while (true) {
    if (consume("+")) {
      n = new_node(ND_ADD, n, mul());
    } else if (consume("-")) {
      n = new_node(ND_SUB, n, mul());
    } else {
      return n;
    }
  }
}

node *mul() {
  node *n = unary();
  while (true) {
    if (consume("*")) {
      n = new_node(ND_MUL, n, unary());
    } else if (consume("/")) {
      n = new_node(ND_DIV, n, unary());
    } else {
      return n;
    }
  }
}

node *primary() {
  if (consume("(")) {
    node *n = expr();
    expect(')');
    n->str = "(" + n->str + ")";
    return n;
  }

  return new_node_num(expect_number());
}

node *unary() {
  if (consume("+")) {
    return primary();
  }
  if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }

  return primary();
}

