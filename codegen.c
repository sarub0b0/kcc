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
int func_seq = 0;

int gen_expr(struct node *);
void gen_stmt(struct node *);

size_t type_size(struct type *ty) {
  if (ty->kind == TY_ARRAY) return type_size(ty->ptr_to);

  return ty->size;
}

const char *areg(struct type *ty) {
  const char *r[] = {"al", "ax", "eax", "rax"};

  if (ty->kind == TY_ARRAY) return areg(ty->ptr_to);

  switch (size_of(ty)) {
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
  if (ty->kind == TY_ARRAY) return argreg(ty->ptr_to, i);

  switch (size_of(ty)) {
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
  if (ty->kind == TY_ARRAY) return reg(ty->ptr_to, i);

  switch (size_of(ty)) {
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
  switch (size_of(ty)) {
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
        printf("    lea %s, [rbp-%d]\n", reg64[inc++], n->var->offset);
      } else {
        printf("    mov %s, offset %s\n", reg64[inc++], n->var->name);
      }
      return;
    case ND_DEREF:
      gen_expr(n->lhs);
      return;
    case ND_MEMBER:
      gen_addr(n->lhs);
      printf("    add %s, %d\n", reg64[inc - 1], n->member->offset);
      return;
    default:
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

  int seq = label_seq++;
  gen_expr(node->cond);
  printf("    cmp %s, 0\n", reg64[inc - 1]);
  inc--;

  if (node->els) {
    printf("    je  .L.else.%03d\n", seq);

    gen_stmt(node->then);

    printf("    jmp  .L.end.%03d\n", seq);
    printf(".L.else.%03d:\n", seq);

    gen_stmt(node->els);

    printf(".L.end.%03d:\n", seq);
  } else {

    printf("    je  .L.end.%03d\n", seq);

    gen_stmt(node->then);

    printf(".L.end.%03d:\n", seq);
  }
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

  int seq = label_seq++;
  if (node->init) {
    gen_stmt(node->init);
  }
  printf(".L.begin.%03d:\n", seq);

  if (node->cond) {
    gen_expr(node->cond);
    inc--;
    printf("    cmp %s, 0\n", reg64[inc]);
    printf("    je  .L.end.%03d\n", seq);
  }

  if (node->then) {
    gen_stmt(node->then);
  }

  if (node->inc) {
    gen_stmt(node->inc);
  }
  printf("    jmp .L.begin.%03d\n", seq);
  printf(".L.end.%03d:\n", seq);
}

void gen_do(struct node *node) {
  // while (A) B -> for (;B;) D
  // for (A; B; C) D
  // do STMT while (EXPR)
  //
  // .L.begin.XXX:
  //   Bをコンパイル
  //   pop rax
  //   cmp rax, 0
  //   je  .L.end.XXX
  //   Dをコンパイル
  //   jmp .L.begin.XXX
  // .L.end.XXX

  int seq = label_seq++;
  printf(".L.begin.%03d:\n", seq);

  if (node->then) {
    gen_stmt(node->then);
  }

  if (node->cond) {
    gen_expr(node->cond);
    inc--;
    printf("    cmp %s, 0\n", reg64[inc]);
    printf("    je  .L.end.%03d\n", seq);
  }

  printf("    jmp .L.begin.%03d\n", seq);
  printf(".L.end.%03d:\n", seq);
}

void gen_block(struct node *node) {
  for (struct node *n = node->body; n; n = n->next) gen_stmt(n);
}

void gen_func(struct node *node) {
  struct type *fn_ty = node->func_ty;

  for (struct node *n = node->body; n; n = n->next) {
    gen_expr(n);
    inc--;
  }
  if (6 < node->nargs) {
    error("Too many funcall argumentes");
  }

  for (int i = node->nargs - 1; 0 <= i; i--) {
    struct var *arg = node->args[i];

    int size = size_of(arg->type);
    struct type *arg_ty = arg->type;

    if (arg_ty->kind == TY_ARRAY) size = 8;

    char *insn = arg_ty->is_unsigned ? "movzx" : "movsx";

    switch (size) {
      case 1:
        printf(
            "    %s %s, BYTE PTR [rbp-%d]\n", insn, argreg32[i], arg->offset);
        break;
      case 2:
        printf(
            "    %s %s, WORD PTR [rbp-%d]\n", insn, argreg32[i], arg->offset);
        break;
      case 4:
        printf("    mov %s, DWORD PTR [rbp-%d]\n", argreg32[i], arg->offset);
        break;
      case 8:
        printf("    mov %s, [rbp-%d]\n", argreg64[i], arg->offset);
        break;
    }
  }

  printf("    push r10\n");
  printf("    push r11\n");
  printf("    mov rax, 0\n");
  printf("    call %s\n", node->token->str);

  printf("    pop r11\n");
  printf("    pop r10\n");

  if (fn_ty->kind != TY_VOID)
    printf("    mov %s, %s\n", reg(fn_ty, inc++), areg(fn_ty));
  else
    inc++;

  return;
}

void load(struct type *type) {
  if (type->kind == TY_ARRAY || type->kind == TY_STRUCT ||
      type->kind == TY_UNION) {
    return;
  }
  const char *r = reg(type, inc - 1);
  const char *s = size_name(type);

  char *insn = type->is_unsigned ? "movzx" : "movsx";
  printf("    mov %s, %s PTR [%s]\n", r, s, reg64[inc - 1]);
  switch (size_of(type)) {
    case 1:
    case 2:
      printf("    %s %s, %s\n", insn, reg32[inc - 1], r);
      break;
  }
}

void store(struct type *type) {
  if (type->kind == TY_STRUCT || type->kind == TY_UNION) {

    for (int i = 0; i < size_of(type); i++) {
      printf("    mov al, [%s+%d]\n", reg64[inc - 2], i);
      printf("    mov [%s+%d], al\n", reg64[inc - 1], i);
    }
  } else {
    printf("    mov [%s], %s\n", reg64[inc - 1], reg(type, inc - 2));
  }
  inc--;
}

int gen_expr(struct node *node) {
  if (!node) {
    return 0;
  }

  struct type *ty = node->type;
  switch (node->kind) {
    case ND_NUM:
      printf("    mov %s, %d\n", reg(ty, inc++), node->val);
      return 0;
    case ND_VAR:
      gen_addr(node);
      load(node->type);
      return 0;
    case ND_MEMBER:
      gen_addr(node);
      load(node->type);
      return 0;
    case ND_ASSIGN: {
      // case 1
      //    const int  a = 0;
      //    a = 1;
      // case 2
      //    int b, c;
      //    const int *a = &b;
      //    int const *a = &b;
      //    a = &c; // OK
      //    *a = c; // Err
      //
      // case 3
      //    int b, c;
      //    int *const a = &b;
      //    a = &c; // Err
      //    *b = c; // OK
      //
      // case 4
      //    int b, c;
      //    const int *const a = &b;
      //    a = &c; // Err
      //    *b = c; // Err

      // case 2 only
      enum node_kind kind = node->lhs->kind;
      struct type *ty1 = node->lhs->type;
      struct type *ty2 = kind == ND_MEMBER ? node->lhs->lhs->type : NULL;

      if ((ty1->is_const && node->is_init == false) |
          (kind == ND_MEMBER && ty2->is_const)) {

        error_tok(node->token, "cannot assign to a const variable");
      }

      gen_expr(node->rhs);
      gen_addr(node->lhs);
      store(node->type);
      return 0;
    }
    case ND_FUNCALL:
      gen_expr(node->lhs);
      gen_func(node);
      return 0;
    case ND_COMMA:
      gen_expr(node->lhs);
      gen_expr(node->rhs);
      inc--;
      return 0;
    case ND_ADDR:
      gen_addr(node->lhs);
      return 0;
    case ND_DEREF:
      gen_expr(node->lhs);
      load(node->type);
      return 0;
    case ND_NOT:
      gen_expr(node->lhs);
      printf("    cmp %s, 0\n", reg(node->lhs->type, --inc));
      printf("    sete %s\n", reg8[inc]);
      printf("    movzb %s, %s\n", reg64[inc], reg8[inc]);
      inc++;
      return 0;
    case ND_BITNOT:
      gen_expr(node->lhs);
      printf("    not %s\n", reg(node->lhs->type, inc - 1));
      return 0;
    case ND_LOGOR: {
      // lhs || rhs
      // lhs == 0 ?
      int seq = label_seq++;
      gen_expr(node->lhs);
      printf("    cmp %s, 0\n", reg(node->lhs->type, --inc));
      printf("    jne .L.true.%03d\n", seq);

      // rhs == 0 ?
      gen_expr(node->rhs);
      printf("    cmp %s, 0\n", reg(node->rhs->type, --inc));
      printf("    jne .L.true.%03d\n", seq);
      printf("    mov %s, 0\n", reg64[inc - 1]);
      printf("    jmp .L.end.%03d\n", seq);
      printf(".L.true.%03d:\n", seq);
      printf("    mov %s, 1\n", reg64[inc++]);
      printf(".L.end.%03d:\n", seq);
      return 0;
    }
    case ND_LOGAND: {
      // lhs && rhs
      // lhs == 0 ?
      int seq = label_seq++;
      gen_expr(node->lhs);
      printf("    cmp %s, 0\n", reg(node->lhs->type, --inc));
      printf("    je .L.false.%03d\n", seq);

      // rhs == 0 ?
      gen_expr(node->rhs);
      printf("    cmp %s, 0\n", reg(node->rhs->type, --inc));
      printf("    je .L.false.%03d\n", seq);
      printf("    mov %s, 1\n", reg64[inc - 1]);
      printf("    jmp .L.end.%03d\n", seq);
      printf(".L.false.%03d:\n", seq);
      printf("    mov %s, 0\n", reg64[inc++]);
      printf(".L.end.%03d:\n", seq);
      return 0;
    }
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

      char *insn = to->is_unsigned ? "movzx" : "movsx";
      int to_size = size_of(to);
      if (to_size == 1) {
        printf("    %s %s, %s\n", insn, reg32[inc - 1], reg8[inc - 1]);
      } else if (to_size == 2) {
        printf("    %s %s, %s\n", insn, reg32[inc - 1], reg16[inc - 1]);
      } else if (to_size == 4) {
        printf("    mov %s, %s\n", reg32[inc - 1], reg32[inc - 1]);
      } else if (is_integer(from) && size_of(from) < 8) {
        printf("    movsx %s, %s\n", reg64[inc - 1], reg(from, inc - 1));
      }

      return 0;
    }
    case ND_COND: {
      int seq = label_seq++;
      gen_expr(node->cond);
      printf("    cmp %s, 0\n", reg64[inc - 1]);
      inc--;

      printf("    je  .L.else.%03d\n", seq);

      gen_expr(node->then);
      inc--;

      printf("    jmp  .L.end.%03d\n", seq);
      printf(".L.else.%03d:\n", seq);

      gen_expr(node->els);

      printf(".L.end.%03d:\n", seq);

      return 0;
    }
    case ND_LIST_EXPR:
      for (struct node *n = node->body; n; n = n->next) {
        gen_expr(n);
        inc--;
      }
      inc++;
      return 0;
    default:
      break;
  }

  gen_expr(node->lhs);
  gen_expr(node->rhs);

  const char *rs = reg(node->lhs->type, inc - 1);
  const char *rd = reg(node->lhs->type, inc - 2);
  const char *ra = areg(node->lhs->type);
  inc--;

  switch (node->kind) {
    case ND_ADD:
      printf("    add %s, %s\n", rd, rs);
      break;
    case ND_SUB:
      printf("    sub %s, %s\n", rd, rs);
      break;
    case ND_MUL:
      printf("    imul %s, %s\n", rd, rs);
      break;
    case ND_DIV:
      printf("    mov %s, %s\n", ra, rd);
      printf("    cqo\n");
      printf("    idiv %s\n", rs);
      printf("    mov %s, %s\n", rd, ra);
      break;
    case ND_EQ:
      printf("    cmp %s, %s\n", rd, rs);
      printf("    sete al\n");
      printf("    movzb %s, al\n", rd);
      break;
    case ND_NE:
      printf("    cmp %s, %s\n", rd, rs);
      printf("    setne al\n");
      printf("    movzb %s, al\n", rd);
      break;
    case ND_LE:
      printf("    cmp %s, %s\n", rd, rs);
      if (node->lhs->type->is_unsigned) {
        printf("    setbe al\n");
      } else {
        printf("    setle al\n");
      }
      printf("    movzb %s, al\n", rd);
      break;
    case ND_LT:
      printf("    cmp %s, %s\n", rd, rs);
      if (node->lhs->type->is_unsigned) {
        printf("    setb al\n");
      } else {
        printf("    setl al\n");
      }
      printf("    movzb %s, al\n", rd);
      break;
    case ND_GE:
      printf("    cmp %s, %s\n", rd, rs);
      printf("    setge al\n");
      printf("    movzb %s, al\n", rd);
      break;
    case ND_GT:
      printf("    cmp %s, %s\n", rd, rs);
      printf("    setg al\n");
      printf("    movzb %s, al\n", rd);
      break;
    case ND_BITOR:
      printf("    or %s, %s\n", rd, rs);
      break;
    case ND_BITXOR:
      printf("    xor %s, %s\n", rd, rs);
      break;
    case ND_BITAND:
      printf("    and %s, %s\n", rd, rs);
      break;
    case ND_SHL:
      printf("    mov rcx, %s\n", reg64[inc]);
      printf("    shl %s, cl\n", rd);
      break;
    case ND_SHR:
      printf("    mov rcx, %s\n", reg64[inc]);
      if (node->lhs->type->is_unsigned) {
        printf("    shr %s, cl\n", rd);
      } else {
        printf("    sar %s, cl\n", rd);
      }
      break;
    default:
      error_tok(node->token, "Don't assembly calculation (%d)", node->kind);
      break;
  }

  return 0;
}

void gen_stmt(struct node *node) {
  switch (node->kind) {
    case ND_RETURN:
      gen_expr(node->lhs);
      if (node->lhs) {
        printf("    mov rax, %s\n", reg64[--inc]);
      }
      printf("    jmp .L.return.%s\n", current_fn->name);
      return;
    case ND_IF:
      gen_if(node);
      return;
    case ND_FOR:
      gen_for(node);
      return;
    case ND_DO:
      gen_do(node);
      return;
    case ND_BLOCK:
      gen_block(node);
      return;
    case ND_EXPR_STMT:
      gen_expr(node->lhs);
      inc--;
      return;
    default:
      error_tok(node->token, "Don't assembly statement (%d)", node->kind);
      return;
  }
}

void header() {
  printf("    .intel_syntax noprefix\n");
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
  if (ty->kind == TY_ARRAY) return align(ty->ptr_to);

  if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) return ty->align;

  return ty->size;
}

void emit_value(int *pos,
                int size,
                struct var *var,
                struct type *type,
                struct value **value) {

  struct value *val = *value;
  if (val && val->offset == *pos) {
    printf("    .quad %s%+ld\n", val->label, val->addend);
    *value = val->next;
    *pos += 8;
    return;
  }

  if (type->is_string) {
    printf("    .string \"%s\"\n", var->data);
    *pos += var->type->size;
    return;
  }

  if (type->kind == TY_ARRAY) {
    for (int i = 0; i < type->array_size; i++) {
      emit_value(pos, size_of(type->ptr_to), var, type->ptr_to, value);
    }
    return;
  }

  if (type->kind == TY_STRUCT || type->kind == TY_UNION) {
    for (struct member *m = type->members; m; m = m->next) {
      int sz = 0;
      if (m->next) {
        sz = m->next->offset - m->offset;
      } else {
        sz = size - m->offset;
      }

      emit_value(pos, sz, var, m->type, value);
    }
    return;
  }

  switch (size) {
    case 1:
      printf("    .byte %d\n", (char) var->data[*pos]);
      *pos += 1;
      return;
    case 2:
      printf("    .short %d\n", (short) var->data[*pos]);
      *pos += 2;
      return;
    case 4:
      printf("    .long %d\n", (int) var->data[*pos]);
      *pos += 4;
      return;
    case 8:
      printf("    .quad %ld\n", (long) var->data[*pos]);
      *pos += 8;
  }
}

void emit_data_info(struct var *v, bool is_rodata) {
  if (!v->is_static) {
    printf("    .globl %s\n", v->name);
  }

  if (is_rodata) {
  }
  printf("    .align %lu\n", align(v->type));

  printf("%s: // %s", v->name, type_to_name(v->type->kind));

  if (v->type->ptr_to) {
    printf(" -> %s", type_to_name(v->type->ptr_to->kind));
  }
  printf("\n");
}

void data_section(struct program *prog) {
  printf("    .data\n");

  for (struct var *v = prog->globals; v; v = v->next) {
    if (!v->data) continue;

    emit_data_info(v, false);

    struct value **value = &v->values;

    int size = size_of(v->type);
    int pos = 0;
    while (pos < size) {
      emit_value(&pos, size, v, v->type, value);
    }
  }
}
void bss_section(struct program *prog) {
  printf("    .bss\n");

  for (struct var *v = prog->globals; v; v = v->next) {
    if (v->data) continue;

    emit_data_info(v, false);
    printf("    .zero %d\n", size_of(v->type));
  }
}
void prologue(int stack_size) {
  printf("    push rbp\n");
  printf("    mov rbp, rsp\n");
  printf("    sub rsp, %d\n", stack_size);
}

void epilogue() {
  printf("    mov rsp, rbp\n");
  printf("    pop rbp\n");
  printf("    ret\n");
}

int stack_size(int offset) {
  return (offset + 15) & ~15;
}

void set_offset_and_stack_size(struct function *fn) {
  int offset = 32;
  for (struct var *v = fn->locals; v; v = v->next) {
    offset += size_of(v->type);
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

void text_section(struct program *prog) {
  printf("    .text\n");

  for (struct function *fn = prog->functions; fn; fn = fn->next) {
    current_fn = fn;

    if (!fn->is_static) {
      printf("    .globl %s\n", fn->name);
    }

    printf("%s:\n", fn->name);

    printf(".LFB%d:\n", func_seq);

    set_offset_and_stack_size(fn);

    prologue(fn->stack_size);

    printf("    mov [rbp-8], r12\n");
    printf("    mov [rbp-16], r13\n");
    printf("    mov [rbp-24], r14\n");
    printf("    mov [rbp-32], r15\n");

    int params_num = nargs(fn->params);

    for (struct var *v = fn->params; v; v = v->next) {
      printf(
          "    mov [rbp-%d], %s\n", v->offset, argreg(v->type, --params_num));
    }
    for (struct node *n = fn->stmt; n; n = n->next) {
      gen_stmt(n);
      assert(inc == 0);
    }

    printf(".L.return.%s:\n", fn->name);

    printf("    mov r12, [rbp-8] \n");
    printf("    mov r13, [rbp-16]\n");
    printf("    mov r14, [rbp-24]\n");
    printf("    mov r15, [rbp-32]\n");

    epilogue();

    printf(".LFE%d:\n", func_seq++);
  }
}

void gen_code(struct program *prog) {
  printf("    .file \"%s\"\n", prog->filename);

  header();

  bss_section(prog);
  data_section(prog);
  text_section(prog);

  return;
}
