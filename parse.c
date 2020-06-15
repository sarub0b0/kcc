#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct tag_scope {
  struct tag_scope *next;
  struct type *type;

  int depth;
  char *name;
};

struct var_scope {
  struct var_scope *next;
  struct var *var;
  struct type *enum_ty;

  int depth;
  char *name;
  int enum_val;
};

struct function *functions;
struct function *current_fn;
struct var *locals;
struct var *globals;
struct tag_scope *tags;
struct var_scope *vars;

int scope_depth;

struct function *funcdef(struct token **, struct token *, struct type *);
struct type *declarator(struct token **, struct token *, struct type *);
struct type *struct_declarator(struct token **, struct token *);
struct type *enum_declarator(struct token **, struct token *);
struct type *funcdef_args(struct token **, struct token *, struct type *);
struct type *type_suffix(struct token **, struct token *, struct type *);
struct node *compound_stmt(struct token **, struct token *);
struct node *assign(struct token **, struct token *);
struct node *conditional(struct token **, struct token *);
struct node *logor(struct token **, struct token *);
struct node *logand(struct token **, struct token *);
struct node * bitor (struct token **, struct token *);
struct node *bitxor(struct token **, struct token *);
struct node *bitand(struct token **, struct token *);
struct node *stmt(struct token **, struct token *);
struct node *expr(struct token **, struct token *);
struct node *equality(struct token **, struct token *);
struct node *relational(struct token **, struct token *);
struct node *add(struct token **, struct token *);
struct node *mul(struct token **, struct token *);
struct node *cast(struct token **, struct token *);
struct node *unary(struct token **, struct token *);
struct node *postfix(struct token **, struct token *);
struct node *primary(struct token **, struct token *);

struct node *funcall(struct token **, struct token *, struct token *);

struct node *lvar_initializer(struct token **, struct token *, struct var *,
                              struct type *);
void gvar_initializer(struct token **, struct token *, struct var *,
                      struct type *);

struct function *find_func(char *);

#define MAX_LEN (int)(256)

void print_tok(struct token *tk, char end_str) {

  char *loc = tk->loc;
  char *line = tk->loc;
  char *input = tk->input;
  char *file = tk->file;
  while (input < line && line[-1] != '\n')
    line--;

  char *end = line;
  while (*end != end_str)
    end++;

  end++;

  int line_num = 1;
  for (char *p = input; p < line; p++)
    if (*p == '\n')
      line_num++;

  fprintf(stdout, " %s:%d: ", file, line_num);
  fprintf(stdout, "%.*s\n", (int)(end - line), line);
}

void print_tok_pos(struct token *tk) {

  char *loc = tk->loc;
  char *line = tk->loc;
  char *input = tk->input;
  char *file = tk->file;
  while (input < line && line[-1] != '\n')
    line--;

  char *end = line;
  while (*end != '\n')
    end++;

  end++;

  int line_num = 1;
  for (char *p = input; p < line; p++)
    if (*p == '\n')
      line_num++;

  fprintf(stdout, "%s:%d\n", file, line_num);
}

void print_function(struct program *p) {
  printf("Function list:\n");
  for (struct function *fn = p->functions; fn; fn = fn->next) {
    print_tok(fn->token, ')');
  }
}

void print_param(struct var *params, bool is_next, struct function *fn) {
  for (struct var *v = params; v; v = v->next) {
    if (is_next)
      printf("| ");
    else
      printf("  ");

    if (v->next)
      printf("|-Param %s\n", v->name);
    else {
      if (fn->stmt)
        printf("|-Param %s\n", v->name);
      else
        printf("`-Param %s\n", v->name);
    }
  }
}

void print_globals(bool has_function) {
  for (struct var *v = globals; v; v = v->next) {
    char *prefix = "|-";
    char *type = "Param";

    if (!has_function)
      prefix = "`-";

    printf("%s%s ", prefix, type);
    printf("%s %s", type_to_name(v->type->kind), v->name);
    if (v->data)
      printf(": %d\n", *(int *)v->data);
    else
      printf("\n");
  }
}

void print_struct(bool has_function) {

  struct tag_scope *last = NULL;
  for (struct tag_scope *tag = tags; tag; tag = tag->next)
    last = tag;

  for (struct tag_scope *t = tags; t; t = t->next) {
    char *first_prefix = "| ";
    if (last == t && !has_function) {
      printf("`-Struct %s\n", t->name);
      first_prefix = "  ";
    } else {
      printf("|-Struct %s\n", t->name);
    }
    for (struct member *m = t->type->members; m; m = m->next) {
      char *second_prefix = "|-";
      if (t->next || has_function)
        first_prefix = "| ";
      else
        first_prefix = "  ";

      if (m->next)
        second_prefix = "|-";
      else
        second_prefix = "`-";
      printf("%s%sMember %s %s\n", first_prefix, second_prefix,
             type_to_name(m->type->kind), m->name);
    }
  }
}

