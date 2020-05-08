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
node *funcall();

void skip(token *token, std::string const &op) {
  if (!equal(token, op)) {
    error("expected '%s', but op is '%s'", op.c_str(), token->str.c_str());
  }

  tk = token->next;
}

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
  case ND_IF:
    n->str = "if";
    break;
  case ND_FOR:
    n->str = "for/while";
    break;
  case ND_RETURN:
    n->str = "return";
    break;
  case ND_FUNC:
    n->str = "func";
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

node *expr() {
  node *n = assign();

  if (equal(tk, ",")) {
    tk = tk->next;
    n = new_node(ND_COMMA, n, expr());
  }

  return n;
}

node *stmt() {
  node *n;

  if (equal(tk, "return")) {
    tk = tk->next;
    n = new_node(ND_RETURN, nullptr, nullptr);

    n->lhs = expr();

    skip(tk, ";");
    return n;
  }

  // "if" "(" expr ")" stmt ("else" stmt)?
  if (equal(tk, "if")) {
    n = new_node(ND_IF, nullptr, nullptr);

    skip(tk->next, "(");
    n->cond = expr();
    skip(tk, ")");

    n->then = stmt();

    if (equal(tk, "else")) {
      tk = tk->next;
      n->els = stmt();
    }

    return n;
  }

  // "while" "(" expr ")" stmt
  if (equal(tk, "while")) {
    n = new_node(ND_FOR, nullptr, nullptr);
    skip(tk->next, "(");
    n->cond = expr();
    skip(tk, ")");
    n->then = stmt();

    return n;
  }

  // "for" "(" expr? ";" expr? ";" expr? ")" stmt
  if (equal(tk, "for")) {
    n = new_node(ND_FOR, nullptr, nullptr);
    skip(tk->next, "(");
    if (!equal(tk, ";")) {
      n->init = expr();
    }
    skip(tk, ";");

    if (!equal(tk, ";")) {
      n->cond = expr();
    }
    skip(tk, ";");

    if (!equal(tk, ")")) {
      n->inc = expr();
    }
    skip(tk, ")");

    n->then = stmt();

    return n;
  }

  if (equal(tk, "{")) {
    tk = tk->next;
    n = new_node(ND_BLOCK, nullptr, nullptr);

    node head = {};
    node *cur = &head;

    while (!equal(tk, "}")) {
      cur = cur->next = stmt();
    }

    n->body = head.next;

    skip(tk, "}");
    return n;
  }

  n = expr();
  expect(";");
  // if (equal(tk, ";")) {
  //   skip(tk, ";");
  // } else {
  // }

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
  node *n;
  if (consume("(")) {
    n = expr();
    expect(")");
    n->str = "(" + n->str + ")";
    return n;
  }

  token *tok = consume_ident();
  if (tok) {
    if (equal(tk, "(")) {
      n = new_node(ND_FUNC, nullptr, nullptr);
      n->str = tok->str;
      tk = tk->next;

      node head = {};
      node *cur = &head;

      if (tk->kind == TK_IDENT || tk->kind == TK_NUM) {
        n->lhs = expr();
      }

      skip(tk, ")");

    } else {

      n = new_node(ND_LVAR, nullptr, nullptr);

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

  if (consume("*")) {
    return new_node(ND_DEREF, unary(), nullptr);
  }
  if (consume("&")) {
    return new_node(ND_ADDR, unary(), nullptr);
  }
  return primary();
}

void program() {
  int i = 0;
  while (!at_eof()) {
    code.push_back(stmt());
  }
}
