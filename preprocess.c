#include "stdio.h"

#include "kcc.h"

struct macro {
  struct macro *next;
  char *macro;

  struct token *expand;
};

void macro_definitions(struct token *tk) {
  while (!at_eof(tk)) {
    if (!equal(tk, "#")) {
      tk = tk->next;
      continue;
    }
    // tk = tk->next;
    if (consume(&tk, tk->next, "define")) {

      if (tk->kind != TK_IDENT) {
        error_at(tk->loc, "macro name must be an identifier");
      }

      char *name = tk->str;

      debug("macro name %s", name);

      struct token *macro = tk->next;

      while (!tk->at_bol) {
        debug("%s", tk->str);
        tk = tk->next;
      }

      // print_tok_pos(macro);
      continue;
    }
    tk = tk->next;
  }
}

struct token *preprocess(struct token *tk) {
  macro_definitions(tk);

  return tk;
}