void print_stmt(struct node *n, bool is_next_stmt, bool is_next_node,
                char *prefix) {
  if (!n)
    return;

  char *local_prefix;
  char *scope_prefix;
  char buf[MAX_LEN];

  local_prefix = strndup(prefix, MAX_LEN);
  scope_prefix = strndup(prefix, MAX_LEN);

  if (n->next || is_next_node) {
    is_next_stmt = true;
    snprintf(buf, MAX_LEN, "%s |", local_prefix);
  } else {
    is_next_stmt = false;
    snprintf(buf, MAX_LEN, "%s `", local_prefix);
  }

  local_prefix = strndup(buf, MAX_LEN);

  switch (n->kind) {
  case ND_RETURN:
    printf("%s-Return\n", local_prefix);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_BITOR:
  case ND_BITXOR:
  case ND_BITAND:
    printf("%s-Calc '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
    print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_NUM:
    printf("%s-Num '%d'\n", local_prefix, n->val);
    break;
  case ND_VAR:
    printf("%s-Var '%s'\n", local_prefix, n->str);
    break;
  case ND_ADDR:
    printf("%s-Addr '%s'\n", local_prefix, n->str);
    break;
  case ND_DEREF:
    printf("%s-Deref '%s'\n", local_prefix, n->str);
    break;
  case ND_EXPR_STMT:
    printf("%s-ExprStmt\n", local_prefix);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_STMT_EXPR:
    printf("%s-StmtExpr\n", local_prefix);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    for (struct node *stmt = n->body; stmt; stmt = stmt->next) {
      if (stmt->next == NULL) {
        print_stmt(stmt, is_next_stmt, false, scope_prefix);
      } else {
        print_stmt(stmt, is_next_stmt, true, scope_prefix);
      }
    }
    break;
  case ND_ASSIGN:
    printf("%s-Assign '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
    print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_IF:
    printf("%s-If '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->cond, is_next_stmt, true, scope_prefix);
    print_stmt(n->then, is_next_stmt, true, scope_prefix);
    print_stmt(n->els, is_next_stmt, false, scope_prefix);
    break;
  case ND_FOR:
    printf("%s-Loop '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->init, is_next_stmt, true, scope_prefix);
    print_stmt(n->cond, is_next_stmt, true, scope_prefix);
    print_stmt(n->inc, is_next_stmt, true, scope_prefix);
    print_stmt(n->then, is_next_stmt, false, scope_prefix);
    break;
  case ND_EQ:
  case ND_NE:
  case ND_LE:
  case ND_LT:
  case ND_GE:
  case ND_GT:
  case ND_LOGOR:
  case ND_LOGAND:
    printf("%s-Cond '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }

    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
    print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_BLOCK:
    printf("%s-Block '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    for (struct node *stmt = n->body; stmt; stmt = stmt->next) {
      if (stmt->next == NULL) {
        print_stmt(stmt, is_next_stmt, false, scope_prefix);
      } else {
        print_stmt(stmt, is_next_stmt, true, scope_prefix);
      }
    }
    break;
  case ND_FUNCALL:

    printf("%s-Funcall '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    for (struct node *arg = n->body; arg; arg = arg->next) {
      if (arg->next == NULL) {
        print_stmt(arg, is_next_stmt, false, scope_prefix);
      } else {
        print_stmt(arg, is_next_stmt, true, scope_prefix);
      }
    }
    break;
  case ND_COND:
    printf("%s-Cond '%s'\n", local_prefix, n->str);
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->cond, is_next_stmt, true, scope_prefix);
    print_stmt(n->then, is_next_stmt, true, scope_prefix);
    print_stmt(n->els, is_next_stmt, false, scope_prefix);
    break;
  case ND_MEMBER:
    printf("%s-Var.Member '%s.%s'\n", local_prefix, n->lhs->str, n->str);
    break;
  case ND_CAST:
    printf("%s-Cast %s\n", local_prefix, type_to_name(n->type->kind));
    if (is_next_node) {
      snprintf(buf, MAX_LEN, "%s |", scope_prefix);
    } else {
      snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
    }
    scope_prefix = strndup(buf, MAX_LEN);
    print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
    break;
  default:
    printf("%s-none(%d)\n", local_prefix, n->kind);
    break;
  }
}

void print_compound_stmt(struct node *stmt, bool is_next) {
  if (!stmt)
    return;

  char buf[MAX_LEN];
  char *global_prefix;
  char *prefix;
  if (is_next) {
    global_prefix = "| ";
  } else {
    global_prefix = "  ";
  }
  if (stmt) {
    prefix = strndup(global_prefix, MAX_LEN);
    printf("%s`-CompoundStmt\n", prefix);
    snprintf(buf, MAX_LEN, "%s ", prefix);

    prefix = buf;

    for (struct node *n = stmt; n; n = n->next) {
      if (n->next == NULL) {
        print_stmt(n, true, false, prefix);
      } else {
        print_stmt(n, true, true, prefix);
      }
    }
  }
}

void print_ast(struct program *pr, char *funcname) {
  struct function *last;

  if (funcname) {
    struct function *f = find_func(funcname);
    printf("`-Function '%s'\n", f->name);
    print_param(f->params, false, f);
    print_compound_stmt(f->stmt, false);
    printf("\n");
    return;
  }

  bool has_function = pr->functions ? true : false;
  bool has_struct = tags ? true : false;
  print_globals(has_function || has_struct);

  print_struct(has_function);

  for (struct function *fn = pr->functions; fn; fn = fn->next)
    last = fn;

  for (struct function *fn = pr->functions; fn; fn = fn->next) {
    if (fn == last) {
      printf("`-Function '%s'\n", fn->name);
      print_param(fn->params, false, fn);
      print_compound_stmt(fn->stmt, false);
    } else {
      printf("|-Function '%s'\n", fn->name);
      print_param(fn->params, true, fn);
      print_compound_stmt(fn->stmt, true);
    }
  }
  printf("\n");
}

void skip(struct token **ret, struct token *tk, char *op) {
  if (!equal(tk, op)) {
    error_at(tk->loc, "expected '%s', but op is '%s'", op, tk->str);
  }

  *ret = tk->next;
}

bool consume_end(struct token **ret, struct token *tk) {

  if (equal(tk, "}")) {
    *ret = tk->next;
    return true;
  }

  if (equal(tk, ",") && equal(tk->next, "}")) {
    *ret = tk->next->next;
    return true;
  }

  return false;
}

void expect(struct token **ret, struct token *tk, char *op) {
  if (tk->kind != TK_RESERVED || !equal(tk, op)) {
    error_at(tk->loc, "'%s'ではありません", op);
  }
  *ret = tk->next;
}

int expect_number(struct token **ret, struct token *tk) {
  if (tk->kind != TK_NUM) {
    error_at(tk->loc, "数ではありません");
  }
  *ret = tk->next;
  return tk->val;
}

int get_number(struct token *tok) {
  if (tok->kind != TK_NUM) {
    error_at(tok->loc, "expected an number");
  }
  return tok->val;
}

char *get_ident(struct token *tok) {
  if (tok->kind != TK_IDENT) {
    error_at(tok->loc, "expected an identifier");
  }

  return strndup(tok->str, tok->len);
}

struct member *get_member(struct type *ty, struct token *tk) {
  for (struct member *m = ty->members; m; m = m->next) {
    if (strlen(m->name) == tk->len && strncmp(m->name, tk->str, tk->len) == 0)
      return m;
  }

  return NULL;
}

bool is_typename(struct token *tok) {
  char *keyword[] = {"int", "void", "char", "short", "long", "bool", "struct"};
  for (int i = 0; i < sizeof(keyword) / sizeof(*keyword); i++) {
    if (equal(tok, keyword[i])) {
      return true;
    }
  }

  return false;
}

struct function *find_func(char *name) {
  for (struct function *fn = functions->next; fn; fn = fn->next) {
    if (strlen(fn->name) == strlen(name) &&
        strncmp(fn->name, name, strlen(name)) == 0) {
      return fn;
    }
  }

  struct function *fn = current_fn;
  if (strlen(fn->name) == strlen(name) &&
      strncmp(fn->name, name, strlen(name)) == 0) {
    return fn;
  }
  return NULL;
}

struct var *find_var(struct token *tok) {
  // for (struct var *var = locals; var; var = var->next) {
  //   debug("locals: %s", var->name);
  // }
  for (struct var *var = locals; var; var = var->next) {
    if (strlen(var->name) == tok->len &&
        strncmp(var->name, tok->str, tok->len) == 0) {
      return var;
    }
  }
  for (struct var *var = globals; var; var = var->next) {
    if (strlen(var->name) == tok->len &&
        strncmp(var->name, tok->str, tok->len) == 0) {
      return var;
    }
  }
  return NULL;
}

struct tag_scope *find_tag(struct token *tk) {
  for (struct tag_scope *t = tags; t; t = t->next) {
    if (strlen(t->name) == tk->len && strncmp(t->name, tk->str, tk->len) == 0)
      return t;
  }

  return NULL;
}
struct var_scope *find_var_scope(struct token *tk) {
  for (struct var_scope *v = vars; v; v = v->next) {
    if (strlen(v->name) == tk->len && strncmp(v->name, tk->str, tk->len) == 0)
      return v;
  }

  return NULL;
}
bool is_void_assign_element(struct node *node) {
  if (!node) {
    return false;
  }
  if (node->kind == ND_FUNCALL) {
    struct function *f = find_func(node->str);
    if (f->type->kind == VOID)
      return true;
    else
      return false;
  }

  // return is_void_assign_element(node->lhs);
  return is_void_assign_element(node->rhs);
}

bool at_eof(struct token *tk) { return tk->kind == TK_EOF; }

struct node *new_node(enum node_kind kind, struct token *token) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = kind;
  n->str = token->str;
  return n;
}

struct node *new_node_binary(enum node_kind kind, struct node *lhs,
                             struct node *rhs) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = kind;
  n->lhs = lhs;
  n->rhs = rhs;
  return n;
}

struct node *new_node_num(int val) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = ND_NUM;
  n->val = val;
  n->type = copy_type(ty_int);
  return n;
}

struct node *new_node_var(struct var *var, struct type *ty) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = ND_VAR;
  n->var = var;
  n->str = var->name;
  n->type = ty;
  return n;
}

