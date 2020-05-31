#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct type *ty_short = &(struct type){SHORT, 2, ""};
struct type *ty_int = &(struct type){INT, 4, ""};
struct type *ty_long = &(struct type){LONG, 8, ""};

struct type *ty_char = &(struct type){CHAR, 1, ""};
struct type *ty_void = &(struct type){VOID, 0, ""};

bool is_integer(struct type *type) {
  if (type->kind == INT || type->kind == CHAR || type->kind == SHORT ||
      type->kind == LONG) {
    return true;
  }
  return false;
}

struct type *copy_type(struct type *ty) {
  struct type *ret = calloc(1, sizeof(struct type));
  memcpy(ret, ty, sizeof(struct type));
  return ret;
}

struct type *pointer_to(struct type *base) {
  struct type *ty = calloc(1, sizeof(struct type));
  ty->ptr_to = base;
  ty->kind = PTR;
  ty->size = 8;
  return ty;
}

struct type *array_to(struct type *base, size_t len) {
  struct type *type = calloc(1, sizeof(struct type));
  type->kind = ARRAY;
  type->size = len * base->size;
  type->array_size = len;
  type->ptr_to = base;
  return type;
}

bool is_scalar(struct type *ty) { return is_scalar(ty) || ty->ptr_to; }

void add_type(struct node *n) {
  if (!n || n->type)
    return;

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
  case ND_ASSIGN:
    if (is_scalar(n->rhs->type))
      n->rhs = new_cast(n->rhs, n->lhs->type);
    n->type = n->lhs->type;
    return;
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_GT:
  case ND_GE:
  case ND_NUM:
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
  case ND_STMT_EXPR: {
    struct node *stmt = n->body;
    while (stmt->next)
      stmt = stmt->next;
    n->type = stmt->lhs->type;
    return;
  }
  }
}
