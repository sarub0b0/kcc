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
  struct type *type_def;

  int depth;
  char *name;
  int enum_val;
};

struct init_data {
  struct token *token;
  struct type *type;
  int len;
  struct node *expr;

  struct init_data **child;
};

struct var_attr {
  bool is_static;
  bool is_extern;
  bool is_typedef;
};

static struct function *functions;
// static struct function *current_fn;
static struct var *current_fn;
static struct var *locals;
static struct var *globals;
static struct tag_scope *tags;
static struct var_scope *vars;
static int scope_depth;

struct function *funcdef(struct token **,
                         struct token *,
                         struct type *,
                         struct var_attr *);
struct type *declarator(struct token **, struct token *, struct type *);
struct type *struct_declarator(struct token **,
                               struct token *,
                               enum type_kind);
struct type *enum_declarator(struct token **, struct token *);
struct type *union_declarator(struct token **,
                              struct token *,
                              enum type_kind);
struct type *typedef_declarator(struct token **,
                                struct token *,
                                struct type *);
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
struct node *shift(struct token **, struct token *);
struct node *add(struct token **, struct token *);
struct node *mul(struct token **, struct token *);
struct node *cast(struct token **, struct token *);
struct node *unary(struct token **, struct token *);
struct node *postfix(struct token **, struct token *);
struct node *primary(struct token **, struct token *);

struct node *funcall(struct token **, struct token *, struct token *);

struct node *lvar_initializer(struct token **, struct token *, struct var *);
void gvar_initializer(struct token **, struct token *, struct var *);

struct init_data *initializer(struct token **, struct token *, struct type *);

struct function *find_func(char *);

struct tag_scope *find_tag(struct token *);
struct var_scope *find_var(struct token *);
struct type *find_typedef(struct token *);

#define MAX_LEN (int) (256)

void print_tok(struct token *tk, char end_str) {

  char *loc = tk->loc;
  char *line = tk->loc;
  char *input = tk->input;
  char *file = tk->filename;
  while (input < line && line[-1] != '\n') line--;

  char *end = line;
  while (*end != end_str) end++;

  end++;

  int line_num = 1;
  for (char *p = input; p < line; p++)
    if (*p == '\n') line_num++;

  fprintf(stdout, " %s:%d: ", file, line_num);
  fprintf(stdout, "%.*s\n", (int) (end - line), line);
}

void print_tok_pos(struct token *tk) {

  char *loc = tk->loc;
  char *line = tk->loc;
  char *input = tk->input;
  char *file = tk->filename;
  while (input < line && line[-1] != '\n') line--;

  char *end = line;
  while (*end != '\n') end++;

  end++;

  int line_num = 1;
  for (char *p = input; p < line; p++)
    if (*p == '\n') line_num++;

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

    if (!has_function) prefix = "`-";

    printf("%s%s ", prefix, type);
    if (v->type->kind == TY_STRUCT) {
      printf("struct %s %s", v->type->tag, v->name);
    } else if (v->type->kind == TY_UNION) {
      printf("union %s %s", v->type->tag, v->name);
    } else {
      printf("%s %s", type_to_name(v->type->kind), v->name);
    }
    if (v->data) {
      switch (v->type->kind) {
        case TY_INT:
          printf(": %d\n", *(int *) v->data);
          break;
        case TY_ARRAY:
          if (v->type->ptr_to->kind == TY_CHAR) {
            printf(": %s\n", (char *) v->data);
          }
          break;
        default:
          break;
      }
    } else
      printf("\n");
  }
}

void print_struct(bool has_function) {

  struct tag_scope *last = NULL;
  for (struct tag_scope *tag = tags; tag; tag = tag->next) last = tag;

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
      printf("%s%sMember %s %s\n",
             first_prefix,
             second_prefix,
             type_to_name(m->type->kind),
             m->name);
    }
  }
}

