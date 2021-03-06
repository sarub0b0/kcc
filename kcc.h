#ifndef __KCC_H
#define __KCC_H

#include <stdbool.h>
#include <stdio.h>

#define debug(fmt...)                              \
  do {                                             \
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
    fprintf(stderr, fmt);                          \
    fprintf(stderr, "\n");                         \
  } while (0)

#define find_cond(name, token) \
  (strlen(name) == token->len && strncmp(name, token->str, token->len) == 0)

// #define error(...)
// #define verror_at(...)
// #define error_at(...)
// #define error_tok(...)
// #define warn_tok(...)
// #define info_tok(...)

struct type;

struct hideset {
  struct hideset *next;
  char *name; // macro name
};

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
  long val;
  char *str;
  char *loc;
  int len;
  char *filename;
  char *input;
  int file_num;
  int col;

  // string-literal
  int str_len;
  char *str_literal;

  // line info
  bool at_bol;
  bool has_space;
  int line_num;

  // type of numeric literals
  struct type *type;

  // hideset is used macro expansion
  struct hideset *hideset;
};

enum node_kind {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_MOD,
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
  ND_DO,
  ND_SWITCH,
  ND_CASE,
  ND_CONTINUE,
  ND_BREAK,
  ND_LABEL,
  ND_BLOCK,
  ND_FUNCALL,
  ND_VAR,
  ND_ADDR,
  ND_DEREF,
  ND_BINARY,
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
  ND_SHL,
  ND_SHR,
};

enum type_kind {
  TY_PTR,
  TY_ARRAY,
  TY_VOID,
  TY_BOOL,
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_FLOAT,
  TY_DOUBLE,
  TY_STRUCT,
  TY_UNION,
  TY_ENUM,
  TY_FUNC,
  TY_KIND_NUM,
};

struct type {
  enum type_kind kind;
  size_t size;
  size_t align;
  bool is_unsigned;
  bool is_const;
  bool is_variadic;
  bool is_va_list;

  struct type *ptr_to;
  size_t array_size;

  struct type *next;

  // name token
  struct token *token;
  // char *name;

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

  bool is_static;

  char *data;
  long addend;
  struct value *values;
};

struct member {
  struct member *next;
  struct type *type;
  int align;
  int offset;
  struct token *name;
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

  // switch-case
  struct node *case_next;
  struct node *default_case;
  int case_label;
  int switch_seq;

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

  long val;

  bool is_init;
};

struct function {
  char *name;
  struct token *token;
  struct function *next;
  struct type *type;
  struct var *params;

  bool is_variadic;
  bool is_static;

  struct node *stmt;

  struct var *locals;
  int stack_size;
};

struct program {
  struct function *functions;
  struct var *globals;
  char *filename;
};
void verror_at(char *, char *, char *, int, char *, va_list);
void error(char *, ...);
void error_at(char *, char *, ...);
void warn_tok(struct token *, char *, ...);
void error_tok(struct token *, char *, ...);
void info_tok(struct token *, char *, ...);

struct token *tokenize(char *, char *, int);
struct token *tokenize_file(char *);
struct token *preprocess(struct token *);
struct program *parse(struct token *);
void gen_code(struct program *);
char **get_input_files();
void skip(struct token **, struct token *, char *);
bool consume(struct token **, struct token *, char *);
struct token *consume_ident(struct token **, struct token *);
bool equal(struct token *, char *);
bool at_eof(struct token *);
long get_number(struct token *);
long const_expr(struct token **, struct token *);

struct token *copy_token(struct token *);

void print_tokens(struct token *);
void print_tokens_text(struct token *);
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
struct type *func_type(struct type *);

char *type_to_name(enum type_kind);

extern struct type *ty_void;
extern struct type *ty_bool;

extern struct type *ty_char;
extern struct type *ty_short;
extern struct type *ty_int;
extern struct type *ty_long;

extern struct type *ty_schar;
extern struct type *ty_sshort;
extern struct type *ty_sint;
extern struct type *ty_slong;

extern struct type *ty_uchar;
extern struct type *ty_ushort;
extern struct type *ty_uint;
extern struct type *ty_ulong;

extern struct type *ty_float;
extern struct type *ty_double;

extern struct type *ty_enum;
extern struct type *ty_struct;
extern struct type *ty_union;

extern int verbose;
extern char *current_userinput;
extern char *current_filename;
extern char **include_paths;

#endif
