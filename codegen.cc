#include <cstdio>

#include "kcc.h"

int label_seq = 0;

void gen_lval(node *node) {
  if (node->kind != ND_LVAR) {
    error("代入の左辺値が変数ではありません");
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen_if(node *node) {
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

  gen(node->cond);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");

  if (node->els) {
    printf("  je  .L.else.%03d\n", label_seq);

    gen(node->then);

    printf("  jmp  .L.end.%03d\n", label_seq);
    printf(".L.else.%03d:\n", label_seq);

    gen(node->els);

    printf(".L.end.%03d:\n", label_seq);
  } else {

    printf("  je  .L.end.%03d\n", label_seq);

    gen(node->then);

    printf(".L.end.%03d:\n", label_seq);

    label_seq++;
  }
}

void gen_for(node *node) {
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
    gen(node->init);
  }
  printf(".L.begin.%03d:\n", label_seq);

  gen(node->cond);
  printf("  pop rax\n");
  printf("  cmp rax, 0\n");
  printf("  je  .L.end.%03d\n", label_seq);

  gen(node->then);

  if (node->inc) {
    gen(node->inc);
  }
  printf("  jmp .L.begin.%03d\n", label_seq);
  printf(".L.end.%03d:\n", label_seq);

  label_seq++;
}

void gen(node *node) {
  if (!node) {
    goto out;
  }

  switch (node->kind) {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  case ND_IF:
    gen_if(node);
    return;
  case ND_FOR:
    gen_for(node);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GE:
    printf("  cmp rax, rdi\n");
    printf("  setge al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GT:
    printf("  cmp rax, rdi\n");
    printf("  setg al\n");
    printf("  movzb rax, al\n");
    break;
  }

out:
  printf("  push rax\n");
}
