#include <cstdio>
#include <cstring>

#include <string>

#include "kcc.h"

lvar *locals;

node *assign();
node *stmt();
node *expr();
node *equality();
node *relational();
node *add();
node *mul();
node *unary();
node *primary();

void expect(const char *op) {
  if (tk->kind != TK_RESERVED || tk->str.size() != std::strlen(op) ||
      tk->str.compare(op)) {
    error_at(tk->pos, "'%s'ではありません", op);
  }
  tk = tk->next;
}

int expect_number() {
  if (tk->kind != TK_NUM) {
    error_at(tk->pos, "数ではありません");
  }
  int val = tk->val;
  tk = tk->next;
  return val;
}

void print_lvar(lvar *lv) {
  printf("lvar name(%s)\n", lv->name.c_str());
  printf("lvar offset(%d)\n", lv->offset);
  printf("lvar next(0x%p)\n", lv->next);
}

lvar *find_lvar(token *tok) {
  for (lvar *var = locals; var; var = var->next) {
    if (var->name.size() == tok->str.size() &&
        var->name.compare(tok->str) == 0) {
      return var;
    }
  }
  return nullptr;
}

bool at_eof() { return tk->kind == TK_EOF; }

node *new_node(node_kind kind, node *lhs, node *rhs) {
  node *n = new node;
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
  case ND_ASSIGN:
    n->str = "=";
    break;
  case ND_NUM:
  case ND_LVAR:
    break;
  }
  return n;
}

node *new_node_num(int val) {
  node *n = new node;
  n->kind = ND_NUM;
  n->val = val;
  n->str = std::to_string(val);
  return n;
}

node *assign() {
  node *n = equality();
  if (consume("=")) {
    n = new_node(ND_ASSIGN, n, assign());
  }
  return n;
}

node *expr() { return assign(); }

node *stmt() {
  node *n;

  if (tk->kind == TK_RETURN) {
    tk = tk->next;
    n = new node;
    n->kind = ND_RETURN;
    n->lhs = expr();
  } else {
    n = expr();
  }

  expect(";");
  return n;
}

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
    expect(")");
    n->str = "(" + n->str + ")";
    return n;
  }

  token *tok = consume_ident();
  if (tok) {
    node *n = new node;
    n->kind = ND_LVAR;

    lvar *lvar = find_lvar(tok);
    if (lvar) {
      n->offset = lvar->offset;
    } else {
      lvar = new struct lvar;
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->offset = locals->offset + 8;
      n->offset = lvar->offset;
      locals = lvar;
    }

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

void program() {
  int i = 0;
  while (!at_eof()) {
    code.push_back(stmt());
  }
}