struct node *new_node_unary(enum node_kind kind, struct node *expr,
                            struct token *token) {
  struct node *n = new_node(kind, token);
  n->lhs = expr;
  return n;
}

struct node *new_node_expr(struct token **ret, struct token *tk) {
  struct node *n = new_node(ND_EXPR_STMT, tk);
  n->lhs = expr(ret, tk);
  return n;
}

struct node *new_cast(struct node *expr, struct type *ty) {
  add_type(expr);
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = ND_CAST;
  n->lhs = expr;
  n->type = copy_type(ty);
  return n;
}

int eval(struct node *node, struct var **var) {
  switch (node->kind) {
  case ND_ADD:
    return eval(node->lhs, var) + eval(node->rhs, NULL);
  case ND_MUL:
    return eval(node->lhs, NULL) * eval(node->rhs, NULL);
  case ND_NUM:
    return node->val;
  case ND_VAR:
    *var = node->var;
    return 0;
  default:
    return 0;
  }
}

struct node *new_add(struct node *lhs, struct node *rhs) {
  add_type(lhs);
  add_type(rhs);

  // num + num
  if (is_integer(lhs->type) && is_integer(rhs->type)) {
    return new_node_binary(ND_ADD, lhs, rhs);
  }

  // num + ptr to ptr + num
  if (!lhs->type->ptr_to && rhs->type->ptr_to) {
    struct node *tmp;
    tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  // ptr + num
  // num * sizeof(type)
  rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->type->ptr_to->size));
  return new_node_binary(ND_ADD, lhs, rhs);
}

struct node *new_sub(struct token *tk, struct node *lhs, struct node *rhs) {
  add_type(lhs);
  add_type(rhs);

  // num - num
  if (is_integer(lhs->type) && is_integer(rhs->type)) {
    return new_node_binary(ND_SUB, lhs, rhs);
  }

  // ptr - num
  if (lhs->type->ptr_to && rhs->type->kind == INT) {
    rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->type->ptr_to->size));
    return new_node_binary(ND_SUB, lhs, rhs);
  }

  // ptr - ptr
  if (lhs->type->ptr_to && rhs->type->ptr_to) {
    struct node *sub = new_node_binary(ND_SUB, lhs, rhs);
    return new_node_binary(ND_DIV, sub, new_node_num(lhs->type->ptr_to->size));
  }

  error_at(tk->loc, "invalid operands");
  return NULL;
}

struct tag_scope *new_tagscope(struct type *type) {
  struct tag_scope *tag = calloc(1, sizeof(struct tag_scope));
  tag->name = type->name;
  tag->next = tags;
  tag->type = type;
  tags = tag;
  return tag;
}

struct var_scope *new_varscope(char *name) {
  struct var_scope *vs = calloc(1, sizeof(struct var_scope));
  vs->name = name;
  vs->next = vars;
  vars = vs;
  return vs;
}

struct var *new_lvar(char *name, struct type *type) {
  struct var *v = calloc(1, sizeof(struct var));
  v->name = name;
  v->next = locals;
  v->type = type;
  v->is_local = true;
  locals = v;
  return v;
}

