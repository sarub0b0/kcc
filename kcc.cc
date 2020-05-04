#include <cctype>
#include <cstdio>
#include <cstring>

#include <string>

#include <stdio.h>
#include <stdlib.h>

struct token;
struct node;

token *tk;
char *user_input;

node *expr();
node *equality();
node *relational();
node *add();
node *mul();
node *unary();
node *primary();

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

void showTrunks(trunk *p) {
  if (p == nullptr)
    return;
  showTrunks(p->prev);
  printf("%s", p->str.c_str());
}

void print_tree(node *root, trunk *prev, bool isLeft) {
  if (root == nullptr)
    return;

  std::string prev_str = "     ";
  trunk *tr = new trunk(prev, prev_str);
  print_tree(root->lhs, tr, true);

  if (!prev)
    tr->str = "---";
  else if (isLeft) {
    tr->str = ".---";
    prev_str = "    |";
  } else {
    tr->str = "`---";
    prev->str = prev_str;
  }

  showTrunks(tr);
  printf("%s\n", root->str.c_str());
  if (prev)
    prev->str = prev_str;

  tr->str = "    |";
  print_tree(root->rhs, tr, false);
}

void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(const char *loc, const char *fmt, ...) {
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

bool consume(const char *op) {
  if (tk->kind != TK_RESERVED || tk->str.size() != std::strlen(op) ||
      tk->str.compare(op)) {
    return false;
  }
  tk = tk->next;
  return true;
}

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

token *new_token(token_kind kind, token *cur, char *str, int len) {
  token *t = new token[1];
  t->kind = kind;
  t->str = std::string(str, len);
  cur->next = t;
  return t;
}

int starts_with(const char *p, const char *q) {
  return std::strncmp(p, q, std::strlen(q)) == 0;
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

void gen(node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GE:
    printf("  cmp rax, rdi\n");
    printf("  setge al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GT:
    printf("  cmp rax, rdi\n");
    printf("  setg al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NUM:
    break;
  }

  printf("  push rax\n");
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません。\n");
    return 1;
  }

  user_input = argv[1];
  tk = tokenize(argv[1]);

  node *n = expr();

  // print_tree(n, nullptr, false);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  gen(n);

  printf("  pop rax\n");
  printf("  ret\n");

  return 0;
}
