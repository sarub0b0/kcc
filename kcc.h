#ifndef __KCC_H
#define __KCC_H

#include <stdbool.h>

enum token_kind {
    TK_RESERVED,
    TK_IDENT,
    TK_STR,
    TK_NUM,
    TK_EOF,
    TK_SIZEOF,
    TK_KIND_NUM, // Tokenの種類の数
};

struct token {
    enum token_kind kind;
    struct token *next;
    int val;
    char *str;
    char *loc;
    int len;
};

enum node_kind {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_GT,
    ND_GE,
    ND_ASSIGN,
    ND_NUM,
    ND_RETURN,
    ND_IF,
    ND_FOR,
    ND_LABEL,
    ND_BLOCK,
    ND_FUNCALL,
    ND_VAR,
    ND_ADDR,
    ND_DEREF,
    ND_EXPR_STMT,
};

enum type_kind {
    INT,
    PTR,
};

struct type {
    enum type_kind kind;
    int size;
    struct type *ptr_to;

    // type_kind kind;
    // std::string name;
};

struct var {
    struct var *next;
    char *name;
    struct type *type;

    // local
    int offset;
};

struct node {
    enum node_kind kind;
    char *str;
    struct type *type;

    struct node *lhs;
    struct node *rhs;

    // if, for. while
    struct node *cond;
    struct node *then;
    struct node *els;
    struct node *init;
    struct node *inc;

    // block statement
    struct node *body;
    struct node *next;

    // function call
    struct node *args;
    // var **args;
    int nargs;

    // variable
    struct var *var;

    int val;
};

struct function {
    char *name;
    struct function *next;
    struct type *type;
    struct var *params;

    struct node *stmt;

    struct var *locals;
    int stack_size;
};

void error(const char *fmt, ...);
void error_at(const char *, const char *, ...);

struct token *tokenize(char *);
void gen_code(struct function *);
struct node *expr();
bool consume(char *);
struct token *consume_ident();
void program();

bool equal(struct token *, char *);
void print_tokens(struct token *);
void print_ast(struct function *);

void add_type(struct node *);
struct type *pointer_to(struct type *);

extern char *user_input;
extern struct token *tk;

extern struct var *locals;
extern struct function *functions;
extern int verbose;

extern struct type *ty_int;

#endif