char *gvar_name() {
  char *buf = calloc(24, sizeof(char));
  static int inc = 0;
  snprintf(buf, 24, ".LC%d", inc++);
  return buf;
}
struct var *new_gvar(struct type *type) {
  struct var *v = calloc(1, sizeof(struct var));
  v->name = type->name;
  v->next = globals;
  v->type = type;
  v->is_local = false;
  globals = v;
  return v;
}

struct var *new_string_literal(char *data, int len) {
  struct type *type = array_to(ty_char, len);
  struct var *v = new_gvar(type);
  v->name = gvar_name();
  v->data = data;
  return v;
}

struct type *pointers(struct token **ret, struct token *tk, struct type *ty) {

  while (consume(&tk, tk, "*")) {
    ty = pointer_to(ty);
  }
  *ret = tk;
  return ty;
}

struct type *typespec(struct token **ret, struct token *tk) {

  struct type *ty = ty_int;
  if (consume(&tk, tk, "short")) {
    ty = copy_type(ty_short);
  }

  if (consume(&tk, tk, "long")) {
    ty = copy_type(ty_long);
  }

  if (consume(&tk, tk, "int")) {
    ty = copy_type(ty_int);
  }

  if (consume(&tk, tk, "char")) {
    ty = copy_type(ty_char);
  }

  if (consume(&tk, tk, "void")) {
    ty = copy_type(ty_void);
  }
  if (consume(&tk, tk, "bool")) {
    ty = copy_type(ty_bool);
  }

  if (consume(&tk, tk, "struct")) {
    ty = struct_declarator(&tk, tk);
  }

  if (consume(&tk, tk, "enum")) {
    ty = enum_declarator(&tk, tk);
  }

  *ret = tk;
  return ty;
}

struct member *struct_members(struct token **ret, struct token *tk) {

  struct member head = {};
  struct member *cur = &head;

  // まずは①
  // ① struct-member = typespec declarator ";"
  // struct-member = (typespec declarator ( "," declarator )* ";" )*
  skip(&tk, tk, "{");
  while (!equal(tk, "}")) {
    struct member *m = calloc(1, sizeof(struct member));

    struct type *base_type = typespec(&tk, tk);
    m->type = declarator(&tk, tk, base_type);
    m->name = m->type->name;
    m->align = m->type->align;

    cur = cur->next = m;

    skip(&tk, tk, ";");
  }

  *ret = tk->next;
  return head.next;
}

struct type *struct_declarator(struct token **ret, struct token *tk) {

  struct token *tag = NULL;
  if (tk->kind == TK_IDENT) {
    tag = tk;
    tk = tk->next;
  }

  if (tag && !equal(tk, "{")) {

    *ret = tk;
    struct tag_scope *t = find_tag(tag);
    if (t) {
      return t->type;
    }

    struct type *ty = calloc(1, sizeof(struct type));
    ty->kind = STRUCT;

    return ty;
  }

  struct type *ty = calloc(1, sizeof(struct type));

  ty->kind = STRUCT;
  ty->members = struct_members(&tk, tk);
  // if (tag) {
  ty->name = tag->str;
  // }

  new_tagscope(ty);

  // align and offset calculation
  // アライメント
  // 各メンバーのアドレスは型のアライメントの倍数になる
  // アドレスがアライメントの倍数にならない場合パディングを行う（offsetを調整）
  //
  int max_align = 0;
  int offset = 0;
  for (struct member *m = ty->members; m; m = m->next) {
    if (offset % m->align == 0) { // メンバーのアドレスがアライメント倍数
      m->offset = offset;
    } else {
      m->offset = offset + (m->align - (offset % m->align));
      offset = m->offset;
    }
    offset += m->type->size;

    if (ty->align < m->align)
      ty->align = m->align;

    ty->size = offset + m->type->size;
  }

  if (ty->size % ty->align != 0) {
    ty->size = offset + (ty->align - (offset % ty->align));
  }

  *ret = tk;
  return ty;
}

struct type *enum_declarator(struct token **ret, struct token *tk) {

  struct token *tag = consume_ident(&tk, tk);

  if (tag && !equal(tk, "{")) {
    *ret = tk;
    struct tag_scope *ts = find_tag(tk);
    if (!ts) {
      error_at(tk->loc, "unknown enum");
    }
    if (ts->type->kind != ENUM) {
      error_at(tk->loc, "not an enum tag");
    }
    return ts->type;
  }

  struct type *ty = copy_type(ty_enum);
  ty->name = tag->str;

  new_tagscope(ty);

  skip(&tk, tk, "{");

  int i = 0;
  int val = 0;
  while (!consume_end(&tk, tk)) {
    if (0 < i++) {
      skip(&tk, tk, ",");
    }

    char *name = get_ident(tk);
    tk = tk->next;

    if (equal(tk, "=")) {
      val = get_number(tk->next);
      tk = tk->next->next;
    }

    struct var_scope *vs = new_varscope(name);
    vs->enum_ty = ty;
    vs->enum_val = val++;
  }

  if (tag) {
    new_tagscope(ty);
  }

  *ret = tk;
  return ty;
}

struct type *typename(struct token **ret, struct token *tk) {

  struct type *ty = typespec(&tk, tk);
  *ret = tk;
  return pointers(ret, tk, ty);
}

struct function *funcdef(struct token **ret, struct token *tk,
                         struct type *type) {
  locals = NULL;

  struct function *fn = calloc(1, sizeof(struct function));

  current_fn = fn;
  fn->name = type->name;
  fn->type = type->return_type;
  fn->token = tk;

  for (struct type *ty = type->params; ty; ty = ty->next) {
    new_lvar(ty->name, ty);
  }

  fn->params = locals;

  fn->stmt = compound_stmt(ret, tk)->body;
  fn->locals = locals;

  return fn;
}

struct type *declarator(struct token **ret, struct token *tk,
                        struct type *base) {
  struct type *type = base;

  type = pointers(&tk, tk, type);
  // ident
  if (tk->kind != TK_IDENT) {
    error_at(tk->loc, "expected a variable name");
  }

  char *type_name = get_ident(tk);

  type = type_suffix(&tk, tk->next, type);
  type->name = type_name;

  *ret = tk;
  return type;
}

