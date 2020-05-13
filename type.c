#include <stdlib.h>

#include "kcc.h"

struct type ty_int_ = (struct type){INT};
struct type *ty_int = &ty_int_;

struct type *pointer_to(struct type *base) {
  struct type *ty = calloc(1, sizeof(struct type));
  ty->ptr_to = base;
  ty->kind = PTR;
  return ty;
}

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
    n->type = pointer_to(n->lhs->type);
    return;
  case ND_DEREF:
    if (n->lhs->type->kind == PTR) {
      n->type = n->lhs->type->ptr_to;
    } else {
      n->type = ty_int;
    }
    return;
  case ND_FUNCALL:

    return;
  }
}

