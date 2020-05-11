#include <cstdio>
#include <cstdlib>

#include "kcc.h"

token *tk;
char *user_input;
std::vector<node *> code;
int verbose;

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

int main(int argc, char *argv[]) {
  verbose = 0;

  if (argc <= 1) {
    fprintf(stderr, "引数の個数が正しくありません。\n");
    return 1;
  }

  locals = new var;
  user_input = argv[1];
  tk = tokenize(argv[1]);

  program();

  gen_code(nullptr);

  return 0;
}
