#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct type *ty_short = &(struct type){SHORT, 2, 2, ""};
struct type *ty_int = &(struct type){INT, 4, 4, ""};
struct type *ty_long = &(struct type){LONG, 8, 8, ""};
struct type *ty_bool = &(struct type){BOOL, 1, 1, ""};

struct type *ty_char = &(struct type){CHAR, 1, 1, ""};
struct type *ty_void = &(struct type){VOID, 1, 1, ""};
struct type *ty_enum = &(struct type){ENUM, 4, 4, ""};

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
    case INT:
      return "int";
    case CHAR:
      return "char";
    case PTR:
      return "ptr";
    case ARRAY:
      return "array";
    case VOID:
      return "void";
    case SHORT:
      return "short";
    case LONG:
      return "long";
    case STRUCT:
      return "struct";
  }

  debug("None(%d)", kind);
  return "None";
}
bool is_integer(struct type *type) {
  if (type->kind == INT || type->kind == CHAR || type->kind == SHORT ||
      type->kind == LONG || type->kind == BOOL) {
    return true;
  }
  return false;
}

struct type *copy_type(struct type *ty) {
  struct type *ret = calloc(1, sizeof(struct type));
  *ret = *ty;
  return ret;
}

struct type *pointer_to(struct type *base) {
  struct type *ty = calloc(1, sizeof(struct type));
  ty->ptr_to = base;
  ty->kind = PTR;
  ty->size = 8;
  ty->align = 8;
  return ty;
}

struct type *array_to(struct type *base, size_t len) {
  struct type *type = calloc(1, sizeof(struct type));
  type->kind = ARRAY;
  type->size = len * base->size;
  type->array_size = len;
  type->ptr_to = base;
  type->align = base->align;
  return type;
}

bool is_scalar(struct type *ty) {
  return is_integer(ty) || ty->ptr_to;
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
    case ND_BITAND: {
      struct type *ty1 = n->lhs->type;
      struct type *ty2 = n->rhs->type;
      struct type *ty = NULL;
      if (ty1->ptr_to)
        ty = pointer_to(ty1->ptr_to);
      else
        ty = ty1->size > ty2->size ? ty1 : ty2;

      n->rhs = new_node_cast(n->rhs, ty);
      n->lhs = new_node_cast(n->lhs, ty);
      n->type = ty;
      return;
    }
    case ND_COND:
      n->type = n->then->type;
      return;
    case ND_ASSIGN:
      if (is_scalar(n->rhs->type) && n->lhs->type->kind != n->rhs->type->kind)
        n->rhs = new_node_cast(n->rhs, n->lhs->type);
      n->type = n->lhs->type;
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_GT:
    case ND_GE:
    case ND_NUM:
    case ND_LOGOR:
    case ND_LOGAND:
      n->type = ty_int;
      return;
    case ND_ADDR:
      if (n->lhs->type->kind == ARRAY) {
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
  }
}
