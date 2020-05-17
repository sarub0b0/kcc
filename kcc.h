#ifndef __KCC_H
#define __KCC_H

#include <stddef.h>
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

    char *string;
    char *string_len;
    int string_idx;
};

struct string {
    struct string *next;
    char *str;
    int len;
    int idx;
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
    ND_STR,
};

enum type_kind {
    INT,
    PTR,
    ARRAY,
    CHAR,
};

struct type {
    enum type_kind kind;
    size_t size;
    char *name;
    struct type *ptr_to;
    size_t array_size;

    struct type *next;

    // function
    struct type *return_type;
    struct type *params;
};

struct var {
    struct var *next;
    char *name;
    struct type *type;

    // local
    int offset;

    bool is_local;
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

    // string literal
    struct string *string;
    int string_idx;

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

struct program {
    struct function *functions;
    struct var *globals;
};

void error(char *fmt, ...);
void error_at(char *, char *, ...);

struct token *tokenize(char *);
void gen_code(struct program *);
struct node *expr();
bool consume(char *);
struct token *consume_ident();
struct program *parse();

bool equal(struct token *, char *);
void print_tokens(struct token *);
void print_ast(struct program *);

bool is_integer(struct type *);
void add_type(struct node *);
struct type *copy_type(struct type *);
struct type *pointer_to(struct type *);
struct type *array_to(struct type *, size_t len);

extern char *user_input;
extern struct token *tk;

extern struct var *locals;
extern int verbose;

extern struct type *ty_int;
extern struct type *ty_char;

extern struct string *strings;

#endif