struct type *type_suffix(struct token **ret, struct token *tk,
                         struct type *type) {

  if (consume(&tk, tk, "(")) {
    return funcdef_args(ret, tk, type);
  }

  if (consume(&tk, tk, "[")) {
    int size = 0;
    if (!equal(tk, "]")) {
      size = get_number(tk);
      tk = tk->next;
    }
    skip(&tk, tk, "]");
    type = type_suffix(&tk, tk, type);
    type = array_to(type, size);
  }

  *ret = tk;
  return type;
}

struct type *funcdef_args(struct token **ret, struct token *tk,
                          struct type *type) {

  struct type head = {};
  struct type *cur = &head;
  while (!equal(tk, ")")) {
    if (cur != &head) {
      skip(&tk, tk, ",");
    }
    struct type *ty;
    ty = typespec(&tk, tk);
    ty = declarator(&tk, tk, ty);

    cur = cur->next = copy_type(ty);
  }
  skip(&tk, tk, ")");

  type->return_type = type;
  type->params = head.next;
  *ret = tk;
  return type;
}

struct node *declaration(struct token **ret, struct token *tk) {

  struct node *node;
  struct var *var;

  struct type *base = typespec(&tk, tk);

  struct node head = {};
  struct node *cur = &head;
  int count = 0;
  while (!equal(tk, ";")) {
    if (0 < count++) {
      skip(&tk, tk, ",");
    }

    struct type *type = declarator(&tk, tk, base);

    var = new_lvar(type->name, type);

    if (consume(&tk, tk, "=")) {
      cur = cur->next = lvar_initializer(&tk, tk, var, type);
    } else
      cur = cur->next = new_node_var(var, type);
  }

  node = new_node(ND_BLOCK, tk);
  node->body = head.next;
  *ret = tk;
  return node;
}

struct node *assign(struct token **ret, struct token *tk) {
  struct node *n = conditional(&tk, tk);
  struct token *start = tk;
  if (consume(&tk, tk, "=")) {
    n = new_node_binary(ND_ASSIGN, n, assign(&tk, tk));
    n->str = "=";
    if (is_void_assign_element(n->rhs))
      // error_at(start->loc, "Assigning to type from incompatible type
      // 'void'");
      error_at(start->loc, "Can't Assign to void function");
  }

  if (consume(&tk, tk, "+=")) {
    *ret = tk->next;
    return new_node_binary(ND_ASSIGN, n,
                           new_add(n, new_node_num(get_number(tk))));
  }

  if (consume(&tk, tk, "-=")) {
    *ret = tk->next;
    return new_node_binary(ND_ASSIGN, n,
                           new_sub(tk, n, new_node_num(get_number(tk))));
  }

  if (consume(&tk, tk, "*=")) {
    *ret = tk->next;
    return new_node_binary(
        ND_ASSIGN, n, new_node_binary(ND_MUL, n, new_node_num(get_number(tk))));
  }

  if (consume(&tk, tk, "/=")) {
    *ret = tk->next;
    return new_node_binary(
        ND_ASSIGN, n, new_node_binary(ND_DIV, n, new_node_num(get_number(tk))));
  }

  if (consume(&tk, tk, "++")) {
    *ret = tk;
    return new_node_binary(ND_ASSIGN, n, new_add(n, new_node_num(1)));
  }

  if (consume(&tk, tk, "--")) {
    *ret = tk;
    return new_node_binary(ND_ASSIGN, n, new_sub(tk, n, new_node_num(1)));
  }

  *ret = tk;
  return n;
}

struct node *conditional(struct token **ret, struct token *tk) {
  struct node *node = logor(&tk, tk);

  if (consume(&tk, tk, "?")) {
    struct node *cond = new_node(ND_COND, tk);

    cond->str = "?:";
    cond->cond = node;
    cond->then = expr(&tk, tk);
    skip(&tk, tk, ":");
    cond->els = conditional(&tk, tk);

    *ret = tk;
    return cond;
  }

  *ret = tk;
  return node;
}

struct node *logor(struct token **ret, struct token *tk) {
  struct node *node = logand(&tk, tk);

  while (equal(tk, "||")) {
    node = new_node_binary(ND_LOGOR, node, logand(&tk, tk->next));
    node->str = "||";
  }

  *ret = tk;
  return node;
}

struct node *logand(struct token **ret, struct token *tk) {
  struct node *node = bitor (&tk, tk);

  while (equal(tk, "&&")) {
    node = new_node_binary(ND_LOGAND, node, bitor (&tk, tk->next));
    node->str = "&&";
  }

  *ret = tk;
  return node;
}

struct node * bitor (struct token * *ret, struct token *tk) {
  struct node *node = bitxor(&tk, tk);

  while (equal(tk, "|")) {
    node = new_node_binary(ND_BITOR, node, bitxor(&tk, tk->next));
    node->str = "|";
  }

  *ret = tk;
  return node;
}

struct node *bitxor(struct token **ret, struct token *tk) {
  struct node *node = bitand(&tk, tk);

  while (equal(tk, "^")) {
    node = new_node_binary(ND_BITXOR, node, bitand(&tk, tk->next));
    node->str = "^";
  }

  *ret = tk;
  return node;
}

struct node *bitand(struct token **ret, struct token *tk) {
  struct node *node = equality(&tk, tk);

  while (equal(tk, "&")) {
    node = new_node_binary(ND_BITAND, node, equality(&tk, tk->next));
    node->str = "&";
  }

  *ret = tk;
  return node;
}
struct node *compound_stmt(struct token **ret, struct token *tk) {
  struct node *n = NULL;

  if (!equal(tk, "{"))
    return NULL;

  n = new_node(ND_BLOCK, tk);
  n->str = "{}";

  skip(&tk, tk, "{");

  struct node head = {};
  struct node *cur = &head;

  while (!equal(tk, "}")) {
    if (is_typename(tk)) {
      cur = cur->next = declaration(&tk, tk);
      skip(&tk, tk, ";");
    } else {
      cur = cur->next = stmt(&tk, tk);
    }
    add_type(cur);
  }

