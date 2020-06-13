#include <assert.h>
#include <stdio.h>

#include "kcc.h"

const char *argreg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
const char *argreg16[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
const char *argreg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
const char *argreg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

const char *reg8[] = {"r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};
const char *reg16[] = {"r10w", "r11w", "r12w", "r13w", "r14w", "r15w"};
const char *reg32[] = {"r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
const char *reg64[] = {"r10", "r11", "r12", "r13", "r14", "r15"};

int inc = 0;
struct function *current_fn;

int label_seq = 0;

int gen_expr(struct node *);
void gen_stmt(struct node *);

size_t type_size(struct type *ty) {
  if (ty->kind == ARRAY)
    return type_size(ty->ptr_to);

  return ty->size;
}

char *type_kind_name(enum type_kind kind) {
  switch (kind) {
  case INT:
    return "INT";
  case CHAR:
    return "CHAR";
  case PTR:
    return "PTR";
  case ARRAY:
    return "ARRAY";
  case VOID:
    return "VOID";
  }
  return "";
}

const char *areg(struct type *ty) {
  const char *r[] = {"al", "ax", "eax", "rax"};

  if (ty->kind == ARRAY)
    return areg(ty->ptr_to);

  switch (ty->size) {
  case 1:
    return r[0];
  case 2:
    return r[1];
  case 4:
    return r[2];
  default:
    return r[3];
  }
}

const char *argreg(struct type *ty, int i) {
  if (ty->kind == ARRAY)
    return argreg(ty->ptr_to, i);

  switch (ty->size) {
  case 1:
    return argreg8[i];
  case 2:
    return argreg16[i];
  case 4:
    return argreg32[i];
  default:
    return argreg64[i];
  }
}

const char *reg(struct type *ty, int i) {
  if (ty->kind == ARRAY)
    return reg(ty->ptr_to, i);

  switch (ty->size) {
  case 1:
    return reg8[i];
  case 2:
    return reg16[i];
  case 4:
    return reg32[i];
  default:
    return reg64[i];
  }
}

const char *size_name(struct type *ty) {
  switch (ty->size) {
  case 1:
    return "BYTE";
  case 2:
    return "WORD";
  case 4:
    return "DWORD";
  default:
    return "QWORD";
  }
}

void gen_addr(struct node *n) {
  switch (n->kind) {
  case ND_VAR:
    if (n->var->is_local) {
      printf("  lea %s, [rbp-%d]\n", reg64[inc++], n->var->offset);
    } else {
      printf("  mov %s, offset %s\n", reg64[inc++], n->var->name);
    }
    return;
  case ND_DEREF:
    gen_expr(n->lhs);
    return;
  }
}

void gen_if(struct node *node) {
  // if (expr) stmt (else stmt);
  //
  // if (expr) stmt;
  //   n->condをコンパイル
  //   pop rax
  //   cmp rax, 0
  //   je  .LendXXX
  //   n->thenをコンパイル
  // .LendXXX
  //
  // if (expr) stmt else stmt;
  //   n->condをコンパイル
  //   pop rax
  //   cmp rax, 0
  //   je  .LendXXX
  //   n->thenをコンパイル
  //   jmp .LendXXX
  // .LelseXXX
  //   n->elsをコンパイル
  // .LendXXX

  gen_expr(node->cond);
  printf("  cmp %s, 0\n", reg64[inc - 1]);
  inc--;

  if (node->els) {
    printf("  je  .L.else.%03d\n", label_seq);

    gen_stmt(node->then);

    printf("  jmp  .L.end.%03d\n", label_seq);
    printf(".L.else.%03d:\n", label_seq);

    gen_stmt(node->els);

    printf(".L.end.%03d:\n", label_seq);
  } else {

    printf("  je  .L.end.%03d\n", label_seq);

    gen_stmt(node->then);

    printf(".L.end.%03d:\n", label_seq);
  }
  label_seq++;
}

void gen_for(struct node *node) {
  // while (A) B -> for (;B;) D
  // for (A; B; C) D
  //
  //   cond->initをコンパイル
  // .L.begin.XXX:
  //   Bをコンパイル
  //   pop rax
  //   cmp rax, 0
  //   je  .L.end.XXX
  //   Dをコンパイル
  //   Cをコンパイル
  //   jmp .L.begin.XXX
  // .L.end.XXX

  if (node->init) {
    gen_stmt(node->init);
  }
  printf(".L.begin.%03d:\n", label_seq);

  if (node->cond) {
    gen_expr(node->cond);
    inc--;
    printf("  cmp %s, 0\n", reg64[inc]);
    printf("  je  .L.end.%03d\n", label_seq);
  }

  if (node->then) {
    gen_stmt(node->then);
  }

  if (node->inc) {
    gen_stmt(node->inc);
  }
  printf("  jmp .L.begin.%03d\n", label_seq);
  printf(".L.end.%03d:\n", label_seq);

  label_seq++;
}

void gen_block(struct node *node) {
  for (struct node *n = node->body; n; n = n->next)
    gen_stmt(n);
}

void gen_func(struct node *node) {
  int nargs = 0;
  struct type *fn_ty = node->func_ty;
  for (struct node *n = node->body; n; n = n->next) {
    gen_expr(n);
    nargs++;
  }

  int i = nargs - 1;
  for (struct var *arg = node->args; arg; arg = arg->next) {
    int size = type_size(arg->type);
    struct type *arg_ty = arg->type;

    if (arg_ty->kind == ARRAY)
      size = 8;

    switch (size) {
    case 1:
      printf("  mov %s, %s\n", argreg8[i], reg8[--inc]);
      break;
    case 2:
      printf("  mov %s, %s\n", argreg16[i], reg16[--inc]);
      break;
    case 4:
      printf("  mov %s, %s\n", argreg32[i], reg32[--inc]);
      break;
    case 8:
      printf("  mov %s, %s\n", argreg64[i], reg64[--inc]);
      break;
    }
    i--;
  }

  printf("  push r10\n");
  printf("  push r11\n");
  printf("  mov %s, 0\n", areg(fn_ty));
  printf("  call %s\n", node->str);

  printf("  pop r11\n");
  printf("  pop r10\n");

  if (fn_ty->kind != VOID)
    printf("  mov %s, %s\n", reg(fn_ty, inc++), areg(fn_ty));
  else
    inc++;

  return;
}

void load(struct type *type) {
  printf("//load\n");
  if (type->kind == ARRAY) {
    return;
  }
  const char *r = reg(type, inc - 1);
  const char *s = size_name(type);

  printf("  mov %s, %s PTR [%s]\n", r, s, reg64[inc - 1]);
  switch (type->size) {
  case 1:
  case 2:
    printf("  movsx %s, %s\n", reg64[inc - 1], r);
    break;
  }
}

void store(struct type *type) {
  printf("  mov [%s], %s\n", reg64[inc - 1], reg64[inc - 2]);
  inc--;
}

int gen_expr(struct node *node) {
  if (!node) {
    return 0;
  }

  struct type *ty = node->type;
  switch (node->kind) {
  case ND_NUM:
    printf("  mov %s, %d\n", reg(ty, inc++), node->val);
    return 0;
  case ND_VAR:
    gen_addr(node);
    load(node->type);
    return 0;
  case ND_ASSIGN:
    gen_expr(node->rhs);
    gen_addr(node->lhs);
    store(node->type);
    return 0;
  case ND_FUNCALL:
    gen_expr(node->lhs);
    gen_func(node);
    return 0;
  case ND_ADDR:
    gen_addr(node->lhs);
    return 0;
  case ND_DEREF:
    gen_expr(node->lhs);
    load(node->type);
    return 0;
  case ND_BITNOT:
    gen_expr(node->lhs);
    printf("  not %s\n", reg(node->lhs->type, inc - 1));
    return 0;
  case ND_LOGOR:
    // lhs || rhs
    // lhs == 0 ?
    gen_expr(node->lhs);
    printf("  cmp %s, 0\n", reg(node->lhs->type, --inc));
    printf("  jne .L.true.%03d\n", label_seq);

    // rhs == 0 ?
    gen_expr(node->rhs);
    printf("  cmp %s, 0\n", reg(node->rhs->type, --inc));
    printf("  jne .L.true.%03d\n", label_seq);
    printf("  mov %s, 0\n", reg64[inc - 1]);
    printf("  jmp .L.end.%03d\n", label_seq);
    printf(".L.true.%03d:\n", label_seq);
    printf("  mov %s, 1\n", reg64[inc++]);
    printf(".L.end.%03d:\n", label_seq);
    label_seq++;
    return 0;
  case ND_LOGAND:
    // lhs && rhs
    // lhs == 0 ?
    gen_expr(node->lhs);
    printf("  cmp %s, 0\n", reg(node->lhs->type, --inc));
    printf("  je .L.false.%03d\n", label_seq);

    // rhs == 0 ?
    gen_expr(node->rhs);
    printf("  cmp %s, 0\n", reg(node->rhs->type, --inc));
    printf("  je .L.false.%03d\n", label_seq);
    printf("  mov %s, 1\n", reg64[inc - 1]);
    printf("  jmp .L.end.%03d\n", label_seq);
    printf(".L.false.%03d:\n", label_seq);
    printf("  mov %s, 0\n", reg64[inc++]);
    printf(".L.end.%03d:\n", label_seq);
    label_seq++;
    return 0;
  case ND_STMT_EXPR:
    for (struct node *n = node->body; n; n = n->next) {
      gen_stmt(n);
    }
    inc++;
    return 0;
  case ND_CAST: {
    gen_expr(node->lhs);

    struct type *from = node->lhs->type;
    struct type *to = node->type;

    if (to->size == 1) {
      printf("  movsx %s, %s\n", reg64[inc - 1], reg(from, inc - 1));
    } else if (to->size == 2) {
      printf("  movsx %s, %s\n", reg64[inc - 1], reg(from, inc - 1));
    } else if (to->size == 4) {
      // printf("  mov %s, %s\n", reg32[inc - 1], reg32[inc - 1]);
    } else if (is_integer(from) && from->size < 8) {
      printf("  movsx %s, %s\n", reg64[inc - 1], reg(from, inc - 1));
    }

    return 0;
  }
  case ND_COND:
    gen_expr(node->cond);
    printf("  cmp %s, 0\n", reg64[inc - 1]);
    inc--;

    printf("  je  .L.else.%03d\n", label_seq);

    gen_expr(node->then);
    inc--;

    printf("  jmp  .L.end.%03d\n", label_seq);
    printf(".L.else.%03d:\n", label_seq);

    gen_expr(node->els);

    printf(".L.end.%03d:\n", label_seq);
    label_seq++;

    return 0;
  }

  gen_expr(node->lhs);
  gen_expr(node->rhs);

  const char *rs = reg(node->lhs->type, inc - 1);
  const char *rd = reg(node->lhs->type, inc - 2);
  const char *ra = areg(node->lhs->type);
  inc--;

  switch (node->kind) {
  case ND_ADD:
    printf("  add %s, %s\n", rd, rs);
    break;
  case ND_SUB:
    printf("  sub %s, %s\n", rd, rs);
    break;
  case ND_MUL:
    printf("  imul %s, %s\n", rd, rs);
    break;
  case ND_DIV:
    printf("  mov %s, %s\n", ra, rd);
    printf("  cqo\n");
    printf("  idiv %s\n", rs);
    printf("  mov %s, %s\n", rd, ra);
    break;
  case ND_EQ:
    printf("  cmp %s, %s\n", rd, rs);
    printf("  sete al\n");
    printf("  movzb %s, al\n", rd);
    break;
  case ND_NE:
    printf("  cmp %s, %s\n", rd, rs);
    printf("  setne al\n");
    printf("  movzb %s, al\n", rd);
    break;
  case ND_LE:
    printf("  cmp %s, %s\n", rd, rs);
    printf("  setle al\n");
    printf("  movzb %s, al\n", rd);
    break;
  case ND_LT:
    printf("  cmp %s, %s\n", rd, rs);
    printf("  setl al\n");
    printf("  movzb %s, al\n", rd);
    break;
  case ND_GE:
    printf("  cmp %s, %s\n", rd, rs);
    printf("  setge al\n");
    printf("  movzb %s, al\n", rd);
    break;
  case ND_GT:
    printf("  cmp %s, %s\n", rd, rs);
    printf("  setg al\n");
    printf("  movzb %s, al\n", rd);
    break;
  case ND_BITOR:
    printf("  or %s, %s\n", rd, rs);
    break;
  case ND_BITXOR:
    printf("  xor %s, %s\n", rd, rs);
    break;
  case ND_BITAND:
    printf("  and %s, %s\n", rd, rs);
    break;
  default:
    error("Don't assembly");
    break;
  }

  return 0;
}

void gen_stmt(struct node *node) {
  switch (node->kind) {
  case ND_RETURN:
    gen_expr(node->lhs);
    if (node->lhs) {
      printf("  mov rax, %s\n", reg64[--inc]);
    }
    printf("  jmp .L.return.%s\n", current_fn->name);
    return;
  case ND_IF:
    gen_if(node);
    return;
  case ND_FOR:
    gen_for(node);
    return;
  case ND_BLOCK:
    gen_block(node);
    return;
  case ND_EXPR_STMT:
    gen_expr(node->lhs);
    inc--;
    return;
  }
}

void header() { printf(".intel_syntax noprefix\n"); }

void prologue(int stack_size) {
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", stack_size);
}

void epilogue() {
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

int stack_size(int offset) { return (offset + 15) & ~15; }

void set_offset_and_stack_size(struct function *fn) {
  int offset = 32;
  for (struct var *v = fn->locals; v; v = v->next) {
    offset += v->type->size;
    v->offset = offset;
  }

  fn->stack_size = stack_size(offset);
}

int nargs(struct var *v) {
  int n = 0;
  for (struct var *i = v; i; i = i->next) {
    n++;
  }
  return n;
}

char *data_symbol(int size) {
  switch (size) {
  case 1:
    return ".byte";
  case 2:
    return ".short";
  case 4:
    return ".long";
  case 8:
    return ".quad";
  }
  return ".quad";
}

size_t align(struct type *ty) {
  if (ty->kind == ARRAY)
    return align(ty->ptr_to);

  return ty->size;
}

void global_data(struct program *prog) {
  for (struct var *v = prog->globals; v; v = v->next) {
    printf(".align %lu\n", align(v->type));
    printf("%s: // %s", v->name, type_kind_name(v->type->kind));
    if (v->type->ptr_to)
      printf(" -> %s", type_kind_name(v->type->ptr_to->kind));
    printf("\n");

    if (v->type->kind == INT) {
      if (v->data)
        printf("  %s %d\n", data_symbol(ty_int->size), *(int *)v->data);
      else
        printf("  .quad 0\n");
      continue;
    }
    if (v->type->kind == PTR) {
      if (v->data)
        printf("  .quad %s", v->data);
      else
        printf("  .quad 0");

      if (v->addend) {
        printf(" + %d", v->addend);
      }
      printf("\n");
      continue;
    }
    if (v->type->kind == ARRAY) {
      if (v->type->ptr_to->kind == CHAR) {
        printf("  .string \"%s\"\n", v->data);
        continue;
      } else {

        struct value *value = v->val;
        for (int i = 0; i < v->type->array_size; i++) {
          if (value) {
            if (v->type->ptr_to->kind == INT)
              printf("  %s %d\n", data_symbol(ty_int->size), value->val);

            if (v->type->ptr_to->kind == PTR) {
              if (v->type->ptr_to->ptr_to->kind == CHAR) {
                printf("  .quad %s\n", value->label);
              }
            }
            value = value->next;
          } else {
            printf("  %s %d\n", data_symbol(v->type->ptr_to->size), 0);
          }
        }
        continue;
      }
    }
    if (!v->data)
      printf("  .zero %lu\n", v->type->size);
  }
  return;
}

void gen_code(struct program *prog) {

  header();

  printf(".data\n");

  global_data(prog);

  printf(".text\n");

  for (struct function *fn = prog->functions; fn; fn = fn->next) {
    current_fn = fn;

    printf(".global %s\n", fn->name);

    printf("%s:\n", fn->name);

    set_offset_and_stack_size(fn);

    prologue(fn->stack_size);

    printf("  mov [rbp-8], r12\n");
    printf("  mov [rbp-16], r13\n");
    printf("  mov [rbp-24], r14\n");
    printf("  mov [rbp-32], r15\n");

    int params_num = nargs(fn->params);

    for (struct var *v = fn->params; v; v = v->next) {
      printf("  mov [rbp-%d], %s\n", v->offset, argreg(v->type, --params_num));
    }
    for (struct node *n = fn->stmt; n; n = n->next) {
      gen_stmt(n);
      // fprintf(stderr, "%d\n", inc);
      assert(inc == 0);
    }

    printf(".L.return.%s:\n", fn->name);

    printf("  mov r12, [rbp-8] \n");
    printf("  mov r13, [rbp-16]\n");
    printf("  mov r14, [rbp-24]\n");
    printf("  mov r15, [rbp-32]\n");

    epilogue();
  }
  return;
}