void print_stmt(struct node *n,
                bool is_next_stmt,
                bool is_next_node,
                char *prefix) {
  if (!n) return;

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
      printf("%s-Add '+'\n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
      print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
      break;
    case ND_SUB:
      printf("%s-Sub '-'\n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
      print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
      break;
    case ND_MUL:
      printf("%s-Mul '*'\n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
      print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
      break;
    case ND_DIV:
      printf("%s-Div '/'\n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
      print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
      break;
    case ND_BITOR:
      printf("%s-BitOr '|'\n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
      print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
      break;
    case ND_BITXOR:
      printf("%s-BitXor '^'\n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
      print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
      break;
    case ND_BITAND:
      printf("%s-BitAnd '&'\n", local_prefix);
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
      printf("%s-Var '%s'\n", local_prefix, n->var->name);
      break;
    case ND_ADDR:
      printf("%s-Addr '%s'\n", local_prefix, n->token->str);
      break;
    case ND_DEREF:
      printf("%s-Deref '%s'\n", local_prefix, n->token->str);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
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
    case ND_LIST_EXPR:
      printf("%s-ListExpr\n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      for (struct node *init = n->body; init; init = init->next) {
        if (init->next == NULL) {
          print_stmt(init, is_next_stmt, false, scope_prefix);
        } else {
          print_stmt(init, is_next_stmt, true, scope_prefix);
        }
      }
      break;
    case ND_ASSIGN:
      printf("%s-Assign '='\n", local_prefix);
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
      printf("%s-If '%s'\n", local_prefix, n->token->str);
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
      printf("%s-Loop '%s'\n", local_prefix, n->token->str);
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
      printf("%s-Cond '%s'\n", local_prefix, n->token->str);
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
      printf("%s-Block '%s'\n", local_prefix, n->token->str);
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
      printf("%s-Funcall '%s'\n", local_prefix, n->token->str);
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
    case ND_COMMA:
      printf("%s-Comma \n", local_prefix);
      if (is_next_node) {
        snprintf(buf, MAX_LEN, "%s |", scope_prefix);
      } else {
        snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
      }
      scope_prefix = strndup(buf, MAX_LEN);
      print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
      print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
      break;
    case ND_COND:
      printf("%s-Cond '%s'\n", local_prefix, n->token->str);
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
      printf("%s-Var.Member %s '%s.%s'\n",
             local_prefix,
             type_to_name(n->type->kind),
             n->lhs->token->str,
             n->token->str);
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
  if (!stmt) return;

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
  print_struct(has_function);
  print_globals(has_function || has_struct);

  for (struct function *fn = pr->functions; fn; fn = fn->next) last = fn;

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

void enter_scope() {
  scope_depth++;
}
void leave_scope() {
  scope_depth--;

  while (tags && tags->depth > scope_depth) {
    tags = tags->next;
  }
  while (vars && vars->depth > scope_depth) {
    vars = vars->next;
  }
}

void skip(struct token **ret, struct token *tk, char *op) {
  if (!equal(tk, op)) {
    error_tok(tk, "expected '%s', but op is '%s'", op, tk->str);
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
    error_tok(tk, "'%s'ではありません", op);
  }
  *ret = tk->next;
}

int expect_number(struct token **ret, struct token *tk) {
  if (tk->kind != TK_NUM) {
    error_tok(tk, "数ではありません");
  }
  *ret = tk->next;
  return tk->val;
}

int get_number(struct token *tok) {
  if (tok->kind != TK_NUM) {
    error_tok(tok, "expected an number");
  }
  return tok->val;
}

char *get_ident(struct token *tok) {
  if (tok->kind != TK_IDENT) {
    error_tok(tok, "expected an identifier");
  }

  return strndup(tok->str, tok->len);
}

struct member *get_member(struct type *ty, struct token *tk) {
  // debug("(%s)'s members: ", ty->name);
  // for (struct member *m = ty->members; m; m = m->next) {
  //   debug("  %s", m->name);
  // }
  for (struct member *m = ty->members; m; m = m->next) {
    if (find_cond(m->name, tk)) return m;
  }

  return NULL;
}

struct type *find_typedef(struct token *tk) {

  struct var_scope *vs = find_var(tk);
  if (vs) {
    return vs->type_def;
  }
  return NULL;
}

bool is_typename(struct token *tok) {
  char *keyword[] = {
      "void",
      "bool",
      "char",
      "short",
      "int",
      "long",
      "struct",
      "enum",
      "union",
      "typedef",
      "static",
      "extern",
      "signed",
      "unsigned",
      "const",
  };

  for (int i = 0; i < sizeof(keyword) / sizeof(*keyword); i++) {
    if (equal(tok, keyword[i])) {
      return true;
    }
  }

  return find_typedef(tok);
}

struct function *find_func(char *name) {
  for (struct function *fn = functions; fn; fn = fn->next) {
    if (!fn->name) continue;
    if (strlen(fn->name) == strlen(name) &&
        strncmp(fn->name, name, strlen(name)) == 0) {
      return fn;
    }
  }

  return NULL;
}
struct tag_scope *find_tag(struct token *tk) {
  for (struct tag_scope *t = tags; t; t = t->next) {
    if (find_cond(t->name, tk)) return t;
  }

  return NULL;
}
struct var_scope *find_var(struct token *tk) {
  // debug("find var(%s)", tk->str);
  // for (struct var_scope *v = vars; v; v = v->next) {
  //   debug("  %s", v->name);
  // }
  for (struct var_scope *v = vars; v; v = v->next) {
    if (find_cond(v->name, tk)) return v;
  }

  return NULL;
}
bool is_void_assign_element(struct node *node) {
  if (!node) {
    return false;
  }
  if (node->kind == ND_FUNCALL) {
    struct function *f = find_func(node->token->str);
    if (f->type->kind == TY_VOID)
      return true;
    else
      return false;
  }

  // return is_void_assign_element(node->lhs);
  return is_void_assign_element(node->rhs);
}

struct node *new_node(enum node_kind kind, struct token *token) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = kind;
  n->token = token;
  return n;
}

struct node *new_node_binary(enum node_kind kind,
                             struct node *lhs,
                             struct node *rhs,
                             struct token *token) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = kind;
  n->lhs = lhs;
  n->rhs = rhs;
  n->token = token;
  return n;
}

struct node *new_node_num(int val, struct token *token) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = ND_NUM;
  n->val = val;
  n->type = copy_type(ty_int);
  n->token = token;
  return n;
}
struct node *new_node_ulong(int val, struct token *token) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = ND_NUM;
  n->val = val;
  n->type = copy_type(ty_ulong);
  n->token = token;
  return n;
}
struct node *new_node_var(struct var *var, struct token *token) {
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = ND_VAR;
  n->token = token;
  n->var = var;
  n->type = var->type;
  return n;
}

struct node *new_node_unary(enum node_kind kind,
                            struct node *expr,
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

struct node *new_node_assign(struct node *lhs,
                             struct node *rhs,
                             struct token *token) {
  add_type(lhs);
  add_type(rhs);

  struct node *n = new_node_binary(ND_ASSIGN, lhs, rhs, token);

  return n;
}

struct node *new_node_cast(struct node *expr, struct type *ty) {
  add_type(expr);
  struct node *n = calloc(1, sizeof(struct node));
  n->kind = ND_CAST;
  n->lhs = expr;
  n->token = expr->token;
  n->type = copy_type(ty);
  n->token = expr->token;
  return n;
}

struct init_data *new_init(struct type *type,
                           int len,
                           struct node *expr,
                           struct token *tk) {
  struct init_data *init = calloc(1, sizeof(struct init_data));
  init->type = type;
  init->len = len;
  init->expr = expr;
  init->token = tk;

  if (len) {
    init->child = calloc(len, sizeof(struct init_data *));
  }
  return init;
}

int eval(struct node *node, struct var **var) {
  add_type(node);
  switch (node->kind) {
    case ND_ADD:
      return eval(node->lhs, var) + eval(node->rhs, NULL);
    case ND_SUB:
      return eval(node->lhs, var) - eval(node->rhs, NULL);
    case ND_MUL:
      return eval(node->lhs, NULL) * eval(node->rhs, NULL);
    case ND_DIV:
      return eval(node->lhs, NULL) / eval(node->rhs, NULL);
    case ND_NUM:
      return node->val;
    case ND_VAR:
      *var = node->var;
      return 0;
    case ND_ADDR:
      *var = node->lhs->var;
      return 0;
    case ND_LOGOR:
      return eval(node->lhs, NULL) || eval(node->rhs, NULL);
    case ND_LOGAND:
      return eval(node->lhs, NULL) && eval(node->rhs, NULL);
    case ND_BITOR:
      return eval(node->lhs, NULL) | eval(node->rhs, NULL);
    case ND_BITXOR:
      return eval(node->lhs, NULL) ^ eval(node->rhs, NULL);
    case ND_BITAND:
      return eval(node->lhs, NULL) & eval(node->rhs, NULL);
    case ND_EQ:
      return eval(node->lhs, NULL) == eval(node->rhs, NULL);
    case ND_NE:
      return eval(node->lhs, NULL) != eval(node->rhs, NULL);
    case ND_LT:
      if (node->type->is_unsigned) {
        return (unsigned long) eval(node->lhs, NULL) < eval(node->rhs, NULL);
      }
      return eval(node->lhs, NULL) < eval(node->rhs, NULL);
    case ND_LE:
      if (node->type->is_unsigned) {
        return (unsigned long) eval(node->lhs, NULL) <= eval(node->rhs, NULL);
      }
      return eval(node->lhs, NULL) <= eval(node->rhs, NULL);
    case ND_GT:
      return eval(node->lhs, NULL) > eval(node->rhs, NULL);
    case ND_GE:
      return eval(node->lhs, NULL) >= eval(node->rhs, NULL);
    case ND_COND:
      return eval(node->cond, NULL) ? eval(node->then, NULL)
                                    : eval(node->els, NULL);
    case ND_NOT:
      return !eval(node->lhs, NULL);
    case ND_CAST: {
      long val = eval(node->lhs, var);
      if (!is_integer(node->type) || size_of(node->type) == 8) {
        return val;
      }
      switch (size_of(node->type)) {
        case 1:
          if (node->type->is_unsigned) return (unsigned char) val;
          return (char) val;
        case 2:
          if (node->type->is_unsigned) return (unsigned short) val;
          return (short) val;
        default:
          if ((size_of(node->type)) != 4) {
            error_tok(node->token, "invalid size");
          }
          if (node->type->is_unsigned) return (unsigned int) val;
          return (int) val;
      }
    }
    default:
      break;
  }
  error_tok(node->token, "not a constant expression");
  return 0;
}

struct node *new_add(struct node *lhs,
                     struct node *rhs,
                     struct token *token) {
  add_type(lhs);
  add_type(rhs);

  // num + num
  if (is_integer(lhs->type) && is_integer(rhs->type)) {
    return new_node_binary(ND_ADD, lhs, rhs, token);
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
  rhs = new_node_binary(
      ND_MUL, rhs, new_node_num(size_of(lhs->type->ptr_to), token), token);

  struct node *n = new_node_binary(ND_ADD, lhs, rhs, token);

  return n;
}

struct node *new_sub(struct node *lhs, struct node *rhs, struct token *tk) {
  add_type(lhs);
  add_type(rhs);

  struct node *n;
  // num - num
  if (is_integer(lhs->type) && is_integer(rhs->type)) {
    n = new_node_binary(ND_SUB, lhs, rhs, tk);
    return n;
  }

  // ptr - num
  if (lhs->type->ptr_to && rhs->type->kind == TY_INT) {
    rhs = new_node_binary(
        ND_MUL, rhs, new_node_num(size_of(lhs->type->ptr_to), tk), tk);
    n = new_node_binary(ND_SUB, lhs, rhs, tk);
    return n;
  }

  // ptr - ptr
  if (lhs->type->ptr_to && rhs->type->ptr_to) {
    struct node *sub = new_node_binary(ND_SUB, lhs, rhs, tk);
    n = new_node_binary(
        ND_DIV, sub, new_node_num(size_of(lhs->type->ptr_to), tk), tk);
    return n;
  }

  error_tok(tk, "invalid operands");
  return NULL;
}

struct tag_scope *new_tagscope(struct type *type) {
  struct tag_scope *tag = calloc(1, sizeof(struct tag_scope));
  tag->name = type->name;
  tag->depth = scope_depth;
  tag->next = tags;
  tag->type = type;
  tags = tag;
  return tag;
}

struct var_scope *new_varscope(char *name) {
  struct var_scope *vs = calloc(1, sizeof(struct var_scope));
  vs->name = name;
  vs->next = vars;
  vs->depth = scope_depth;
  vars = vs;
  return vs;
}

struct var *new_lvar(char *name, struct type *type) {
  struct var *v = calloc(1, sizeof(struct var));
  v->name = name;
  v->next = locals;
  v->type = copy_type(type);
  v->is_local = true;
  locals = v;
  new_varscope(name)->var = v;
  return v;
}

struct var *new_gvar(char *name,
                     struct type *type,
                     bool is_static,
                     bool emit) {
  struct var *v = calloc(1, sizeof(struct var));
  v->name = name;
  v->type = copy_type(type);
  v->is_static = is_static;
  v->is_local = false;
  if (emit) {
    v->next = globals;
    globals = v;
  }
  new_varscope(name)->var = v;
  return v;
}
char *gvar_name() {
  char *buf = calloc(24, sizeof(char));
  static int inc = 0;
  snprintf(buf, 24, ".LC%d", inc++);
  return buf;
}

char *static_lvar_name(char *lname) {
  int size = strlen(lname) + 4;
  char *buf = malloc(size);
  static int inc = 0;
  snprintf(buf, size, "%s.%d", lname, inc++);
  return buf;
}

struct var *new_string_literal(char *data, int len) {
  struct type *type = array_to(ty_char, len);
  type->is_string = true;
  char *name = gvar_name();
  struct var *v = new_gvar(name, type, true, true);
  v->name = name;
  v->data = data;
  return v;
}

struct type *pointers(struct token **ret, struct token *tk, struct type *ty) {

  while (consume(&tk, tk, "*")) {
    ty = pointer_to(ty);
    while (equal(tk, "const") || equal(tk, "restrict")) {
      if (equal(tk, "const")) {
        ty->is_const = true;
      }
      tk = tk->next;
    }
  }
  *ret = tk;
  return ty;
}

long const_expr(struct token **ret, struct token *tk) {
  struct node *cond = conditional(ret, tk);
  return eval(cond, NULL);
}

struct type *typespec(struct token **ret,
                      struct token *tk,
                      struct var_attr *attr) {

  enum {
    VOID = 1 << 0,
    BOOL = 1 << 2,
    CHAR = 1 << 4,
    SHORT = 1 << 6,
    INT = 1 << 8,
    LONG = 1 << 10,
    SIGNED = 1 << 12,
    UNSIGNED = 1 << 14,
  };

  struct type *ty = ty_int;
  int type = 0;

  bool is_const = false;

  while (is_typename(tk)) {

    if (consume(&tk, tk, "const")) {
      is_const = true;
      continue;
    }

    if (equal(tk, "typedef") || equal(tk, "extern") || equal(tk, "static")) {
      if (!attr) {
        error_tok(tk, "");
      }

      if (equal(tk, "typedef")) {
        attr->is_typedef = true;
      }
      if (equal(tk, "extern")) {
        attr->is_extern = true;
      }
      if (equal(tk, "static")) {
        attr->is_static = true;
      }

      if (attr->is_extern + attr->is_static + attr->is_typedef > 1) {
        error_tok(tk, "extern, static and typedef may not be used together");
      }
      tk = tk->next;
      continue;
    }

    if (equal(tk, "struct")) {
      ty = struct_declarator(&tk, tk->next, TY_STRUCT);
      continue;
    }

    if (equal(tk, "union")) {
      ty = union_declarator(&tk, tk->next, TY_UNION);
      continue;
    }

    if (equal(tk, "enum")) {
      ty = enum_declarator(&tk, tk->next);
      continue;
    }

    struct type *ty2 = find_typedef(tk);
    if (ty2) {
      ty = copy_type(ty2);
      tk = tk->next;
      continue;
    }

    if (equal(tk, "void")) {
      type += VOID;
    } else if (equal(tk, "bool")) {
      type += BOOL;
    } else if (equal(tk, "char")) {
      type += CHAR;
    } else if (equal(tk, "short")) {
      type += SHORT;
    } else if (equal(tk, "int")) {
      type += INT;
    } else if (equal(tk, "long")) {
      type += LONG;
    } else if (equal(tk, "signed")) {
      type |= SIGNED;
    } else if (equal(tk, "unsigned")) {
      type |= UNSIGNED;
    } else {
      error_tok(tk, "internal error");
    }

    switch (type) {
      case VOID:
        ty = ty_void;
        break;
      case BOOL:
        ty = ty_bool;
        break;
      case CHAR:
      case SIGNED + CHAR:
        ty = ty_char;
        break;
      case UNSIGNED + CHAR:
        ty = ty_uchar;
        break;
      case SHORT:
      case SHORT + INT:
      case SIGNED + SHORT:
      case SIGNED + SHORT + INT:
        ty = ty_short;
        break;
      case UNSIGNED + SHORT:
      case UNSIGNED + SHORT + INT:
        ty = ty_ushort;
        break;
      case INT:
      case SIGNED:
      case SIGNED + INT:
        ty = ty_int;
        break;
      case UNSIGNED:
      case UNSIGNED + INT:
        ty = ty_uint;
        break;
      case LONG:
      case LONG + INT:
      case LONG + LONG:
      case LONG + LONG + INT:
      case SIGNED + LONG:
      case SIGNED + LONG + INT:
      case SIGNED + LONG + LONG:
      case SIGNED + LONG + LONG + INT:
        ty = ty_long;
        break;
      case UNSIGNED + LONG:
      case UNSIGNED + LONG + INT:
      case UNSIGNED + LONG + LONG:
      case UNSIGNED + LONG + LONG + INT:
        ty = ty_ulong;
        break;
      default:
        error_tok(tk, "invalid type");
    }
    tk = tk->next;
  }

  ty = copy_type(ty);
  if (is_const) {
    ty->is_const = is_const;
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
    struct var_attr attr = {};
    struct member *m = calloc(1, sizeof(struct member));

    struct type *base_type = typespec(&tk, tk, &attr);
    m->type = declarator(&tk, tk, base_type);
    m->name = m->type->name;
    m->align = m->type->align;

    cur = cur->next = m;

    skip(&tk, tk, ";");
  }

  *ret = tk->next;
  return head.next;
}

struct type *struct_union_declarator(struct token **ret,
                                     struct token *tk,
                                     enum type_kind kind) {
  struct token *tag = NULL;
  if (tk->kind == TK_IDENT) {
    tag = tk;
    tk = tk->next;
  }

  struct type *ty = copy_type(ty_struct);
  if (tag && !equal(tk, "{")) {

    *ret = tk;
    struct tag_scope *t = find_tag(tag);
    if (t) {
      return t->type;
    }

    if (kind == TY_UNION) ty = copy_type(ty_union);
    ty->token = tk;
    ty->is_incomplete = true;
    new_tagscope(ty);

    return ty;
  }

  if (kind == TY_UNION) ty = copy_type(ty_union);
  ty->token = tk;
  ty->members = struct_members(&tk, tk);

  if (tag) {
    ty->name = tag->str;
    ty->tag = tag->str;
    new_tagscope(ty);
  } else {
    ty->name = "";
  }

  *ret = tk;
  return ty;
}

struct type *struct_declarator(struct token **ret,
                               struct token *tk,
                               enum type_kind kind) {

  struct type *ty = struct_union_declarator(ret, tk, kind);

  if (ty->is_incomplete) return ty;
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
    offset += size_of(m->type);

    if (ty->align < m->align) ty->align = m->align;

    ty->size = offset + size_of(m->type);
  }

  if (size_of(ty) % ty->align != 0) {
    ty->size = offset + (ty->align - (offset % ty->align));
  }

  return ty;
}

struct type *union_declarator(struct token **ret,
                              struct token *tk,
                              enum type_kind kind) {
  struct type *ty = struct_union_declarator(ret, tk, kind);

  if (ty->is_incomplete) return ty;
  // align and offset calculation
  // アライメント
  // 各メンバーのオフセットを0にする
  int max_align = 0;
  for (struct member *m = ty->members; m; m = m->next) {
    if (ty->align < m->align) ty->align = m->align;
    if (ty->size < size_of(m->type)) ty->size = size_of(m->type);
  }
  return ty;
}

struct type *enum_declarator(struct token **ret, struct token *tk) {

  struct token *tag = consume_ident(&tk, tk);

  if (tag && !equal(tk, "{")) {
    *ret = tk;
    struct tag_scope *ts = find_tag(tk);
    if (!ts) {
      error_tok(tk, "unknown enum");
    }
    if (ts->type->kind != TY_ENUM) {
      error_tok(tk, "not an enum tag");
    }
    return ts->type;
  }

  struct type *ty = copy_type(ty_enum);
  if (tag) {
    ty->name = tag->str;
    new_tagscope(ty);
  }

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

struct type *typedef_declarator(struct token **ret,
                                struct token *tk,
                                struct type *type) {
  struct var_attr attr = {};

  struct type *ty = copy_type(type);

  while (true) {

    new_varscope(ty->name)->type_def = ty;

    if (consume(&tk, tk, ";")) break;

    ty = declarator(&tk, tk, type);
  }
  *ret = tk;
  return ty;
}

struct type *typename(struct token **ret, struct token *tk) {
  struct type *ty = typespec(&tk, tk, NULL);
  return pointers(ret, tk, ty);
}

struct function *funcdef(struct token **ret,
                         struct token *tk,
                         struct type *type,
                         struct var_attr *attr) {
  locals = NULL;
  struct function *fn = calloc(1, sizeof(struct function));

  fn->name = type->name;
  fn->type = type->return_type;
  fn->token = tk;
  fn->is_static = attr->is_static;

  enter_scope();

  int cnt0 = 0;
  int cnt1 = 0;

  for (struct type *ty = current_fn->type->params; ty; ty = ty->next) cnt0++;
  for (struct type *ty = type->params; ty; ty = ty->next) cnt1++;

  if (0 < cnt0 && cnt0 != cnt1) {
    error_tok(tk, "different number of parameters");
  }

  struct type *ty = type->params;
  for (; ty; ty = ty->next) {
    new_lvar(ty->name, ty);
  }
  fn->params = locals;

  if (equal(tk, "{")) {
    fn->type->is_incomplete = false;
    fn->stmt = compound_stmt(ret, tk)->body;
    fn->locals = locals;
  }
  leave_scope();

  return fn;
}

struct type *declarator(struct token **ret,
                        struct token *tk,
                        struct type *base) {
  struct type *type = base;

  type = pointers(&tk, tk, type);
  // ident
  // if (tk->kind != TK_IDENT) {
  //   error_tok(tk->loc, "expected a variable name");
  // }

  char *type_name = "";
  struct token *tk_name = tk;

  if (tk->kind == TK_IDENT) {
    type_name = get_ident(tk);
    tk_name = tk;
    tk = tk->next;
  }

  type = type_suffix(&tk, tk, type);
  type->name = type_name;
  type->token = tk_name;

  *ret = tk;
  return type;
}

struct type *array_size(struct token **ret,
                        struct token *tk,
                        struct type *type) {
  int size = 0;
  bool is_incomplete = true;
  if (!equal(tk, "]")) {
    size = const_expr(&tk, tk);
    is_incomplete = false;
  }
  skip(&tk, tk, "]");
  type = type_suffix(&tk, tk, type);
  type = array_to(type, size);
  type->is_incomplete = is_incomplete;

  *ret = tk;
  return type;
}

struct type *type_suffix(struct token **ret,
                         struct token *tk,
                         struct type *type) {
  if (consume(&tk, tk, "(")) {
    return funcdef_args(ret, tk, type);
  }

  if (consume(&tk, tk, "[")) {
    return array_size(ret, tk, type);
  }

  *ret = tk;
  return type;
}

struct type *funcdef_args(struct token **ret,
                          struct token *tk,
                          struct type *type) {

  struct type head = {};
  struct type *cur = &head;
  while (!equal(tk, ")")) {
    if (cur != &head) {
      skip(&tk, tk, ",");
    }

    struct type *ty;
    ty = typespec(&tk, tk, NULL);
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
  struct var_attr attr = {};

  struct type *base = typespec(&tk, tk, &attr);

  struct node head = {};
  struct node *cur = &head;
  int count = 0;
  while (!equal(tk, ";")) {
    if (0 < count++) {
      skip(&tk, tk, ",");
    }

    struct type *type = declarator(&tk, tk, base);

    if (attr.is_static) {

      var = new_gvar(static_lvar_name(type->name), type, true, true);
      new_varscope(var->type->name)->var = var;
      if (consume(&tk, tk, "=")) {
        gvar_initializer(&tk, tk, var);
      }
      continue;
    }

    var = new_lvar(type->name, type);

    if (consume(&tk, tk, "=")) {
      cur = cur->next = lvar_initializer(&tk, tk, var);
      // } else {
      // cur = cur->next = new_node_var(var, type->token);
    }
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
    n = new_node_assign(n, assign(&tk, tk), start);
    if (is_void_assign_element(n->rhs))
      error_tok(start, "Can't Assign to void function");
  }

  if (consume(&tk, tk, "+=")) {
    *ret = tk->next;
    return new_node_assign(
        n, new_add(n, new_node_num(get_number(tk), tk), tk), start);
  }

  if (consume(&tk, tk, "-=")) {
    *ret = tk->next;
    return new_node_assign(
        n, new_sub(n, new_node_num(get_number(tk), tk), tk), start);
  }

  if (consume(&tk, tk, "*=")) {
    *ret = tk->next;
    return new_node_assign(
        n,
        new_node_binary(ND_MUL, n, new_node_num(get_number(tk), tk), tk),
        start);
  }

  if (consume(&tk, tk, "/=")) {
    *ret = tk->next;
    return new_node_assign(
        n,
        new_node_binary(ND_DIV, n, new_node_num(get_number(tk), tk), tk),
        start);
  }

  if (consume(&tk, tk, "++")) {
    *ret = tk;
    return new_node_assign(n, new_add(n, new_node_num(1, tk), tk), start);
  }

  if (consume(&tk, tk, "--")) {
    *ret = tk;
    return new_node_assign(n, new_sub(n, new_node_num(1, tk), tk), start);
  }

  *ret = tk;
  return n;
}

struct node *conditional(struct token **ret, struct token *tk) {
  struct node *node = logor(&tk, tk);

  struct token *start = tk;
  if (consume(&tk, tk, "?")) {
    struct node *cond = new_node(ND_COND, start);

    cond->token = start;
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
    node = new_node_binary(ND_LOGOR, node, logand(&tk, tk->next), tk);
  }

  *ret = tk;
  return node;
}

struct node *logand(struct token **ret, struct token *tk) {
  struct node *node = bitor (&tk, tk);

  while (equal(tk, "&&")) {
    node = new_node_binary(ND_LOGAND, node, bitor (&tk, tk->next), tk);
  }

  *ret = tk;
  return node;
}

struct node * bitor (struct token * *ret, struct token *tk) {
  struct node *node = bitxor(&tk, tk);

  while (equal(tk, "|")) {
    node = new_node_binary(ND_BITOR, node, bitxor(&tk, tk->next), tk);
  }

  *ret = tk;
  return node;
}

struct node *bitxor(struct token **ret, struct token *tk) {
  struct node *node = bitand(&tk, tk);

  while (equal(tk, "^")) {
    node = new_node_binary(ND_BITXOR, node, bitand(&tk, tk->next), tk);
  }

  *ret = tk;
  return node;
}

struct node *bitand(struct token **ret, struct token *tk) {
  struct node *node = equality(&tk, tk);

  while (equal(tk, "&")) {
    node = new_node_binary(ND_BITAND, node, equality(&tk, tk->next), tk);
  }

  *ret = tk;
  return node;
}
struct node *compound_stmt(struct token **ret, struct token *tk) {
  struct node *n = NULL;

  if (!equal(tk, "{")) return NULL;

  n = new_node(ND_BLOCK, tk);

  skip(&tk, tk, "{");

  struct node head = {};
  struct node *cur = &head;

  enter_scope();

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

  leave_scope();

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

    if (current_fn->type->kind != TY_VOID) {
      n->lhs = expr(&tk, tk->next);
    } else {
      if (!equal(tk->next, ";"))
        error_tok(tk,
                  "Void function '%s' should not return a value",
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

    enter_scope();
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

    leave_scope();
    *ret = tk;
    return n;
  }

  if (equal(tk, ";")) {
    n = new_node(ND_BLOCK, tk);
    *ret = tk->next;
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
    struct token *start = tk;
    if (consume(&tk, tk, "==")) {
      n = new_node_binary(ND_EQ, n, relational(&tk, tk), start);
    } else if (consume(&tk, tk, "!=")) {
      n = new_node_binary(ND_NE, n, relational(&tk, tk), start);
    } else {

      *ret = tk;
      return n;
    }
  }
}

struct node *relational(struct token **ret, struct token *tk) {
  struct node *n = shift(&tk, tk);

  while (true) {

    struct token *start = tk;
    if (consume(&tk, tk, "<")) {
      n = new_node_binary(ND_LT, n, shift(&tk, tk), start);
    } else if (consume(&tk, tk, "<=")) {
      n = new_node_binary(ND_LE, n, shift(&tk, tk), start);
    } else if (consume(&tk, tk, ">")) {
      n = new_node_binary(ND_GT, n, shift(&tk, tk), start);
    } else if (consume(&tk, tk, ">=")) {
      n = new_node_binary(ND_GE, n, shift(&tk, tk), start);
    } else {
      *ret = tk;
      return n;
    }
  }
}

struct node *shift(struct token **ret, struct token *tk) {
  struct node *n = add(&tk, tk);

  while (true) {
    struct token *start = tk;
    if (consume(&tk, tk, "<<")) {
      n = new_node_binary(ND_SHL, n, add(&tk, tk), start);
    } else if (consume(&tk, tk, ">>")) {
      n = new_node_binary(ND_SHR, n, add(&tk, tk), start);
    } else {
      *ret = tk;
      return n;
    }
  }
}
struct node *add(struct token **ret, struct token *tk) {
  struct node *n = mul(&tk, tk);
  while (true) {
    struct token *start = tk;
    if (consume(&tk, tk, "+")) {
      n = new_add(n, mul(&tk, tk), start);
    } else if (consume(&tk, tk, "-")) {
      n = new_sub(n, mul(&tk, tk), start);
    } else {
      *ret = tk;
      return n;
    }
  }
}

struct node *mul(struct token **ret, struct token *tk) {
  struct node *n = cast(&tk, tk);
  while (true) {
    struct token *start = tk;
    if (consume(&tk, tk, "*")) {
      n = new_node_binary(ND_MUL, n, cast(&tk, tk), start);
    } else if (consume(&tk, tk, "/")) {
      n = new_node_binary(ND_DIV, n, cast(&tk, tk), start);
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
    return new_node_cast(unary(ret, tk), cast_type);
  }

  return unary(ret, tk);
}

struct node *unary(struct token **ret, struct token *tk) {

  struct token *start = tk;
  if (consume(&tk, tk, "+")) {
    struct node *n = cast(ret, tk);
    return n;
  }
  if (consume(&tk, tk, "-")) {
    struct node *n =
        new_node_binary(ND_SUB, new_node_num(0, start), cast(ret, tk), start);
    return n;
  }

  if (consume(&tk, tk, "*")) {
    struct node *n = new_node_unary(ND_DEREF, cast(ret, tk), start);
    return n;
  }
  if (consume(&tk, tk, "&")) {
    struct node *n = new_node_unary(ND_ADDR, cast(ret, tk), start);
    return n;
  }
  if (consume(&tk, tk, "~")) {
    struct node *n = new_node_unary(ND_BITNOT, cast(ret, tk), start);
    return n;
  }
  if (consume(&tk, tk, "!")) {
    struct node *n = new_node_unary(ND_NOT, cast(ret, tk), start);
    return n;
  }

  // ++a
  // assign a = a + 1
  if (consume(&tk, tk, "++")) {
    struct node *a = unary(ret, tk);
    return new_node_assign(a, new_add(a, new_node_num(1, tk), tk), start);
  }
  // --a
  if (consume(&tk, tk, "--")) {
    struct node *a = unary(ret, tk);
    return new_node_assign(a, new_sub(a, new_node_num(1, tk), tk), start);
  }
  return postfix(ret, tk);
}

struct node *postfix(struct token **ret, struct token *tk) {

  struct token *start = tk;

  struct node *n = primary(&tk, tk);

  while (true) {
    if (consume(&tk, tk, "[")) {
      struct token *tk1 = tk;
      struct node *index = expr(&tk, tk);
      skip(&tk, tk, "]");
      n = new_node_unary(ND_DEREF, new_add(n, index, tk1), start);
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

    struct var_scope *vs = find_var(ident);
    if (vs) {
      *ret = tk;
      if (vs->var) return new_node_var(vs->var, ident);
      if (vs->enum_ty) return new_node_num(vs->enum_val, ident);
    }

    error_tok(ident, "変数%sは定義されていません", ident->str);
  }

  if (equal(tk, "sizeof") && equal(tk->next, "(") &&
      is_typename(tk->next->next)) {
    struct token *start = tk->next->next;
    struct type *ty = typename(&tk, tk->next->next);
    ty->token = start;
    skip(ret, tk, ")");
    return new_node_ulong(size_of(ty), start);
  }

  if (equal(tk, "sizeof")) {
    skip(&tk, tk, "sizeof");
    struct node *node = unary(&tk, tk);
    add_type(node);
    *ret = tk;
    return new_node_num(size_of(node->type), tk);
  }

  if (tk->kind == TK_STR) {
    struct var *v = new_string_literal(tk->str_literal, tk->str_len);
    tk = tk->next;

    while (tk->kind == TK_STR) {
      struct type *ty = v->type;
      int old_size = size_of(ty) - 1;
      int str_len = strlen(tk->str_literal);
      ty->size += str_len;
      ty->array_size += str_len;

      char *data = malloc(size_of(ty) * sizeof(char));
      strncpy(data, v->data, old_size);
      strncpy(&data[old_size], tk->str_literal, tk->str_len - 1);
      v->data = data;
      tk = tk->next;
    }

    *ret = tk;
    return new_node_var(v, tk);
  }

  return new_node_num(expect_number(ret, tk), tk);
}

struct node *funcall(struct token **ret,
                     struct token *tk,
                     struct token *ident) {

  skip(&tk, tk, "(");

  struct node head = {};
  struct node *cur = &head;

  struct node *funcall = new_node(ND_FUNCALL, ident);

  struct var_scope *scope = find_var(ident);
  struct var *param = NULL;
  struct node *node = NULL;

  if (scope) {
    struct var *v = scope->var;
    struct type *ty = v->type;
    for (struct type *p = ty->params; p; p = p->next) {
      struct var *v0 = calloc(1, sizeof(struct var));
      v0->type = p;
      v0->name = p->name;
      v0->next = param;
      param = v0;
    }
    funcall->func_ty = ty->return_type;

  } else {
    funcall->func_ty = ty_void;
  }

  int nargs = 0;

  while (!equal(tk, ")")) {
    if (nargs) skip(&tk, tk, ",");

    struct token *start = tk;
    struct node *arg = assign(&tk, tk);
    add_type(arg);

    if (param) {
      arg = new_node_cast(arg, param->type);
      param = param->next;
    }

    struct var *v = arg->type->ptr_to
                        ? new_lvar("", pointer_to(arg->type->ptr_to))
                        : new_lvar("", arg->type);

    if (param) {
      v->name = param->name;
      v->type->name = param->name;
    }

    funcall->args[nargs++] = v;

    cur = cur->next = new_node_assign(new_node_var(v, start), arg, start);
    add_type(cur);
  }
  skip(&tk, tk, ")");

  funcall->nargs = nargs;
  funcall->token = ident;
  funcall->body = head.next;
  funcall->type = funcall->func_ty;

  *ret = tk;
  return funcall;
}

struct init_data *string_initializer(struct token **ret,
                                     struct token *tk,
                                     struct type *type) {
  if (type->is_incomplete) {
    type->size = tk->str_len;
    type->array_size = tk->str_len;
    type->is_incomplete = false;
  }

  struct init_data *init = new_init(type, type->array_size, NULL, tk);

  type->is_string = true;

  for (int i = 0; i < tk->str_len; i++) {
    init->child[i] =
        new_init(type->ptr_to, 0, new_node_num(tk->str_literal[i], tk), tk);
  }

  *ret = tk->next;
  return init;
}

bool is_end(struct token *tk) {
  return equal(tk, "}") || (equal(tk, ",") && equal(tk->next, "}"));
}
struct init_data *array_initializer(struct token **ret,
                                    struct token *tk,
                                    struct type *type) {

  consume(&tk, tk, "{");

  if (type->is_incomplete) {
    int array_size = 0;
    for (struct token *tk1 = tk; !is_end(tk1); array_size++) {
      if (array_size > 0) skip(&tk1, tk1, ",");
      initializer(&tk1, tk1, type->ptr_to);
    }
    type->size = size_of(type->ptr_to) * array_size;
    type->array_size = array_size;

    type->is_incomplete = false;
  }

  struct init_data *init = new_init(type, type->array_size, NULL, tk);

  for (int i = 0; i < init->len && !is_end(tk); i++) {
    if (0 < i) skip(&tk, tk, ",");
    init->child[i] = initializer(&tk, tk, type->ptr_to);
  }

  consume_end(&tk, tk);

  *ret = tk;

  return init;
}
struct init_data *struct_initializer(struct token **ret,
                                     struct token *tk,
                                     struct type *type) {

  if (!equal(tk, "{")) {
    struct token *tk0 = tk;
    struct node *n = assign(&tk0, tk0);

    if (n->type->kind == TY_STRUCT || n->type->kind == TY_UNION) {
      struct init_data *init = new_init(type, 0, n, tk);
      *ret = tk0;
      return init;
    }
  }

  int member_num = 0;

  for (struct member *m = type->members; m; m = m->next) member_num++;

  struct init_data *init = new_init(type, member_num, NULL, tk);

  consume(&tk, tk, "{");
  int i = 0;
  for (struct member *m = type->members; m && !is_end(tk); m = m->next, i++) {
    if (0 < i) skip(&tk, tk, ",");

    init->child[i] = initializer(&tk, tk, m->type);
  }

  consume_end(&tk, tk);
  *ret = tk;
  return init;
}

struct init_data *initializer(struct token **ret,
                              struct token *tk,
                              struct type *type) {

  if (type->kind == TY_ARRAY && type->ptr_to->kind == TY_CHAR &&
      tk->kind == TK_STR) {
    return string_initializer(ret, tk, type);
  }
  if (type->kind == TY_ARRAY) {
    return array_initializer(ret, tk, type);
  }

  if (type->kind == TY_STRUCT || type->kind == TY_UNION) {
    return struct_initializer(ret, tk, type);
  }

  struct token *start = tk;
  bool has_parent = consume(&tk, tk, "{");
  struct init_data *init = new_init(type, 0, assign(&tk, tk), start);

  if (has_parent) {
    consume_end(&tk, tk);
  }

  *ret = tk;
  return init;
}

struct node *create_lvar_init(struct init_data *init,
                              struct var *var,
                              struct node *lvar,
                              struct type *type,
                              struct token *tk) {

  if (type->kind == TY_ARRAY) {

    struct node head = {};
    struct node *cur = &head;
    struct node *n = new_node(ND_LIST_EXPR, tk);

    for (int i = 0; i < type->array_size; i++) {
      struct init_data *child = init ? init->child[i] : NULL;

      struct node *deref = new_node_unary(
          ND_DEREF, new_add(lvar, new_node_num(i, tk), tk), tk);

      cur = cur->next = create_lvar_init(child, var, deref, type->ptr_to, tk);
    }

    n->body = head.next;
    return n;
  }

  if ((type->kind == TY_STRUCT || type->kind == TY_UNION) &&
      (!init || init->len)) {

    struct node head = {};
    struct node *cur = &head;
    struct node *n = new_node(ND_LIST_EXPR, tk);

    int i = 0;
    for (struct member *m = type->members; m; m = m->next, i++) {
      struct init_data *child = init ? init->child[i] : NULL;

      struct node *mem = new_node_unary(ND_MEMBER, lvar, tk);
      mem->member = m;

      cur = cur->next = create_lvar_init(child, var, mem, m->type, tk);
    }

    n->body = head.next;
    return n;
  }

  struct node *expr = init ? init->expr : new_node_num(0, tk);
  struct node *node = new_node_assign(lvar, expr, tk);
  node->is_init = true;
  return node;
}

struct node *lvar_initializer(struct token **ret,
                              struct token *tk,
                              struct var *var) {

  struct init_data *init = initializer(ret, tk, var->type);

  struct node *nvar = new_node_var(var, var->type->token);
  struct node *node =
      create_lvar_init(init, var, nvar, var->type, nvar->token);

  node = new_node_unary(ND_EXPR_STMT, node, tk);

  return node;
}

void write_data(char *data, long val, int size) {
  switch (size) {
    case 1:
      *(char *) data = val;
      return;
    case 2:
      *(short *) data = val;
      return;
    case 4:
      *(int *) data = val;
      return;
    default:
      if (size != 8) {
        error("invalid value size");
      }
      *(long *) data = val;
      return;
  }
}

struct value *create_gvar_data(struct value *cur,
                               char *data,
                               struct init_data *init,
                               struct type *type,
                               int offset) {

  if (type->kind == TY_ARRAY) {
    int size = size_of(type->ptr_to);
    for (int i = 0; i < type->array_size; i++) {
      struct init_data *child = init->child[i];
      if (child)
        cur = create_gvar_data(
            cur, data, child, type->ptr_to, offset + size * i);
    }
    return cur;
  }

  if (type->kind == TY_STRUCT || type->kind == TY_UNION) {
    int i = 0;
    for (struct member *m = type->members; m; m = m->next, i++) {
      struct init_data *child = init->child[i];
      if (child)
        cur = create_gvar_data(cur, data, child, m->type, offset + m->offset);
    }

    return cur;
  }

  struct var *var = NULL;
  long val = eval(init->expr, &var);

  if (var) {
    struct value *v = calloc(1, sizeof(struct value));
    v->offset = offset;
    v->label = var->name;
    v->addend = val;
    cur->next = v;
    return cur->next;
  }

  write_data(data + offset, val, size_of(type));
  return cur;
}

void gvar_initializer(struct token **ret, struct token *tk, struct var *var) {

  struct init_data *init = initializer(ret, tk, var->type);

  char *data = calloc(1, size_of(var->type));
  struct value head = head;
  create_gvar_data(&head, data, init, var->type, 0);
  var->data = data;
  var->values = head.next;
  return;
}
// program = ( funcdef | declaration ";" )*
//
// typespec = "int" | "char"
//          | struct-declarator
//          | enum-declarator
//          | typedef-declarator
//
// typename = typespec pointers
//
// pointers = ( "*" ( "const" | "restrict" )* )*
//
//
// struct-declarator = "struct" ident "{" sturct-member "}" ";"
// struct-member = (typespec declarator ( "," declarator )* ";" )*
// enum-declarator = "enum" (
//                       | ident? "{" enum-list? "}"
//                       | ident ( "{" enum-list? "}" )?
//                       )
//
// typedef-declarator = "typedef" typespec type-name
//
// enum-list = ident ( "=" num )? ( "," ident ( "=" num )? )* ","?
//
// funcdef = typespec declarator ( ";" | compound-stmt )
// declarator = pointers ident type-suffix
//
// type-suffix = ( "[" num "]" )*
//              | "(" funcdef-args? ")"
//
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
// relational = shift ( "<" shift | "<=" shift | ">" shift | ">=" shift )*
//
// shift = add ( "<<" add | ">>" add )*
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
//         | string-literal*
// funcall = ident "(" funcall-args ")"
// funcall-args = assign ( "," assign )*

struct program *parse(struct token *tk) {
  struct function head = {};
  struct function *cur = &head;
  struct type *type;

  functions = &head;
  while (!at_eof(tk)) {
    struct var_attr attr = {};

    type = typespec(&tk, tk, &attr);
    if (consume(&tk, tk, ";")) continue;

    type = declarator(&tk, tk, type);

    if (attr.is_typedef) {
      type = typedef_declarator(&tk, tk, type);
      continue;
    }

    if (type->return_type) {
      struct var_scope *vs = find_var(type->token);
      if (vs) {
        current_fn = vs->var;
      } else {
        current_fn = new_gvar(type->name, type, attr.is_static, false);
      }

      if (!consume(&tk, tk, ";")) {
        cur = cur->next = funcdef(&tk, tk, type, &attr);
      }
      continue;
    }

    struct var *gvar =
        new_gvar(type->name, type, attr.is_static, !attr.is_extern);
    if (!equal(tk, ";")) {
      while (!equal(tk, ";")) {
        if (consume(&tk, tk, "=")) {
          gvar_initializer(&tk, tk, gvar);
        }
        if (consume(&tk, tk, ",")) {
          type = declarator(&tk, tk, type);
          gvar = new_gvar(type->name, type, attr.is_static, !attr.is_extern);
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