  n->body = head.next;
  skip(&tk, tk, "}");

  *ret = tk;

  return n;
}

struct node *expr(struct token **ret, struct token *tk) {
  struct node *n = assign(&tk, tk);

  *ret = tk;
  return n;
}

struct node *stmt(struct token **ret, struct token *tk) {
  struct node *n = NULL;

  if (equal(tk, "return")) {
    n = new_node(ND_RETURN, tk);

    if (current_fn->type->kind != VOID) {
      n->lhs = expr(&tk, tk->next);
    } else {
      if (!equal(tk->next, ";"))
        error_at(tk->loc, "Void function '%s' should not return a value",
                 current_fn->name);
      tk = tk->next;
    }
    skip(&tk, tk, ";");

    *ret = tk;
    return n;
  }

  // "if" "(" expr ")" stmt ("else" stmt)?
  if (equal(tk, "if")) {
    n = new_node(ND_IF, tk);

    skip(&tk, tk->next, "(");
    n->cond = expr(&tk, tk);
    skip(&tk, tk, ")");

    n->then = stmt(&tk, tk);

    if (equal(tk, "else")) {
      tk = tk->next;
      n->els = stmt(&tk, tk);
    }

    *ret = tk;
    return n;
  }

  // "while" "(" expr ")" stmt
  if (equal(tk, "while")) {
    n = new_node(ND_FOR, tk);
    skip(&tk, tk->next, "(");
    n->cond = expr(&tk, tk);
    skip(&tk, tk, ")");
    n->then = stmt(&tk, tk);

    *ret = tk;
    return n;
  }

  // "for" "(" ( declaration | expr )? ";" expr? ";" expr? ")" stmt
  if (equal(tk, "for")) {
    n = new_node(ND_FOR, tk);
    skip(&tk, tk->next, "(");
    if (!equal(tk, ";")) {
      if (tk->kind == TK_RESERVED)
        n->init = declaration(&tk, tk);
      else
        n->init = new_node_expr(&tk, tk);
    }
    skip(&tk, tk, ";");

    if (!equal(tk, ";")) {
      n->cond = expr(&tk, tk);
    }
    skip(&tk, tk, ";");

    if (!equal(tk, ")")) {
      n->inc = new_node_expr(&tk, tk);
    }
    skip(&tk, tk, ")");

    n->then = stmt(&tk, tk);

    *ret = tk;
    return n;
  }

  if (equal(tk, "{")) {
    return compound_stmt(ret, tk);
  }

  n = new_node_expr(&tk, tk);
  skip(&tk, tk, ";");
  *ret = tk;
  return n;
}

struct node *equality(struct token **ret, struct token *tk) {
  struct node *n = relational(&tk, tk);
  while (true) {
    if (consume(&tk, tk, "==")) {
      n = new_node_binary(ND_EQ, n, relational(&tk, tk));
      n->str = "==";
    } else if (consume(&tk, tk, "!=")) {
      n = new_node_binary(ND_NE, n, relational(&tk, tk));
      n->str = "!=";
    } else {

      *ret = tk;
      return n;
    }
  }
}

struct node *relational(struct token **ret, struct token *tk) {
  struct node *n = add(&tk, tk);

  while (true) {
    if (consume(&tk, tk, "<")) {
      n = new_node_binary(ND_LT, n, add(&tk, tk));
      n->str = "<";
    } else if (consume(&tk, tk, "<=")) {
      n = new_node_binary(ND_LE, n, add(&tk, tk));
      n->str = "<=";
    } else if (consume(&tk, tk, ">")) {
      n = new_node_binary(ND_GT, n, add(&tk, tk));
      n->str = ">";
    } else if (consume(&tk, tk, ">=")) {
      n = new_node_binary(ND_GE, n, add(&tk, tk));
      n->str = ">=";
    } else {
      *ret = tk;
      return n;
    }
  }
}

struct node *add(struct token **ret, struct token *tk) {
  struct node *n = mul(&tk, tk);
  while (true) {
    if (consume(&tk, tk, "+")) {
      n = new_add(n, mul(&tk, tk));
      n->str = "+";
    } else if (consume(&tk, tk, "-")) {
      n = new_sub(tk, n, mul(&tk, tk));
      n->str = "-";
    } else {
      *ret = tk;
      return n;
    }
  }
}

struct node *mul(struct token **ret, struct token *tk) {
  struct node *n = cast(&tk, tk);
  while (true) {
    if (consume(&tk, tk, "*")) {
      n = new_node_binary(ND_MUL, n, cast(&tk, tk));
      n->str = "*";
    } else if (consume(&tk, tk, "/")) {
      n = new_node_binary(ND_DIV, n, cast(&tk, tk));
      n->str = "/";
    } else {
      *ret = tk;
      return n;
    }
  }
}

struct node *cast(struct token **ret, struct token *tk) {

  if (equal(tk, "(") && is_typename(tk->next)) {
    struct type *cast_type = typename(&tk, tk->next);
    skip(&tk, tk, ")");
    *ret = tk;
    return new_cast(unary(ret, tk), cast_type);
  }

  return unary(ret, tk);
}

struct node *unary(struct token **ret, struct token *tk) {

  if (consume(&tk, tk, "+")) {
    return cast(ret, tk);
  }
  if (consume(&tk, tk, "-")) {
    return new_node_binary(ND_SUB, new_node_num(0), cast(ret, tk));
  }

  if (consume(&tk, tk, "*")) {
    return new_node_unary(ND_DEREF, cast(ret, tk), tk);
  }
  if (consume(&tk, tk, "&")) {
    return new_node_unary(ND_ADDR, cast(ret, tk), tk);
  }
  if (consume(&tk, tk, "~")) {
    return new_node_unary(ND_BITNOT, cast(ret, tk), tk);
  }
  // ++a
  // assign a = a + 1
  if (consume(&tk, tk, "++")) {
    struct node *a = unary(ret, tk);
    return new_node_binary(ND_ASSIGN, a, new_add(a, new_node_num(1)));
  }
  // --a
  if (consume(&tk, tk, "--")) {
    struct node *a = unary(ret, tk);
    return new_node_binary(ND_ASSIGN, a, new_sub(tk, a, new_node_num(1)));
  }
  return postfix(ret, tk);
}

