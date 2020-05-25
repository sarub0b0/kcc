#include <assert.h>
#include <stdio.h>

#include "kcc.h"

const char *argreg64[] = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};

const char *reg[] = {
    "r10", "r11", "r12", "r13", "r14", "r15",
};
const char *reg8[] = {
    "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
};

int inc = 0;
struct function *current_fn;

int label_seq = 0;

int gen_expr(struct node *);
void gen_stmt(struct node *);

void gen_addr(struct node *n) {
  switch (n->kind) {
  case ND_VAR:
    if (n->var->is_local) {
      printf("  lea %s, [rbp-%d]\n", reg[inc++], n->var->offset);
    } else {
      printf("  mov %s, offset %s\n", reg[inc++], n->var->name);
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
  printf("  cmp %s, 0\n", reg[inc - 1]);
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
    printf("  cmp %s, 0\n", reg[inc]);
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
  for (struct node *n = node->args; n; n = n->next) {
    gen_expr(n);
    nargs++;
  }

  for (int i = nargs - 1; 0 <= i; i--) {
    printf("  mov %s, %s\n", argreg64[i], reg[--inc]);
  }

  printf("  push r10\n");
  printf("  push r11\n");
  printf("  mov rax, 0\n");
  printf("  call %s\n", node->str);

  printf("  pop r11\n");
  printf("  pop r10\n");

  printf("  mov %s, rax\n", reg[inc++]);
  return;
}

void load(struct type *type) {
  if (type->kind == ARRAY) {
    return;
  }
  if (type->kind == CHAR) {
    printf("  movsx %s, BYTE PTR [%s]\n", reg[inc - 1], reg[inc - 1]);
  } else {
    printf("  mov %s, [%s]\n", reg[inc - 1], reg[inc - 1]);
  }
}

void store(struct type *type) {
  if (type->kind == CHAR) {
    printf("  mov [%s], %s\n", reg[inc - 1], reg8[inc - 2]);
  } else {
    printf("  mov [%s], %s\n", reg[inc - 1], reg[inc - 2]);
  }
  inc--;
}

int gen_expr(struct node *node) {
  if (!node) {
    return 0;
  }

  switch (node->kind) {
  case ND_NUM:
    printf("  mov %s, %d\n", reg[inc++], node->val);
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
  case ND_EXPR_STMT:
    for (struct node *n = node->body; n; n = n->next) {
      gen_stmt(n);
      inc++;
    }
    return 0;
  }

  gen_expr(node->lhs);
  gen_expr(node->rhs);

  const char *rs = reg[inc - 1];
  const char *rd = reg[inc - 2];
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
    printf("  mov rax, %s\n", rd);
    printf("  cqo\n");
    printf("  idiv %s\n", rs);
    printf("  mov %s, rax\n", rd);
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
  }

  return 0;
}

void gen_stmt(struct node *node) {
  switch (node->kind) {
  case ND_RETURN:
    gen_expr(node->lhs);
    if (node->lhs->kind == ND_FUNCALL) {
      printf("  push rax\n");
    }
    printf("  mov rax, %s\n", reg[--inc]);
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
  }
}

void global_data(struct program *prog) {
  for (struct var *v = prog->globals; v; v = v->next) {
    printf("%s: // %s\n", v->name, type_kind_name(v->type->kind));
    if (v->type->kind == INT) {
      printf("  .quad %d\n", *(int *)v->data);
      continue;
    }
    if (v->type->kind == PTR) {
      printf("  .quad %s", v->data);
      if (v->addend) {
        printf(" + %d", v->addend);
      }
      printf("\n");
      continue;
    }
    if (v->type->kind == ARRAY && v->type->ptr_to->kind != CHAR) {
      for (struct value *val = v->val; val; val = val->next) {
        if (v->type->ptr_to->kind == INT)
          printf("  .quad %d\n", val->val);

        if (v->type->ptr_to->kind == PTR) {
          if (v->type->ptr_to->ptr_to->kind == CHAR) {
            printf("  .string \"%s\"\n", val->label);
          }
        }
      }
      continue;
    }
    if (!v->data) {
      printf("  .zero %lu\n", v->type->size);
      continue;
    }
    printf("  .string \"%s\"\n", v->data);
  }
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
      printf("  mov [rbp-%d], %s\n", v->offset, argreg64[--params_num]);
    }
    for (struct node *n = fn->stmt->body; n; n = n->next) {
      gen_stmt(n);
      assert(inc == 0);
    }

    printf(".L.return.%s:\n", fn->name);

    printf("  mov r12, [rbp-8] \n");
    printf("  mov r13, [rbp-16]\n");
    printf("  mov r14, [rbp-24]\n");
    printf("  mov r15, [rbp-32]\n");

    epilogue();
  }
}
