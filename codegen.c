#include <stdio.h>

#include "kcc.h"

const char *argreg64[] = {
    "rdi",
    "rsi",
    "rdx",
    "rcx",
    "r8",
    "r9",
};

void gen_lval(struct node *);

char *regs[] = {};

int label_seq = 0;

int gen_expr(struct node *);
void gen_stmt(struct node *);

void gen_addr(struct node *n) {
    switch (n->kind) {
        case ND_VAR:
            gen_lval(n);
            return;
        case ND_DEREF:
            gen_expr(n->lhs);
            return;
    }
}

void gen_lval(struct node *node) {
    if (node->kind != ND_VAR) {
        error("代入の左辺値が変数ではありません");
    }

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->var->offset);
    printf("  push rax\n");
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
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");

    if (node->els) {
        printf("  je  .L.else.%03d\n", label_seq);

        gen_stmt(node->then);
        printf("  pop rax\n");

        printf("  jmp  .L.end.%03d\n", label_seq);
        printf(".L.else.%03d:\n", label_seq);

        gen_stmt(node->els);
        printf("  pop rax\n");

        printf(".L.end.%03d:\n", label_seq);
    } else {

        printf("  je  .L.end.%03d\n", label_seq);

        gen_stmt(node->then);
        printf("  pop rax\n");

        printf(".L.end.%03d:\n", label_seq);

        label_seq++;
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

    if (node->init) {
        gen_stmt(node->init);
        printf("  pop rax\n");
    }
    printf(".L.begin.%03d:\n", label_seq);

    if (node->cond) {
        gen_expr(node->cond);
        printf("  pop rax\n");
    }
    printf("  cmp rax, 0\n");
    printf("  je  .L.end.%03d\n", label_seq);

    if (node->then) {
        gen_stmt(node->then);
        printf("  pop rax\n");
    }

    if (node->inc) {
        gen_stmt(node->inc);
        printf("  pop rax\n");
    }
    printf("  jmp .L.begin.%03d\n", label_seq);
    printf(".L.end.%03d:\n", label_seq);

    label_seq++;
}

void gen_block(struct node *node) {
    for (struct node *n = node->body; n; n = n->next) gen_stmt(n);
}

void gen_func(struct node *node) {
    int nargs = 0;
    for (struct node *n = node->args; n; n = n->next) {
        gen_expr(n);
        nargs++;
    }

    for (int i = nargs - 1; 0 <= i; i--) {
        printf("  pop %s\n", argreg64[i]);
    }

    printf("  call %s\n", node->str);
    printf("  push rax\n");
    return;
}

int gen_expr(struct node *node) {
    if (!node) {
        return 0;
    }

    switch (node->kind) {
        case ND_NUM:
            printf("  push %d\n", node->val);
            return 0;
        case ND_VAR:
            gen_addr(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return 0;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen_expr(node->rhs);
            // 返り値を持つ関数の時にpush raxしたい
            // if (node->rhs->kind == ND_FUNCALL) {
            // printf("  push rax\n");
            // }
            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
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
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return 0;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

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
    return 0;
}

void gen_stmt(struct node *node) {
    switch (node->kind) {
        case ND_RETURN:
            gen_expr(node->lhs);
            if (node->lhs->kind == ND_FUNCALL) {
                printf("  push rax\n");
            }
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
        case ND_BLOCK:
            gen_block(node);
            return;
        case ND_EXPR_STMT:
            gen_expr(node->lhs);
            return;
    }
}

void header() {
    printf(".intel_syntax noprefix\n");
}

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

int stack_size(int offset) {
    return (offset + 15) & ~15;
}

void set_offset_and_stack_size(struct function *fn) {
    int offset = 0;
    for (struct var *v = fn->params; v; v = v->next) {
        offset += 8;
        v->offset = offset;
    }

    for (struct var *v = fn->locals; v; v = v->next) {
        offset += 8;
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

void gen_code(struct function *func) {

    header();

    for (struct function *fn = func; fn; fn = fn->next) {

        printf(".global %s\n", fn->name);

        printf("%s:\n", fn->name);

        set_offset_and_stack_size(fn);

        prologue(fn->stack_size);

        int params_num = nargs(fn->params);

        for (struct var *v = fn->params; v; v = v->next) {
            printf("  mov [rbp-%d], %s\n", v->offset, argreg64[--params_num]);
        }
        for (struct node *n = fn->stmt->body; n; n = n->next) {

            gen_stmt(n);
        }

        epilogue();
    }
}
