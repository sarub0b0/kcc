#ifndef __KCC_H
#define __KCC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define debug(fmt...)                              \
  do {                                             \
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
    fprintf(stderr, fmt);                          \
    fprintf(stderr, "\n");                         \
  } while (0)

#define find_cond(name, token) \
  (strlen(name) == token->len && strncmp(name, token->str, token->len) == 0)

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
  char *filename;
  char *input;

  // string-literal
  int str_len;
  char *str_literal;

  // line info
  bool at_bol;
  bool has_space;
  int line_num;
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
  ND_LIST_EXPR,
  ND_CAST,
  ND_COND,
  ND_LOGOR,
  ND_LOGAND,
  ND_BITOR,
  ND_BITXOR,
  ND_BITAND,
  ND_BITNOT,
  ND_MEMBER,
  ND_COMMA,
  ND_NOT,
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
  ENUM,
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

  // name token
  struct token *token;

  // function
  struct type *return_type;
  struct type *params;

  // struct member
  struct member *members;
  char *tag;

  //
  bool is_incomplete;
  bool is_string;
};

struct value {
  struct value *next;
  int offset;
  char *label;
  long addend;
};

struct var {
  struct var *next;
  char *name;
  struct type *type;
  // struct token *token;

  // local
  int offset;

  bool is_local;

  char *data;
  int addend;
  struct value *values;
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
  struct token *token;
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
  struct node *args_node;
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

  bool is_variadic;

  struct node *stmt;

  struct var *locals;
  int stack_size;
};

struct program {
  struct function *functions;
  struct var *globals;
};
void verror_at(char *, char *, char *, int, char *, va_list);
void error(char *, ...);
void error_at(char *, char *, ...);
void warn_tok(struct token *, char *, ...);
void error_tok(struct token *, char *, ...);

struct token *tokenize(char *, char *);
struct token *tokenize_file(char *);
struct token *preprocess(struct token *);
struct program *parse(struct token *);
void gen_code(struct program *);

void skip(struct token **, struct token *, char *);
bool consume(struct token **, struct token *, char *);
struct token *consume_ident(struct token **, struct token *);
bool equal(struct token *, char *);
bool at_eof(struct token *);
int get_number(struct token *);
long const_expr(struct token **, struct token *);

void print_tokens(struct token *);
void print_ast(struct program *, char *);
void print_function(struct program *);
void print_tok_pos(struct token *);
void print_type(struct type *);

bool is_integer(struct type *);
void add_type(struct node *);
int size_of(struct type *);

struct node *new_node_cast(struct node *, struct type *);
struct type *copy_type(struct type *);
struct type *pointer_to(struct type *);
struct type *array_to(struct type *, size_t);

char *type_to_name(enum type_kind);

extern struct type *ty_void;
extern struct type *ty_short;
extern struct type *ty_int;
extern struct type *ty_long;
extern struct type *ty_char;
extern struct type *ty_bool;
extern struct type *ty_enum;
extern struct type *ty_struct;

extern int verbose;
extern char *current_userinput;
extern char *current_filename;
extern char **include_paths;
#endif
