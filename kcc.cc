#include <cstdio>
#include <cstdlib>

#include "kcc.h"

token *tk;
char *user_input;
std::vector<node *> code;

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

void header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

void prologue() {
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 512\n");
}
int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません。\n");
    return 1;
  }

  locals = new lvar;
  user_input = argv[1];
  tk = tokenize(argv[1]);

  program();

  // print_tree(code[0], nullptr, false);

  header();
  prologue();

  for (auto &&c : code) {
    gen(c);

    printf("  pop rax\n");
  }

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");

  return 0;
}
