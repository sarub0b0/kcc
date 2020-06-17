#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct macro_arg {
  struct macro_arg *next;
  char *name;
  struct token *arg;
};

struct macro_param {
  struct macro_param *next;
  char *name;
};

struct macro {
  struct macro *next;
  char *name;
  struct token *expand;
  struct macro_param *params;
  bool is_objlike;
};

static struct macro *macros;

struct macro *add_macro(char *name, bool is_objlike, struct token *tk) {
  struct macro *m = calloc(1, sizeof(struct macro));
  m->next = macros;
  m->name = name;
  m->is_objlike = is_objlike;
  m->expand = tk;
  macros = m;
  return m;
}

struct macro *find_macro(struct token *tk) {

  if (tk->kind != TK_IDENT) {
    return NULL;
  }

  // debug("macros:");

  // for (struct macro *m = macros; m; m = m->next) {
  //   debug("  %s", m->name);
  // }
  for (struct macro *m = macros; m; m = m->next) {
    if (find_cond(m->name, tk)) return m;
  }

  return NULL;
}

struct token *copy_token(struct token *tk) {
  struct token *t = calloc(1, sizeof(struct token));
  *t = *tk;
  t->next = NULL;
  return t;
}

struct token *copy_line(struct token **ret, struct token *tk) {
  struct token head = {};
  struct token *cur = &head;

  while (!tk->at_bol) {
    cur = cur->next = copy_token(tk);
    tk = tk->next;
  }

  *ret = tk;

  cur->next = calloc(1, sizeof(struct token));
  cur->next->kind = TK_EOF;
  cur->next->len = 0;

  return head.next;
}

bool expand_macro(struct token **ret, struct token *tk) {

  struct macro *m = find_macro(tk);
  if (!m) {
    return false;
  }

  // #define macro(var) expression
  if (consume(&tk, tk->next, "(")) {
    *ret = m->expand;
  } else {
    // #define macro num | string
    *ret = m->expand;
    (*ret)->next = tk->next;
  }

  return true;
}

struct token *preprocess(struct token *tk) {

  struct token head = {};
  struct token *cur = &head;

  while (!at_eof(tk)) {

    if (expand_macro(&tk, tk)) {
      continue;
    }

    if (!equal(tk, "#")) {
      cur = cur->next = tk;
      tk = tk->next;
      continue;
    }

    if (consume(&tk, tk->next, "define")) {

      if (tk->kind != TK_IDENT) {
        error_at(tk->loc, "macro name must be an identifier");
      }

      add_macro(tk->str, true, copy_line(&tk, tk->next));

      continue;
    }
  }

  return head.next;
}
