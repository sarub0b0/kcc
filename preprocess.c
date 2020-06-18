#include "stdio.h"
#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct macro_arg {
  struct macro_arg *next;
  char *name;
  struct token *token;
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
  struct token *expand_end;
  bool is_objlike;
};

static struct macro *macros;

void print_macro(struct macro *m) {
  if (!m) {
    fprintf(stderr, "Non macro\n");
    return;
  }

  if (m->is_objlike) {
    fprintf(stderr, "macro: %s %s\n", m->name, m->expand->str);
  } else {
    fprintf(stderr, "macro: %s(", m->name);
    int cnt = 0;
    for (struct macro_param *p = m->params; p; p = p->next) {
      if (cnt++) {
        fprintf(stderr, ", ");
      }
      fprintf(stderr, "%s", p->name);
    }
    fprintf(stderr, ") ");
    for (struct token *t = m->expand; t->kind != TK_EOF; t = t->next) {
      fprintf(stderr, "%s ", t->str);
    }
    fprintf(stderr, "\n");
  }
}

struct token *end_token(struct token *expand) {
  struct token *end = NULL;
  if (!expand)
    return NULL;

  for (struct token *tk = expand; tk->kind != TK_EOF; tk = tk->next) {
    end = tk;
  }

  return end;
}

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
    if (find_cond(m->name, tk))
      return m;
  }

  return NULL;
}

struct token *copy_token(struct token *tk) {
  struct token *t = calloc(1, sizeof(struct token));
  *t = *tk;
  t->next = NULL;
  return t;
}

struct token *new_eof() {
  struct token *eof = calloc(1, sizeof(struct token));
  eof->kind = TK_EOF;
  eof->len = 0;
  return eof;
}

struct token *copy_line(struct token **ret, struct token *tk) {
  struct token head = {};
  struct token *cur = &head;

  while (!tk->at_bol) {
    cur = cur->next = copy_token(tk);
    tk = tk->next;
  }

  *ret = tk;

  cur->next = new_eof();

  return head.next;
}

struct macro_arg *macro_args(struct token **ret, struct token *tk) {
  struct macro_arg head = {};
  struct macro_arg *cur = &head;

  // まずは引数1個の値の場合
  while (!equal(tk, ")")) {
    if (cur != &head) {
      skip(&tk, tk, ",");
    }
    struct macro_arg *arg = calloc(1, sizeof(struct macro_arg));

    cur = cur->next = arg;
    cur->name = tk->str;
    cur->token = tk;

    tk = tk->next;
  }

  *ret = tk->next;
  return head.next;
}

struct macro_param *macro_params(struct token **ret, struct token *tk) {

  struct macro_param head = {};
  struct macro_param *cur = &head;

  while (!equal(tk, ")")) {
    if (cur != &head) {
      skip(&tk, tk, ",");
    }

    struct macro_param *p = calloc(1, sizeof(struct macro_param));

    p->name = tk->str;

    cur = cur->next = p;

    tk = tk->next;
  }

  *ret = tk->next;
  return head.next;
}

bool expand_macro(struct token **ret, struct token *tk) {

  struct macro *m = find_macro(tk);
  if (!m) {
    return false;
  }

  // #define macro num | string
  if (m->is_objlike) {
    *ret = copy_token(m->expand);
    (*ret)->next = tk->next;
    return true;
  }
  // #define macro(var) expression
  if (!consume(&tk, tk->next, "("))
    return false;
  struct macro_arg *args = macro_args(&tk, tk);
  struct macro_param *params = m->params;

  struct token head = {};
  struct token *cur = &head;
  struct token *eof = NULL;
  for (struct token *t = m->expand; t->kind != TK_EOF; t = t->next) {
    cur = cur->next = copy_token(t);
  }
  cur->next = new_eof();
  eof = cur;

  struct macro_arg *arg = args;
  for (struct macro_param *p = params; p; p = p->next) {
    bool found = false;
    for (struct token *t = head.next; t->kind != TK_EOF; t = t->next) {
      if (strlen(p->name) == t->len && strncmp(p->name, t->str, t->len) == 0) {
        t->str = arg->name;
        t->kind = arg->token->kind;
        t->val = arg->token->val;
        found = true;
      }
    }
    if (!found)
      error("Not found macro param %s", p->name);

    arg = arg->next;
  }

  *ret = head.next;
  eof->next = tk;

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

      char *name = tk->str;
      tk = tk->next;

      struct macro *m = NULL;
      if (!tk->has_space && equal(tk, "(")) {
        struct macro_param *params = macro_params(&tk, tk->next);
        m = add_macro(name, false, copy_line(&tk, tk));
        m->params = params;

      } else {
        m = add_macro(name, true, copy_line(&tk, tk));
      }

      continue;
    }
  }

  return head.next;
}
