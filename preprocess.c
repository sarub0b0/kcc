#include <stdarg.h>
#include <stdio.h>
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

struct condition {
  struct condition *next;

  struct token *start;
  int is_include;
  int is_else;
};

static struct macro *macros;
static struct condition *conds;

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

bool is_hash(struct token *tk) {
  return tk->at_bol && equal(tk, "#");
}

struct token *skip_line(struct token *tk) {
  if (tk->at_bol) {
    return tk;
  }
  warn_tok(tk, "extra token");
  while (!tk->at_bol) {
    tk = tk->next;
  }
  return tk;
}

struct token *skip_cond2(struct token *tk) {
  while (tk->kind != TK_EOF) {
    if (is_hash(tk) && (equal(tk->next, "if") || equal(tk->next, "ifdef") ||
                        equal(tk->next, "ifndef"))) {
      tk = skip_cond2(tk->next->next);
      continue;
    }
    if (is_hash(tk) && equal(tk->next, "endif"))
      return skip_line(tk->next->next);
    tk = tk->next;
  }
  return tk;
}

struct token *skip_cond(struct token *tk) {

  // if, ifdef, ifndef -> elif, else, endifの間を飛ばしたい
  // 間にif, ifdef, ifndefがあった場合、#endifを探して飛ばす必要がある。
  //
  // #if,ifdef,ifndef
  //   #if,ifdef,ifndef
  //   #else,elif
  //   #endif
  // #else,elif
  // #endif
  while (tk->kind != TK_EOF) {
    if (is_hash(tk) && (equal(tk->next, "if") || equal(tk->next, "ifdef") ||
                        equal(tk->next, "ifndef"))) {

      tk = skip_cond2(tk->next->next);
      continue;
    }

    if (is_hash(tk) && (equal(tk->next, "elif") || equal(tk->next, "else") ||
                        equal(tk->next, "endif"))) {
      break;
    }
    tk = tk->next;
  }

  return tk;
}

void validate_params(struct macro *m) {

  struct macro_param *p = m->params;
  struct token *expand = m->expand;

  for (struct macro_param *p = m->params; p; p = p->next) {
    bool match = false;
    for (struct token *tk = expand; tk->kind != TK_EOF; tk = tk->next) {
      if (strlen(p->name) == tk->len &&
          strncmp(p->name, tk->str, tk->len) == 0)
        match = true;
      if (m->is_variadic) {
        if (strncmp("__VA_ARGS__", tk->str, tk->len) == 0) match = true;
      }
    }
    if (!match) {
      error_tok(p->token, "Unused parameters");
    }
  }
}

struct token *end_token(struct token *tok) {
  if (!tok) return NULL;

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

struct macro_arg *arg(struct token **ret,
                      struct token *tk,
                      bool is_variadic) {
  struct token head = {};
  struct token *cur = &head;

  int depth = 0;
  while (true) {
    if (depth == 0 && equal(tk, ",") && !is_variadic) break;

    if (depth == 0 && equal(tk, ")")) break;

    if (tk->kind == TK_EOF) {
      error_tok(tk, "Invalid");
    }

    if (equal(tk, "(")) depth++;

    if (equal(tk, ")")) depth--;

    cur = cur->next = copy_token(tk);
    tk = tk->next;
  }

  cur->next = new_eof();

  struct macro_arg *arg = calloc(1, sizeof(struct macro_arg));
  arg->token = head.next;

  *ret = tk;
  return arg;
}

struct macro_arg *macro_args(struct token **ret,
                             struct token *tk,
                             struct macro_param *param,
                             bool is_variadic) {
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
    error_tok(tk, "Too many arguments");
  }

  skip(&tk, tk, ")");

  *ret = tk;
  return head.next;
}

struct macro_param *macro_params(struct token **ret,
                                 struct token *tk,
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
        error_tok(tk, "Missing \')\' in macro parameter list");
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

void define_macro(struct token **ret, struct token *tk) {

  struct token *tok = tk;
  if (tok->kind != TK_IDENT) {
    error_tok(tok, "macro name must be an identifier");
  }

  char *name = tok->str;
  tok = tok->next;

  struct macro *m = NULL;
  if (!tok->has_space && equal(tok, "(")) {
    bool is_variadic;
    struct macro_param *params = macro_params(&tok, tok->next, &is_variadic);
    m = add_macro(name, false, copy_line(ret, tok));
    m->params = params;
    m->is_variadic = is_variadic;

    validate_params(m);

  } else {
    m = add_macro(name, true, copy_line(ret, tok));
  }

  return;
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
  if (!consume(&tk, tk->next, "(")) return false;

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
      if (strlen(a->name) == t->len &&
          strncmp(a->name, t->str, t->len) == 0) {
        struct token *next = t->next;
        struct token *end;

        *t = *a->token;
        t = end = end_token(t);
        end->next = next;

        found = true;
      }
    }
    if (!found && !m->is_variadic) {
      error_tok(a->token, "Too many parameter");
    }
  }

  *ret = head.next;
  end_token(head.next)->next = tk;

  return true;
}

