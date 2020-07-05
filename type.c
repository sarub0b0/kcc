#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct type *ty_void = &(struct type){TY_VOID, 1, 1, ""};
struct type *ty_bool = &(struct type){TY_BOOL, 1, 1, ""};
struct type *ty_char = &(struct type){TY_CHAR, 1, 1, ""};
struct type *ty_short = &(struct type){TY_SHORT, 2, 2, ""};
struct type *ty_int = &(struct type){TY_INT, 4, 4, ""};
struct type *ty_long = &(struct type){TY_LONG, 8, 8, ""};

struct type *ty_schar = &(struct type){TY_CHAR, 1, 1, ""};
struct type *ty_sshort = &(struct type){TY_SHORT, 2, 2, ""};
struct type *ty_sint = &(struct type){TY_INT, 4, 4, ""};
struct type *ty_slong = &(struct type){TY_LONG, 8, 8, ""};

struct type *ty_uchar = &(struct type){TY_CHAR, 1, 1, "", true};
struct type *ty_ushort = &(struct type){TY_SHORT, 2, 2, "", true};
struct type *ty_uint = &(struct type){TY_INT, 4, 4, "", true};
struct type *ty_ulong = &(struct type){TY_LONG, 8, 8, "", true};

struct type *ty_enum = &(struct type){TY_ENUM, 4, 4, ""};
struct type *ty_struct = &(struct type){TY_STRUCT, 0, 1, ""};
struct type *ty_union = &(struct type){TY_UNION, 0, 1, ""};

void print_type(struct type *ty) {
  if (!ty) return;

  fprintf(stderr,
          "type: %s; size: %ld; align: %ld; array_size: %ld\n",
          ty->name,
          ty->size,
          ty->align,
          ty->array_size);
}

char *type_to_name(enum type_kind kind) {
  switch (kind) {
    case TY_INT:
      return "int";
    case TY_CHAR:
      return "char";
    case TY_PTR:
      return "ptr";
    case TY_ARRAY:
      return "array";
    case TY_VOID:
      return "void";
    case TY_SHORT:
      return "short";
    case TY_LONG:
      return "long";
    case TY_STRUCT:
      return "struct";
    case TY_UNION:
      return "union";
    default:
      break;
  }

  return "None";
}
bool is_integer(struct type *type) {
  if (type->kind == TY_INT || type->kind == TY_CHAR ||
      type->kind == TY_SHORT || type->kind == TY_LONG ||
      type->kind == TY_BOOL) {
    return true;
  }
  return false;
}

int size_of(struct type *ty) {
  if (ty->is_incomplete) {
    error_tok(ty->token, "incomplete type");
  }

  return ty->size;
}

struct type *copy_type(struct type *ty) {
  struct type *ret = calloc(1, sizeof(struct type));
  *ret = *ty;
  return ret;
}

struct type *pointer_to(struct type *base) {
  struct type *ty = calloc(1, sizeof(struct type));
  ty->ptr_to = base;
  ty->kind = TY_PTR;
  ty->size = 8;
  ty->align = 8;
  return ty;
}

struct type *array_to(struct type *base, size_t len) {
  struct type *type = calloc(1, sizeof(struct type));
  type->kind = TY_ARRAY;
  type->size = len * base->size;
  type->array_size = len;
  type->ptr_to = base;
  type->align = base->align;
  return type;
}

bool is_scalar(struct type *ty) {
  return is_integer(ty) || ty->ptr_to;
}

void type_convert(struct node **lhs, struct node **rhs) {
  struct type *ty1 = (*lhs)->type;
  struct type *ty2 = (*rhs)->type;
  struct type *ty = NULL;

  if (ty1->ptr_to)
    ty = pointer_to(ty1->ptr_to);
  else if (ty1->size != ty2->size)
    ty = ty1->size > ty2->size ? ty1 : ty2;
  else if (ty2->is_unsigned)
    ty = ty2;
  else
    ty = ty1;

  *lhs = new_node_cast(*lhs, ty);
  *rhs = new_node_cast(*rhs, ty);
}

void add_type(struct node *n) {
  if (!n || n->type) return;

  add_type(n->lhs);
  add_type(n->rhs);

  add_type(n->cond);
  add_type(n->then);
  add_type(n->els);
  add_type(n->init);
  add_type(n->inc);

  for (struct node *nn = n->body; nn; nn = nn->next) {
    add_type(nn);
  }

  switch (n->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_DIV:
    case ND_MUL:
    case ND_BITOR:
    case ND_BITXOR:
    case ND_BITAND:
      type_convert(&n->lhs, &n->rhs);
      n->type = n->lhs->type;
      return;
    case ND_COND:
      type_convert(&n->then, &n->els);
      n->type = n->then->type;
      return;
    case ND_ASSIGN:
      if (is_scalar(n->rhs->type) && n->lhs->type->kind != n->rhs->type->kind)
        n->rhs = new_node_cast(n->rhs, n->lhs->type);
      n->type = n->lhs->type;
      return;
    case ND_BITNOT:
    case ND_SHL:
    case ND_SHR:
      n->type = n->lhs->type;
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_GT:
    case ND_GE:
      type_convert(&n->lhs, &n->rhs);
      n->type = copy_type(ty_int);
      return;
    case ND_NUM:
      if (n->token->type)
        n->type = copy_type(n->token->type);
      else
        n->type = copy_type(ty_int);
    case ND_LOGOR:
    case ND_LOGAND:
    case ND_NOT:
      n->type = copy_type(ty_int);
      return;
    case ND_ADDR:
      if (n->lhs->type->kind == TY_ARRAY) {
        n->type = pointer_to(n->lhs->type->ptr_to);
      } else {
        n->type = pointer_to(n->lhs->type);
      }
      return;
    case ND_DEREF:
      if (!n->lhs->type->ptr_to) {
        error("Invalid pointer deference");
      }
      n->type = n->lhs->type->ptr_to;
      return;
    case ND_VAR:
      n->type = n->var->type;
      return;
    case ND_MEMBER:
      n->type = n->member->type;
      return;
    case ND_COMMA:
      n->type = n->rhs->type;
      return;
    case ND_STMT_EXPR: {
      struct node *stmt = n->body;
      while (stmt->next) stmt = stmt->next;
      n->type = stmt->lhs->type;
      return;
    }
    default:
      return;
  }
}
