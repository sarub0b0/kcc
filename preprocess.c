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
  struct token *token;
};

struct macro {
  struct macro *next;
  char *name;
  struct token *expand;
  struct macro_param *params;
  bool is_objlike;
  bool is_variadic;
};

static struct macro *macros;

void print_macro(struct macro *m) {
  if (!m) {
    fprintf(stderr, "This macro is NULL\n");
    return;
  }

  if (m->is_objlike) {
    fprintf(stderr, "macro: %s ", m->name);
    if (m->expand->str) {
      fprintf(stderr, "%s", m->expand->str);
    }
    fprintf(stderr, "\n");
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
      fprintf(stderr, "%s", t->str);
    }
    fprintf(stderr, "\n");
  }
}

void print_args(struct macro_arg *args) {
  if (!args) {
    fprintf(stderr, "Args of macro are NULL\n");
  }

  fprintf(stderr, "args: ");
  for (struct macro_arg *a = args; a; a = a->next) {
    if (a != args) {
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "  %s => ", a->name);
    for (struct token *tk = a->token; tk->kind != TK_EOF; tk = tk->next) {
      fprintf(stderr, "%s", tk->str);
    }
  }
  fprintf(stderr, "\n");
}

void validate_params(struct macro *m) {

  struct macro_param *p = m->params;
  struct token *expand = m->expand;

  for (struct macro_param *p = m->params; p; p = p->next) {
    bool match = false;
    for (struct token *tk = expand; tk->kind != TK_EOF; tk = tk->next) {
      if (strlen(p->name) == tk->len && strncmp(p->name, tk->str, tk->len) == 0)
        match = true;
      if (m->is_variadic) {
        if (strncmp("__VA_ARGS__", tk->str, tk->len) == 0)
          match = true;
      }
    }
    if (!match) {
      error_at(p->token->loc, "Unused parameters");
    }
  }
}

struct token *end_token(struct token *tok) {
  if (!tok)
    return NULL;

  struct token *end = tok;
  for (struct token *tk = tok; tk->kind != TK_EOF; tk = tk->next) {
    end = tk;
  }

  return end;
}

struct macro *add_macro(char *name, bool is_objlike, struct token *tk) {
  struct macro *m = calloc(1, sizeof(struct macro));
  m->next = macros;
  m->name = name;
  m->is_objlike = is_objlike;
  m->is_variadic = false;
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

struct token *new_comma() {
  struct token *tk = calloc(1, sizeof(struct token));
  tk->kind = TK_RESERVED;
  tk->str = ",";
  tk->len = 1;
  return tk;
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

struct macro_arg *arg(struct token **ret, struct token *tk, bool is_variadic) {
  struct token head = {};
  struct token *cur = &head;

  int depth = 0;
  while (true) {
    if (depth == 0 && equal(tk, ",") && !is_variadic)
      break;

    if (depth == 0 && equal(tk, ")"))
      break;

    if (tk->kind == TK_EOF) {
      error_at(tk->loc, "Invalid");
    }

    if (equal(tk, "("))
      depth++;

    if (equal(tk, ")"))
      depth--;

    cur = cur->next = copy_token(tk);
    tk = tk->next;
  }

  cur->next = new_eof();

  struct macro_arg *arg = calloc(1, sizeof(struct macro_arg));
  arg->token = head.next;

  *ret = tk;
  return arg;
}

struct macro_arg *macro_args(struct token **ret, struct token *tk,
                             struct macro_param *param, bool is_variadic) {
  struct macro_arg head = {};
  struct macro_arg *cur = &head;
  struct macro_param *p = param;

  for (; p; p = p->next) {
    if (cur != &head) {
      skip(&tk, tk, ",");
    }

    cur = cur->next = arg(&tk, tk, is_variadic);
    cur->name = p->name;
  }

  if (equal(tk, ",")) {
    error_at(tk->loc, "Too many arguments");
  }

  skip(&tk, tk, ")");

  *ret = tk;
  return head.next;
}

struct macro_param *macro_params(struct token **ret, struct token *tk,
                                 bool *is_variadic) {

  struct macro_param head = {};
  struct macro_param *cur = &head;

  while (!equal(tk, ")")) {
    if (cur != &head) {
      skip(&tk, tk, ",");
    }

    struct macro_param *p = calloc(1, sizeof(struct macro_param));

    p->name = tk->str;
    p->token = copy_token(tk);

    cur = cur->next = p;

    if (equal(tk, "...")) {
      *is_variadic = true;
      tk = tk->next;
      if (!equal(tk, ")")) {
        error_at(tk->loc, "Missing \')\' in macro parameter list");
      }

      break;
    }

    tk = tk->next;
  }

  *ret = tk->next;
  return head.next;
}

struct token *join_args(struct macro_arg *args) {
  struct token head = {};
  struct token *cur = &head;

  for (struct macro_arg *a = args; a; a = a->next) {
    for (struct token *tk = a->token; tk->kind != TK_EOF; tk = tk->next) {
      cur = cur->next = tk;
    }
    cur->next = new_comma();
  }

  cur->next = new_eof();
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

  struct macro_param *params = m->params;
  struct macro_arg *args = macro_args(&tk, tk, params, m->is_variadic);

  struct token head = {};
  struct token *cur = &head;
  for (struct token *t = m->expand; t->kind != TK_EOF; t = t->next) {
    cur = cur->next = copy_token(t);
  }
  cur->next = new_eof();

  for (struct macro_arg *a = args; a; a = a->next) {
    bool found = false;
    for (struct token *t = head.next; t->kind != TK_EOF; t = t->next) {
      if (equal(t, "__VA_ARGS__")) {
        struct token *next = t->next;
        struct token *end;

        *t = *join_args(args);

        t = end = end_token(t);
        end->next = next;
        break;
      }
      if (strlen(a->name) == t->len && strncmp(a->name, t->str, t->len) == 0) {
        struct token *next = t->next;
        struct token *end;

        *t = *a->token;
        t = end = end_token(t);
        end->next = next;

        found = true;
      }
    }
    if (!found && !m->is_variadic) {
      error_at(a->token->loc, "Too many parameter");
    }
  }

  *ret = head.next;
  end_token(head.next)->next = tk;

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
        bool is_variadic;
        struct macro_param *params = macro_params(&tk, tk->next, &is_variadic);
        m = add_macro(name, false, copy_line(&tk, tk));
        m->params = params;
        m->is_variadic = is_variadic;

        validate_params(m);

      } else {
        m = add_macro(name, true, copy_line(&tk, tk));
      }

      continue;
    }
  }

  return head.next;
}