struct condition *push_cond(struct token *start, int is_include) {
  struct condition *c = calloc(1, sizeof(struct condition));
  c->next = conds;
  c->start = start;
  c->is_include = is_include;
  c->is_else = false;
  conds = c;
  return c;
}

void pop_cond() {
  conds = conds->next;
}

struct token *new_num_token(int val, struct token *tk) {
  struct token *ret = copy_token(tk);
  char *buf = calloc(10, sizeof(char));
  snprintf(buf, 10, "%d", val);

  ret->val = val;
  ret->str = buf;
  ret->kind = TK_NUM;
  return ret;
}

struct token *read_expression(struct token **ret, struct token *tk) {

  struct token head = {};
  struct token *cur = &head;

  struct token *tok = copy_line(ret, tk);

  while (!at_eof(tok)) {
    if (equal(tok, "defined")) {

      struct token *start = tok;

      bool has_parent = consume(&tok, tok->next, "(");

      if (tok->kind != TK_IDENT)
        error_tok(tok, "macro name must be an identifier");

      struct macro *m = find_macro(tok);

      tok = tok->next;
      if (has_parent) skip(&tok, tok, ")");

      cur = cur->next = new_num_token(m ? 1 : 0, start);
      continue;
    }

    cur = cur->next = tok;
    tok = tok->next;
  }

  cur->next = tok;
  return head.next;
}

long expression(struct token **ret, struct token *tk) {

  struct token *expr = read_expression(ret, tk);

  struct token *tok = NULL;
  int val = const_expr(&tok, expr);
  if (tok->kind != TK_EOF) {
    error_tok(tok, "Not EOF");
  }

  return val;
}

struct token *preprocess(struct token *tk) {

  struct token head = {};
  struct token *cur = &head;

  while (!at_eof(tk)) {

    if (expand_macro(&tk, tk)) {
      continue;
    }

    if (!is_hash(tk)) {
      cur = cur->next = tk;
      tk = tk->next;
      continue;
    }

    struct token *start = tk;
    tk = tk->next;
    if (equal(tk, "define")) {
      define_macro(&tk, tk->next);

      continue;
    }

    // #if expression
    if (equal(tk, "if")) {
      int is_include = expression(&tk, tk->next);
      push_cond(start, is_include);
      if (!is_include) {
        tk = skip_cond(tk);
      }

      continue;
    }

    // #ifdef identifier
    if (equal(tk, "ifdef")) {
      bool is_include = find_macro(tk->next);
      push_cond(start, is_include);
      tk = skip_line(tk->next->next);
      if (!is_include) {
        tk = skip_cond(tk);
      }
      continue;
    }

    // #ifndef identifier
    if (equal(tk, "ifndef")) {
      bool is_include = find_macro(tk->next);
      push_cond(start, !is_include);
      tk = skip_line(tk->next->next);
      if (is_include) {
        tk = skip_cond(tk);
      }
      continue;
    }

    // #elif expression
    if (equal(tk, "elif")) {

      if (!conds || conds->is_else) {
        error_tok(tk, "stray #elif");
      }

      if (!conds->is_include && expression(&tk, tk->next)) {
        conds->is_include = true;
      } else {
        tk = skip_line(tk);
      }
      continue;
    }

    // #else
    if (equal(tk, "else")) {
      if (!conds || conds->is_else) {
        error_tok(tk, "stray #else");
      }
      conds->is_else = true;
      tk = skip_line(tk->next);
      if (conds->is_include) {
        tk = skip_cond(tk->next);
      }
      continue;
    }

    // #endif
    if (equal(tk, "endif")) {
      if (!conds) {
        error_tok(tk, "stray #endif");
      }
      tk = skip_line(tk->next);
      pop_cond();
      continue;
    }

    if (equal(tk, "error")) {
      error_tok(tk, "");
    }

    if (tk->at_bol) {
      continue;
    }

    error_tok(tk, "Invalid preprocessing direvtive");
  }

  return head.next;
}
