#ifndef __KCC_H
#define __KCC_H

#include <stdbool.h>
#include <stddef.h>

#define debug(fmt...)                                                          \
  do {                                                                         \
    fprintf(stderr, fmt);                                                      \
    fprintf(stderr, "\n");                                                     \
  } while (0)

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
  char *file;
  char *input;
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
  ND_STMT_EXPR,
  ND_NULL_STMT,
  ND_CAST,
  ND_COND,
  ND_LOGOR,
  ND_LOGAND,
  ND_BITOR,
  ND_BITXOR,
  ND_BITAND,
  ND_BITNOT,
  ND_MEMBER,
};

enum type_kind {
  INT,
  PTR,
  ARRAY,
  CHAR,
  VOID,
  SHORT,
  LONG,
  BOOL,
  STRUCT,
  TY_KIND_NUM,
};

struct type {
  enum type_kind kind;
  size_t size;
  size_t align;
  char *name;
  struct type *ptr_to;
  size_t array_size;

  struct type *next;

  // function
  struct type *return_type;
  struct type *params;

  // struct member
  struct member *members;
};

struct value {
  struct value *next;

  char *label;
  int val;
};

struct var {
  struct var *next;
  char *name;
  struct type *type;
  // struct type *type_def;

  // local
  int offset;

  bool is_local;

  char *data;
  int addend;
  struct value *val;
};

struct member {
  struct member *next;
  struct type *type;
  int align;
  int offset;
  char *name;
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
  struct type *func_ty;
  struct var *args[10];
  // var **args;
  int nargs;

  // struct member
  struct member *member;

  // string literal
  struct string *string;
  int string_idx;

  // variable
  struct var *var;

  int val;
};

struct function {
  char *name;
  struct token *token;
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

struct token *tokenize(char *, char *);
struct program *parse(struct token *);
void gen_code(struct program *);

bool consume(struct token **, struct token *, char *);
struct token *consume_ident(struct token **, struct token *);
bool equal(struct token *, char *);

void print_tokens(struct token *);
void print_ast(struct program *, char *);
void print_function(struct program *);

bool is_integer(struct type *);
void add_type(struct node *);

struct node *new_cast(struct node *, struct type *);
struct type *copy_type(struct type *);
struct type *pointer_to(struct type *);
struct type *array_to(struct type *, size_t len);

char *type_to_name(enum type_kind kind);

extern struct type *ty_void;
extern struct type *ty_short;
extern struct type *ty_int;
extern struct type *ty_long;
extern struct type *ty_char;
extern struct type *ty_bool;

extern int verbose;
#endif