struct node *postfix(struct token **ret, struct token *tk) {

  struct token *start = tk;

  struct node *n = primary(&tk, tk);

  while (true) {
    if (consume(&tk, tk, "[")) {
      // while (consume(&tk, tk, "[")) {
      // struct token *start = tk;
      struct node *index = expr(&tk, tk);
      skip(&tk, tk, "]");
      n = new_node_unary(ND_DEREF, new_add(n, index), start);
      // }
      // tk = tk;
      continue;
    }

    if (consume(&tk, tk, ".")) {
      add_type(n);
      n = new_node_unary(ND_MEMBER, n, tk);
      n->member = get_member(n->lhs->type, tk);
      tk = tk->next;
      continue;
    }

    if (consume(&tk, tk, "->")) {
      n = new_node_unary(ND_DEREF, n, tk);
      add_type(n);
      n = new_node_unary(ND_MEMBER, n, tk);
      n->member = get_member(n->lhs->type, tk);
      tk = tk->next;
      continue;
    }

    *ret = tk;
    return n;
  }
}

struct node *primary(struct token **ret, struct token *tk) {
  struct node *n;

  if (equal(tk, "(") && equal(tk->next, "{")) {
    n = new_node(ND_STMT_EXPR, tk);
    n->body = compound_stmt(&tk, tk->next)->body;

    skip(&tk, tk, ")");

    *ret = tk;
    return n;
  }

  if (consume(&tk, tk, "(")) {
    n = expr(&tk, tk);
    skip(&tk, tk, ")");
    *ret = tk;
    return n;
  }

  struct token *ident = consume_ident(&tk, tk);
  if (ident) {
    if (equal(tk, "(")) {
      return funcall(ret, tk, ident);
    }

    struct var_scope *vs = find_var_scope(ident);
    if (vs) {
      *ret = tk;
      return new_node_num(vs->enum_val);
    }

    struct var *var = find_var(ident);
    if (!var) {
      error_at(ident->loc, "変数%sは定義されていません", ident->str);
    }

    *ret = tk;
    return new_node_var(var, var->type);
  }

  if (tk->kind == TK_SIZEOF) {
    skip(&tk, tk, "sizeof");
    struct node *node = unary(&tk, tk);
    add_type(node);
    *ret = tk;
    return new_node_num(node->type->size);
  }

  if (tk->kind == TK_STR) {
    struct var *v = new_string_literal(tk->str, tk->len);
    tk = tk->next;
    *ret = tk;
    return new_node_var(v, v->type);
  }

  return new_node_num(expect_number(ret, tk));
}

struct node *funcall(struct token **ret, struct token *tk,
                     struct token *ident) {

  struct node head = {};
  struct node *cur = &head;

  skip(&tk, tk, "(");

  struct node *funcall = new_node(ND_FUNCALL, ident);

  struct function *fn = find_func(ident->str);
  struct var *param = NULL;
  struct node *n;

  if (fn) {
    param = NULL;
    funcall->func_ty = fn->type;

    for (struct var *v = fn->params; v; v = v->next) {
      struct var *v0 = calloc(1, sizeof(struct var));
      v0->type = v->type;
      v0->name = v->name;
      v0->next = param;
      param = v0;
    }
  } else {
    funcall->func_ty = ty_void;
  }

  int nargs = 0;

  while (!equal(tk, ")")) {

    if (cur != &head)
      skip(&tk, tk, ",");

    n = assign(&tk, tk);
    add_type(n);

    if (param) {
      n = new_cast(n, param->type);
      param = param->next;
    }

    struct var *v = n->type->ptr_to ? new_lvar("", pointer_to(n->type->ptr_to))
                                    : new_lvar("", n->type);

    if (param) {
      v->name = param->name;
      v->type->name = param->name;
    }

    cur = cur->next = n;
    funcall->args[nargs++] = v;
  }
  skip(&tk, tk, ")");

  funcall->nargs = nargs;
  funcall->str = ident->str;
  funcall->body = head.next;
  funcall->type = funcall->func_ty->return_type;

  *ret = tk;
  return funcall;
}

struct node *string_initializer(struct token **ret, struct token *tk,
                                struct var *var, struct type *type) {

  struct node head = {};
  struct node *cur = &head;

  struct node *lvar = new_node_var(var, var->type);
  int array_size = 0;
  struct token *start = tk;

  for (int i = 0; i < tk->len; i++) {
    struct node *deref =
        new_node_unary(ND_DEREF, new_add(lvar, new_node_num(i)), start);
    struct node *node = new_node(ND_EXPR_STMT, start);
    node->lhs = new_node_binary(ND_ASSIGN, deref, new_node_num(tk->str[i]));
    node->lhs->str = "=";
    cur = cur->next = node;
  }
  array_size = start->len;

  //
  if (type->array_size == 0) {
    type->array_size = array_size;
  }

  if (type->array_size < array_size) {
    error_at(start->loc, "Initializer-string for char array is too long\n");
  }

  *ret = tk->next;
  return head.next;
}

struct node *array_initializer(struct token **ret, struct token *tk,
                               struct var *var, struct type *type) {
  struct node *node;
  struct token *start;

  struct node head = {};
  struct node *cur = &head;

  struct node *lvar = new_node_var(var, var->type);
  int array_size = 0;

  if (equal(tk, "{")) {
    tk = tk->next;

    int cnt = 0;
    while (!equal(tk, "}")) {
      if (cnt)
        skip(&tk, tk, ",");
      start = tk;
      struct node *deref =
          new_node_unary(ND_DEREF, new_add(lvar, new_node_num(cnt)), start);
      node = new_node(ND_EXPR_STMT, start);
      node->lhs = new_node_binary(ND_ASSIGN, deref, unary(&tk, tk));
      node->lhs->str = "=";

      cur = cur->next = node;
      cnt++;
    }
  }
  if (type->array_size == 0) {
    type->array_size = array_size;
  }

  if (type->array_size < array_size) {
    error_at(start->loc, "Initializer-string for char array is too long\n");
  }

  *ret = tk->next;
  return head.next;
}

