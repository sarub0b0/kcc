#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct function *functions;
struct function *current_fn;
struct var *locals;
struct var *globals;

struct function *funcdef(struct token **, struct token *, struct type *);
struct type *declarator(struct token **, struct token *, struct type *);
struct type *funcdef_args(struct token **, struct token *, struct type *);
struct type *type_suffix(struct token **, struct token *, struct type *);
struct node *compound_stmt(struct token **, struct token *);
struct node *assign(struct token **, struct token *);
struct node *stmt(struct token **, struct token *);
struct node *expr(struct token **, struct token *);
struct node *equality(struct token **, struct token *);
struct node *relational(struct token **, struct token *);
struct node *add(struct token **, struct token *);
struct node *mul(struct token **, struct token *);
struct node *unary(struct token **, struct token *);
struct node *postfix(struct token **, struct token *);
struct node *primary(struct token **, struct token *);

struct node *funcall(struct token **, struct token *, struct token *);

struct node *lvar_initializer(struct token **, struct token *, struct var *,
                              struct type *);
void gvar_initializer(struct token **, struct token *, struct var *,
                      struct type *);

#define MAX_LEN (int)(256)

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
    if (v->next)
      printf("|-Param %s\n", v->name);
    else {
      if (has_function)
        printf("|-Param %s\n", v->name);
      else
        printf("`-Param %s\n", v->name);
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
  default:
    printf("%s-none\n", local_prefix);
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

void print_ast(struct program *pr) {
  struct function *last;

  bool has_function = pr->functions ? true : false;
  print_globals(has_function);

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

  return is_void_assign_element(node->lhs);
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

struct var *new_lvar(struct type *type) {
  struct var *v = calloc(1, sizeof(struct var));
  v->name = type->name;
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

struct type *typespec(struct token **ret, struct token *tk) {
  if (consume(&tk, tk, "int")) {
    *ret = tk;
    return copy_type(ty_int);
  }

  if (consume(&tk, tk, "char")) {
    *ret = tk;
    return copy_type(ty_char);
  }

  if (consume(&tk, tk, "void")) {
    *ret = tk;
    return copy_type(ty_void);
  }

  *ret = tk;
  return NULL;
}

struct function *funcdef(struct token **ret, struct token *tk,
                         struct type *type) {
  locals = NULL;

  struct function *fn = calloc(1, sizeof(struct function));

  current_fn = fn;
  fn->name = type->name;
  fn->type = type->return_type;

  for (struct type *ty = type->params; ty; ty = ty->next) {
    new_lvar(ty);
  }

  fn->params = locals;

  fn->stmt = compound_stmt(ret, tk)->body;
  fn->locals = locals;

  return fn;
}

struct type *declarator(struct token **ret, struct token *tk,
                        struct type *base) {
  struct type *type = base;

  while (consume(&tk, tk, "*")) {
    type = pointer_to(type);
  }
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

    var = new_lvar(type);

    if (consume(&tk, tk, "=")) {
      cur = cur->next = lvar_initializer(&tk, tk, var, type);
    }
  }

  node = new_node(ND_BLOCK, tk);
  node->body = head.next;
  *ret = tk;
  return node;
}

struct node *assign(struct token **ret, struct token *tk) {
  struct node *n = equality(&tk, tk);
  struct token *start = tk;
  if (consume(&tk, tk, "=")) {
    n = new_node_binary(ND_ASSIGN, n, assign(&tk, tk));
    n->str = "=";
    if (is_void_assign_element(n->rhs))
      // error_at(start->loc, "Assigning to type from incompatible type
      // 'void'");
      error_at(start->loc, "Can't Assign to void function");
  }

  *ret = tk;
  return n;
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
    if (equal(tk, "int") || equal(tk, "char")) {
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
  struct node *n = unary(&tk, tk);
  while (true) {
    if (consume(&tk, tk, "*")) {
      n = new_node_binary(ND_MUL, n, unary(&tk, tk));
      n->str = "*";
    } else if (consume(&tk, tk, "/")) {
      n = new_node_binary(ND_DIV, n, unary(&tk, tk));
      n->str = "/";
    } else {
      *ret = tk;
      return n;
    }
  }
}

struct node *unary(struct token **ret, struct token *tk) {

  if (tk->kind == TK_SIZEOF) {
    skip(&tk, tk, "sizeof");
    struct node *node = unary(&tk, tk);
    add_type(node);
    *ret = tk;
    return new_node_num(node->type->size);
  }

  if (consume(&tk, tk, "+")) {
    return unary(ret, tk);
  }
  if (consume(&tk, tk, "-")) {
    return new_node_binary(ND_SUB, new_node_num(0), unary(ret, tk));
  }

  if (consume(&tk, tk, "*")) {
    return new_node_unary(ND_DEREF, unary(ret, tk), tk);
  }
  if (consume(&tk, tk, "&")) {
    return new_node_unary(ND_ADDR, unary(ret, tk), tk);
  }
  return postfix(ret, tk);
}

struct node *postfix(struct token **ret, struct token *tk) {
  struct node *n = primary(&tk, tk);

  if (n->kind != ND_NUM) {
    while (consume(&tk, tk, "[")) {
      struct token *start = tk;
      struct node *index = expr(&tk, tk);
      skip(&tk, tk, "]");
      n = new_node_unary(ND_DEREF, new_add(n, index), start);
    }
  }
  *ret = tk;
  return n;
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

  struct token *tok = consume_ident(&tk, tk);
  if (tok) {
    if (equal(tk, "(")) {
      return funcall(ret, tk, tok);
    }

    struct var *var = find_var(tok);
    if (!var) {
      error_at(tok->loc, "変数%sは定義されていません", tok->str);
    }

    *ret = tk;
    return new_node_var(var, var->type);
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

  struct var vhead = {};
  struct var *vcur = &vhead;

  skip(&tk, tk, "(");

  struct node *funcall = new_node(ND_FUNCALL, ident);

  struct function *fn = find_func(ident->str);

  struct node *n;
  while (!equal(tk, ")")) {
    if (cur != &head)
      skip(&tk, tk, ",");

    n = assign(&tk, tk);
    add_type(n);

    n->type->name = strndup("", 256);
    vcur = vcur->next = calloc(1, sizeof(struct var));
    vcur->type = n->type;
    vcur->name = n->type->name;

    cur = cur->next = n;
  }
  skip(&tk, tk, ")");

  if (fn) {
    funcall->func_ty = fn->type;
    funcall->args = vhead.next;
    funcall->params = fn->params;
  } else {
    funcall->func_ty = ty_void;
    funcall->args = vhead.next;
  }
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

struct node *lvar_initializer(struct token **ret, struct token *tk,
                              struct var *var, struct type *type) {
  if (type->kind == ARRAY && type->ptr_to->kind == CHAR) {
    return string_initializer(ret, tk, var, type);
  }
  if (type->kind == ARRAY) {
    return array_initializer(ret, tk, var, type);
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
// typespec = "int" | "char"
// funcdef = typespec declarator compound-stmt
// declarator = "*"* ident type-suffix
// type-suffix = ( "[" num "]" )*
//              | "(" funcdef-args? ")"
// funcdef-args = param ( "," param )*
// param = typespec declarator
// declaration = typespec declarator
//               ( "=" initializer )?
//               ( "," declarator ( "=" initializer )? )*
//
// initialize = expr | "{" unary ( "," unary )* "}"
// compound-stmt = "{" ( declaration ";" | stmt )* "}"
//
// stmt = expr ";"
//      | compound-stmt
//      | "if" "(" expr ")" stmt ( "else" stmt )?
//      | "while" "(" expr ")" stmt
//      | "for" "(" ( declaration | expr )? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
// expr = assign
// assign = equality ( "=" assign )?
// equality = relational ( "==" relational | "!=" relational )*
// relational = add ( "<" add | "<=" add | ">" add | ">=" add )*
// add = mul ( "+" mul | "-" mul )*
// mul = unary ( "*" unary | "/" unary )*
// unary = "sizeof" unary | ( "+" | "-" | "*" | "&" )? unary | postfix
// postfix = primary ( "[" expr "]" )*
// primary = "(" "{" compound-stmt "}" ")"
//         | num
//         | ident funcall-args?
//         | "(" expr ")"
//         | string-literal
// funcall = ident "(" funcall-args ")"
// funcall-args = assign ( "," assign )*

struct program *parse(struct token *tk) {
  functions = NULL;
  globals = NULL;

  struct function head = {};
  struct function *cur = &head;
  struct type *type;

  functions = cur;
  while (!at_eof(tk)) {

    type = typespec(&tk, tk);
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
