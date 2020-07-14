#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "kcc.h"

struct predefined {
  char *ident;
  char *replace;
};

struct macro_arg {
  struct macro_arg *next;
  char *name;
  struct token *token;
};

struct macro_param {
  struct macro_param *next;
  char *name;
  struct token *token;
  bool is_variadic;
};

struct macro {
  struct macro *next;
  char *name;
  struct token *expand;
  struct macro_param *params;

  struct token *(*func)(struct token *);
  bool is_objlike;
  bool is_delete;
};

struct condition {
  struct condition *next;

  struct token *start;
  int is_include;
  int is_else;
};

static struct macro *macros;
static struct condition *conds;

struct token *preprocess2(struct token *tk);

void print_macro(struct macro *m) {
  if (!m) {
    fprintf(stderr, "This macro is NULL\n");
    return;
  }
  if (m->is_delete) {
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

bool hideset_contains(struct hideset *hs, char *name, int len) {
  for (; hs; hs = hs->next)
    if (strlen(hs->name) == len && strncmp(hs->name, name, len) == 0)
      return true;

  return false;
}

struct hideset *new_hideset(char *name) {
  struct hideset *hs = calloc(1, sizeof(struct hideset));
  hs->name = name;
  return hs;
}

struct hideset *join_hideset(struct hideset *lhs, struct hideset *rhs) {

  struct hideset head = {};
  struct hideset *cur = &head;
  for (; lhs; lhs = lhs->next) {
    cur = cur->next = new_hideset(lhs->name);
  }
  cur->next = rhs;

  return head.next;
}

struct hideset *hideset_sames(struct hideset *hs1, struct hideset *hs2) {
  struct hideset head = {};
  struct hideset *cur = &head;
  for (; hs1; hs1 = hs1->next) {
    if (hideset_contains(hs2, hs1->name, strlen(hs1->name)))
      cur = cur->next = new_hideset(hs1->name);
  }

  return head.next;
}

struct token *add_hideset(struct token *tk, struct hideset *hs) {
  struct token head = {};
  struct token *cur = &head;
  for (; tk; tk = tk->next) {
    struct token *t = copy_token(tk);
    t->hideset = join_hideset(t->hideset, hs);
    cur = cur->next = t;
  }
  return head.next;
}

bool is_hash(struct token *tk) {
  return tk->at_bol && equal(tk, "#");
}

struct token *skip_line(struct token *tk) {
  if (tk->at_bol) {
    return tk;
  }
  warn_tok(tk, "extra token");
  while (tk->at_bol) {
    tk = tk->next;
  }
  return tk;
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
struct token *append_tokens(struct token *first, struct token *second) {
  if (!first || first->kind == TK_EOF) {
    return second;
  }

  struct token head = {};
  struct token *cur = &head;

  for (; first && first->kind != TK_EOF; first = first->next) {
    cur = cur->next = copy_token(first);
  }

  cur->next = second;

  return head.next;
}

bool exist_file(char *path) {
  struct stat st;
  return !stat(path, &st);
}

char *join_path(char *dir, char *file) {
  int size = strlen(dir) + strlen(file) + 2;
  char *buf = malloc(size);
  snprintf(buf, size, "%s/%s", dir, file);
  return buf;
}

char *join_tokens(struct token *start, struct token *end) {

  int len = 1;
  for (struct token *tk = start; tk != end; tk = tk->next) {
    if (tk != start && tk->has_space) {
      len++;
    }
    len += tk->len;
  }

  char *buf = malloc(len);

  int pos = 0;
  for (struct token *tk = start; tk != end; tk = tk->next) {
    if (tk != start && tk->has_space) {
      buf[pos++] = ' ';
    }

    strncpy(buf + pos, tk->loc, tk->len);
    pos += tk->len;
  }
  buf[pos++] = '\0';

  return buf;
}

char *search_include_paths(char *filename, struct token *tk) {
  for (char **p = include_paths; *p; p++) {
    char *path = join_path(*p, filename);
    if (exist_file(path))
      return path;
  }

  error_tok(tk, "'%s' file not found", filename);
  return NULL;
}

char *read_include_path(struct token **ret, struct token *tk) {
  // #include "..."
  if (tk->kind == TK_STR) {
    struct token *start = tk;
    char *filename = strndup(tk->str + 1, tk->len - 2);

    *ret = skip_line(tk->next);
    if (exist_file(filename)) {
      return filename;
    }

    return search_include_paths(filename, start);
  }

  // #include <...>
  if (equal(tk, "<")) {

    struct token *start = tk;

    for (; !equal(tk, ">"); tk = tk->next) {
      if (tk->at_bol) {
        error_tok(start, "expected '>'");
      }
    }
    char *filename = join_tokens(start->next, tk);
    *ret = skip_line(tk->next);
    return search_include_paths(filename, start);
  }

  // #define HOGE_H <...> | "..."
  // #include HOGE_H
  if (tk->kind == TK_IDENT) {
    struct token *tk2 = preprocess2(tk);
    return read_include_path(ret, tk2);
  }

  error_tok(tk, "expected a \"filename\" or <filename>");
  return NULL;
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
      if (p->is_variadic) {
        if (strncmp("__VA_ARGS__", tk->str, tk->len) == 0)
          match = true;
      }
    }
    // if (!match) {
    //   error_tok(p->token, "Unused parameters");
    // }
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

struct macro *find_macro(struct token *tk) {

  if (tk->kind != TK_IDENT) {
    return NULL;
  }

  // debug("macros:");

  // for (struct macro *m = macros; m; m = m->next) {
  //   debug("  %s", m->name);
  // }
  for (struct macro *m = macros; m; m = m->next) {
    if (find_cond(m->name, tk) && !m->is_delete)
      return m;
  }

  return NULL;
}
void undef_macro(struct token **ret, struct token *tk) {
  if (tk->kind != TK_IDENT) {
    error_tok(tk, "macro name must be an identifier");
  }
  struct macro *m = find_macro(tk);
  if (m) {
    m->is_delete = true;
  }
  *ret = skip_line(tk->next);
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
    if (depth == 0 && equal(tk, ",") && !is_variadic)
      break;

    if (depth == 0 && equal(tk, ")"))
      break;

    if (tk->kind == TK_EOF) {
      error_tok(tk, "Invalid");
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

struct macro_arg *macro_args(struct token **ret,
                             struct token *tk,
                             struct macro_param *param) {
  struct macro_arg head = {};
  struct macro_arg *cur = &head;
  struct macro_param *p = param;

  for (; p; p = p->next) {
    if (cur != &head) {
      skip(&tk, tk, ",");
    }

    cur = cur->next = arg(&tk, tk, p->is_variadic);
    cur->name = p->name;
  }

  if (equal(tk, ",")) {
    error_tok(tk, "Too many arguments");
  }

  skip(&tk, tk, ")");

  *ret = tk;
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
    p->token = copy_token(tk);
    cur = cur->next = p;

    if (tk->kind == TK_IDENT && equal(tk->next, "...")) {
      p->is_variadic = true;
      tk = tk->next->next;
      if (!equal(tk, ")")) {
        error_tok(tk, "Missing \')\' in macro parameter list");
      }
      break;
    }

    if (consume(&tk, tk, "...")) {
      p->is_variadic = true;
      p->name = "__VA_ARGS__";
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
    bool is_variadic = false;
    struct macro_param *params = macro_params(&tok, tok->next);
    m = add_macro(name, false, copy_line(ret, tok));
    m->params = params;

    validate_params(m);

  } else {
    m = add_macro(name, true, copy_line(ret, tok));
  }

  return;
}
struct token *find_arg(struct token *tk, struct macro_arg *args) {
  for (struct macro_arg *a = args; a; a = a->next) {
    if (strlen(a->name) == tk->len &&
        strncmp(a->name, tk->str, tk->len) == 0) {
      return a->token;
    }
  }
  return NULL;
}

struct token *paste(struct token *lhs, struct token *rhs) {
  int size = lhs->len + rhs->len + 1;
  char *buf = malloc(size);

  snprintf(buf, size, "%.*s%.*s", lhs->len, lhs->str, rhs->len, rhs->str);

  struct token *tk = tokenize(lhs->filename, buf, lhs->file_num);

  if (tk->next->kind != TK_EOF) {
    error_tok(lhs, "pasting formde '%s%s', an invalid preprocessing token");
  }

  return tk;
}

char *add_quotes(char *str) {
  int size = 3; // \" ... \"\0
  for (int i = 0; str[i]; i++) {
    if (str[i] == '\\' || str[i] == '"')
      size++;
    size++;
  }

  char *buf = malloc(size);
  int pos = 0;
  buf[pos++] = '"';

  for (int i = 0; str[i]; i++) {
    if (str[i] == '\\' || str[i] == '"')
      buf[pos++] = '\\';

    buf[pos++] = str[i];
  }

  buf[pos++] = '"';
  buf[pos++] = '\0';

  debug("%s", buf);
  return buf;
}

struct token *new_string_token(char *str, struct token *tk) {
  char *new_str = add_quotes(str);

  return tokenize(tk->filename, new_str, tk->file_num);
}

struct token *stringize(struct token *arg, struct token *tk) {
  char *buf = join_tokens(arg, NULL);
  return new_string_token(buf, tk);
}

struct token *replace_token(struct token *tk, struct macro_arg *args) {
  struct token head = {};
  struct token *cur = &head;

  while (tk->kind != TK_EOF) {
    struct token *arg = find_arg(tk, args);

    if (arg) {
      for (struct token *a = arg; a->kind != TK_EOF; a = a->next) {
        cur = cur->next = copy_token(a);
      }

      tk = tk->next;
      continue;
    }

    if (equal(tk, "##")) {
      tk = tk->next;
      struct token *rhs = find_arg(tk, args);

      *cur = *paste(cur, rhs);

      for (struct token *t = rhs->next; t->kind != TK_EOF; t = t->next) {
        cur = cur->next = copy_token(t);
      }
      tk = tk->next;
      continue;
    }

    if (equal(tk, "#")) {
      struct token *arg = find_arg(tk->next, args);

      if (arg) {
        cur = cur->next = stringize(arg, tk);
        tk = tk->next->next;
      }
      continue;
    }

    cur = cur->next = copy_token(tk);
    tk = tk->next;
  }

  return head.next;
}

bool expand_macro(struct token **ret, struct token *tk) {
  if (hideset_contains(tk->hideset, tk->str, tk->len))
    return false;

  struct macro *m = find_macro(tk);
  if (!m) {
    return false;
  }
  if (m->func) {
    *ret = m->func(tk);
    (*ret)->next = tk->next;
    return true;
  }

  // #define macro num | string
  if (m->is_objlike) {
    struct hideset *hs = join_hideset(tk->hideset, new_hideset(m->name));
    struct token *expand = add_hideset(m->expand, hs);
    *ret = append_tokens(expand, tk->next);

    return true;
  }

  // #define macro(var) expression
  if (!equal(tk->next, "("))
    return false;

  struct token *macro_tk = tk;
  struct macro_arg *args = macro_args(&tk, tk->next->next, m->params);

  struct token *tk2 = replace_token(m->expand, args);

  *ret = append_tokens(tk2, tk);

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
  char *buf = calloc(20, sizeof(char));
  snprintf(buf, 20, "%d", val);
  return tokenize(tk->filename, buf, tk->file_num);
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
      if (has_parent)
        skip(&tok, tok, ")");

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

  expr = preprocess2(expr);

  for (struct token *tok = expr; tok->kind != TK_EOF; tok = tok->next) {
    if (tok->kind == TK_IDENT) {
      struct token *next = tok->next;
      *tok = *new_num_token(0, tok);
      tok->next = next;
    }
  }

  struct token *tok = NULL;
  int val = const_expr(&tok, expr);
  if (tok->kind != TK_EOF) {
    error_tok(tok, "Not EOF");
  }

  return val;
}

struct token *preprocess2(struct token *tk) {
  struct token head = {};
  struct token *cur = &head;

  head.next = new_eof();
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

    if (equal(tk, "include")) {
      char *file = read_include_path(&tk, tk->next);
      struct token *tk2 = tokenize_file(file);
      tk = append_tokens(tk2, tk);
      continue;
    }

    if (equal(tk, "define")) {
      define_macro(&tk, tk->next);
      continue;
    }

    if (equal(tk, "undef")) {
      undef_macro(&tk, tk->next);
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
        tk = skip_cond(tk);
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
        tk = skip_cond(tk);
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

  cur->next = tk;
  return head.next;
}

void def_macro(char *def, char *val) {
  struct token *tk = tokenize("(pre-define)", val, 1);
  add_macro(def, true, tk);
}

void builtin_macro(char *def, void *func) {
  struct macro *m = add_macro(def, true, NULL);
  m->func = func;
}

struct token *file_macro(struct token *tk) {
  return new_string_token(tk->filename, tk);
}

struct token *line_macro(struct token *tk) {
  return new_num_token(tk->line_num, tk);
}

void pre_defined_macro() {
  def_macro("__STDC__", "1");
  def_macro("__STDC_HOSTED__", "1");
  def_macro("__STDC_VERSION__", "201112L");
  def_macro("__STDC_NO_ATOMICS__", "1");
  def_macro("__STDC_NO_COMPLEX__", "1");
  def_macro("__STDC_NO_THREADS__", "1");
  def_macro("__STDC_NO_VLA__", "1");

  def_macro("__x86_64__", "1");
  def_macro("__LP64__", "1");
  def_macro("__restrict", "restrict");

  builtin_macro("__FILE__", file_macro);
  builtin_macro("__LINE__", line_macro);
}

struct token *preprocess(struct token *tk) {

  pre_defined_macro();

  struct token *token = preprocess2(tk);

  return token;
}