struct node *struct_initializer(struct token **ret, struct token *tk,
                                struct var *var, struct type *type) {
  return NULL;
}

struct node *lvar_initializer(struct token **ret, struct token *tk,
                              struct var *var, struct type *type) {
  if (type->kind == ARRAY && type->ptr_to->kind == CHAR) {
    return string_initializer(ret, tk, var, type);
  }
  if (type->kind == ARRAY) {
    return array_initializer(ret, tk, var, type);
  }

  if (type->kind == STRUCT) {
    return struct_initializer(ret, tk, var, type);
  }

  struct node *lvar = new_node_var(var, type);
  struct node *node = new_node(ND_EXPR_STMT, tk);
  node->lhs = new_node_binary(ND_ASSIGN, lvar, assign(&tk, tk));
  node->lhs->str = "=";
  *ret = tk;
  return node;
}
void gvar_initilizer(struct token **ret, struct token *tk, struct var *var,
                     struct type *type) {

  if (type->kind == ARRAY && type->ptr_to->kind == CHAR) {
    var->data = tk->str;
    *ret = tk->next;
    return;
  }
  if (type->kind == ARRAY) {
    if (consume(&tk, tk, "{")) {
      struct value head = {};
      struct value *cur = &head;
      int cnt = 0;

      while (!equal(tk, "}")) {
        if (0 < cnt++)
          skip(&tk, tk, ",");

        struct node *n = assign(&tk, tk);

        cur = cur->next = calloc(1, sizeof(struct value));

        if (type->ptr_to->kind == INT)
          cur->val = n->val;
        if (type->ptr_to->kind == PTR)
          cur->label = n->str;
        //
        ;
      }
      var->val = head.next;
      if (var->type->array_size == 0)
        var->type->array_size = cnt;
    }

    *ret = tk->next;
    return;
  }

  struct node *node = assign(&tk, tk);

  if (node->kind == ND_NUM) {
    var->data = calloc(1, type->size);
    *(int *)var->data = node->val;
  }
  if (node->kind == ND_VAR) {
    var->data = node->var->name;
  }
  if (node->kind == ND_ADDR) {
    var->data = node->lhs->var->name;
  }

  if (node->kind == ND_ADD) {
    struct var *v = NULL;
    int addend = eval(node, &v);
    var->data = v->name;
    var->addend = addend;
  }

  *ret = tk;
  return;
}

// program = ( funcdef | declaration ";" )*
// typespec = "int" | "char" | struct-declarator | enum-declarator
// typename = typespec pointers
// pointers = ( "*" )*
// funcdef = typespec declarator compound-stmt
//
// declarator = pointers ident type-suffix
// struct-declarator = "struct" ident "{" sturct-member "}" ";"
// struct-member = (typespec declarator ( "," declarator )* ";" )*
// enum-declarator = "enum" (
//                       | ident? "{" enum-list? "}"
//                       | ident ( "{" enum-list? "}" )?
//                       )
//
// enum-list = ident ( "=" num )? ( "," ident ( "=" num )? )* ","?
//
// type-suffix = ( "[" num "]" )*
//              | "(" funcdef-args? ")"
// funcdef-args = param ( "," param )*
// param = typespec declarator
// declaration = typespec declarator
//               ( "=" initializer )?
//               ( "," declarator ( "=" initializer )? )*
//
// initializer = assign | "{" unary ( "," unary )* "}"
// compound-stmt = "{" ( declaration ";" | stmt )* "}"
//
// stmt = expr ";"
//      | compound-stmt
//      | "if" "(" expr ")" stmt ( "else" stmt )?
//      | "while" "(" expr ")" stmt
//      | "for" "(" ( declaration | expr )? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
//
// expr = assign
//
// assign = conditional (
//          ( "=" | "+=" | "-=" | "*=" | "/=" ) assign
//          | ( "++" | "--" )
//        )?
//
// conditional = logor ( "?" expr ":" conditional )?
//
// logor = logand ( "||"  logand )*
// logand = bitor ( "&&"  bitor )*
// bitor = bitxor ( "|" bitxor )*
// bitxor = bitand ( "^" bitand *)
// bitand = equality ( "&" equality )*
//
// equality = relational ( "==" relational | "!=" relational )*
//
// relational = add ( "<" add | "<=" add | ">" add | ">=" add )*
//
// add = mul ( "+" mul | "-" mul )*
// mul = cast ( "*" cast | "/" cast )*
// cast = ( "(" typename ")" )? unary
// unary = ( "+" | "-" | "*" | "&" | "~" )? cast
//       | ( "++" | "--" ) unary
//       | postfix
//
// postfix = primary
//         | ( "[" expr "]" )*
//         | "." ident
//         | "->" ident
// primary = "(" "{" compound-stmt "}" ")"
//         | num
//         | ident funcall-args?
//         | "(" expr ")"
//         | sizeof unary
//         | string-literal
// funcall = ident "(" funcall-args ")"
// funcall-args = assign ( "," assign )*

struct program *parse(struct token *tk) {
  functions = NULL;
  globals = NULL;
  tags = NULL;
  vars = NULL;
  scope_depth = 0;

  struct function head = {};
  struct function *cur = &head;
  struct type *type;

  functions = cur;
  while (!at_eof(tk)) {

    type = typespec(&tk, tk);
    if (consume(&tk, tk, ";"))
      continue;

    type = declarator(&tk, tk, type);

    if (equal(tk, "{")) {
      cur = cur->next = funcdef(&tk, tk, type);
      continue;
    }

    if (equal(tk, ";")) {
      struct var *gvar = new_gvar(type);
    } else {
      struct var *gvar = new_gvar(type);
      while (!equal(tk, ";")) {
        if (consume(&tk, tk, "=")) {
          gvar_initilizer(&tk, tk, gvar, type);
        }
        if (consume(&tk, tk, ",")) {
          type = declarator(&tk, tk, type);
          gvar = new_gvar(type);
        }
      }
    }
    skip(&tk, tk, ";");
  }

  struct program *programs = calloc(1, sizeof(struct program));
  programs->functions = head.next;
  programs->globals = globals;
  return programs;
}
