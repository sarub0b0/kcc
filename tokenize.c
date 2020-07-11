#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <ctype.h>

#include "kcc.h"

struct string *strings;
char *current_userinput;
char *current_filename;
char **input_files;
static int file_num = 0;

const char *token_kind_str[TK_KIND_NUM] = {
    "reserved",
    "identifier",
    "string",
    "number",
    "eof",
    "sizeof",
};

void print_tokens(struct token *token) {

  for (struct token *t = token; t; t = t->next) {
    fprintf(stderr,
            "%s:%d %*s %s",
            t->filename,
            t->line_num,
            12,
            token_kind_str[t->kind],
            t->str);
    if (t->hideset) {
      fprintf(stderr, "  macro: ");
      for (struct hideset *hs = t->hideset; hs; hs = hs->next)
        fprintf(stderr, "%s, ", hs->name);
    }
    fprintf(stderr, "\n");
  }
}

void print_tokens_text(struct token *token) {
  int depth = 0;
  int line = 1;
  char *space = " ";
  bool prev_paren = false;
  for (struct token *tk = token; tk->kind != TK_EOF; tk = tk->next) {
    if (equal(tk, "{") || equal(tk, "(")) {
      depth += 4;
    }
    if (equal(tk, "}") || equal(tk, ")")) {
      depth -= 4;
    }
    if (line > 1 && tk->at_bol) {
      fprintf(stderr, "\n");
    }

    if (tk->has_space && tk->at_bol) {
      fprintf(stderr, "%*s", depth, " ");
    }

    if (tk->has_space && !tk->at_bol) fprintf(stderr, " ");

    fprintf(stderr, "%.*s", tk->len, tk->loc);
    line++;
  }
  fprintf(stderr, "\n");
}

int starts_with(const char *p, const char *q) {
  return strncmp(p, q, strlen(q)) == 0;
}

bool at_eof(struct token *tk) {
  return tk->kind == TK_EOF;
}

bool equal(struct token *token, char *op) {
  return token->len == strlen(op) && strncmp(token->str, op, strlen(op)) == 0;
}

bool consume(struct token **ret, struct token *tk, char *op) {
  if (equal(tk, op)) {
    *ret = tk->next;
    return true;
  }
  *ret = tk;
  return false;
}

struct token *consume_ident(struct token **ret, struct token *tk) {
  struct token *tok = tk;
  if (tk->kind == TK_IDENT) {
    *ret = tk->next;
    return tok;
  }
  *ret = tk;
  return NULL;
}

struct token *consume_num(struct token **ret, struct token *tk) {
  struct token *tok = tk;
  if (tk->kind == TK_IDENT) {
    *ret = tk->next;
    return tok;
  }
  *ret = tk;
  return NULL;
}

struct token *new_token(enum token_kind kind,
                        struct token *cur,
                        char *loc,
                        int len) {
  struct token *t = calloc(1, sizeof(struct token));
  t->kind = kind;
  t->len = len;
  t->str = strndup(loc, len);
  t->loc = loc;
  t->input = current_userinput;
  t->filename = current_filename;
  cur->next = t;
  return t;
}

bool is_hex(char c) {
  return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
         ('A' <= c && c <= 'F');
}

int hex(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  }
  if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  }

  return 0;
}

int escaped_char(char **new_pos, char *p) {
  // \000 - \777: \x, \xx, \xxx
  if ('0' <= *p && *p <= '7') {
    int c = *p++ - '0';
    if ('0' <= *p && *p <= '7') {
      c = (c * 8) + (*p++ - '0');
      if ('0' <= *p && *p <= '7') {
        c = (c * 8) + (*p++ - '0');
      }
    }
    *new_pos = p;
    return c;
  }

  // 0x00 - 0xff
  if (*p == 'x') {
    p++;
    if (!is_hex(*p)) {
      error_at(p, "invalid hex escape sequence");
    }

    int c = 0;

    for (; is_hex(*p); p++) {
      c = (c * 16) + hex(*p);

      if (c >= 256) {
        error_at(p, "hex escape sequence out of range");
      }
    }

    *new_pos = p;
    return c;
  }

  *new_pos = p + 1;
  switch (*p) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case 'e':
      return 27;
    default:
      return *p;
  }
}

struct token *string_literal_token(struct token *cur, char *start) {
  char *p = start + 1;

  char *end = p;

  while (*end != '"') {
    if (*end == '\\') {
      end++;
    }
    end++;
  }

  char *buf = malloc(end - p + 1);
  int len = 0;

  while (*p != '"') {
    if (*p == '\\') buf[len++] = *p++;
    buf[len++] = *p++;
  }

  buf[len++] = '\0';

  struct token *ret = new_token(TK_STR, cur, start, p - start + 1);
  ret->str_len = len;
  ret->str_literal = buf;

  return ret;
}

struct token *char_literal_token(struct token *cur, char *start) {

  char *p = start + 1;
  if (*p == '\0') {
    error_tok(cur, "");
  }

  int val;

  if (*p == '\\') {
    val = escaped_char(&p, p + 1);
  } else {
    val = *p++;
  }

  if (*p != '\'') {
    error_tok(cur, "char literal too long");
  }
  p++;

  struct token *ret = new_token(TK_NUM, cur, start, p - start);
  ret->val = val;
  ret->type = copy_type(ty_int);
  return ret;
}

int is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

int is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

bool is_keyword(struct token *tok) {
  char *keyword[] = {
      "return",   "if",      "else",     "for",      "while",  "do",
      "switch",   "case",    "default",  "continue", "break",  "void",
      "_Bool",    "char",    "short",    "int",      "long",   "float",
      "double",   "signed",  "unsigned", "sizeof",   "struct", "union",
      "enum",     "typedef", "static",   "extern",   "const",  "restrict",
      "volatile", "inline",
  };

  for (int i = 0; i < sizeof(keyword) / sizeof(*keyword); i++) {
    if (equal(tok, keyword[i])) {
      return true;
    }
  }

  return false;
}

void convert_ident_to_reserved(struct token *token) {
  for (struct token *tok = token; tok; tok = tok->next) {
    if (tok->kind == TK_IDENT && is_keyword(tok)) {
      tok->kind = TK_RESERVED;
      if (equal(tok, "sizeof")) {
        tok->kind = TK_SIZEOF;
      }
      continue;
    }
  }
}

void add_line_info(struct token *tk) {
  char *p = current_userinput;
  bool at_bol = true;
  bool has_space = false;
  int line_num = 1;
  int col = 0;

  do {
    col++;
    if (p == tk->loc) {
      tk->at_bol = at_bol;
      tk->has_space = has_space;
      tk->line_num = line_num;
      tk->col = col;
      tk = tk->next;
    }

    if (*p == '\n') {
      line_num++;
      col = 0;
      at_bol = true;
    } else if (isspace(*p)) {
      has_space = true;
    } else {
      has_space = false;
      at_bol = false;
    }
  } while (*p++);
}

struct token *tokenize(char *filename, char *input, int file_num) {
  strings = NULL;
  current_filename = filename;
  current_userinput = input;

  struct token head = {};
  struct token *cur = &head;
  int string_idx = 0;

  char *p = input;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (starts_with(p, "//")) {
      while (*p != '\n') p++;
      continue;
    }

    if (starts_with(p, "/*")) {
      char *q = strstr(p + 2, "*/");
      if (!q) error_at(p, "コメントが閉じられていません");
      p = q + 2;
      continue;
    }

    if (starts_with(p, "...") || starts_with(p, ">>=") ||
        starts_with(p, "<<=")) {
      cur = new_token(TK_RESERVED, cur, p, 3);
      p += 3;
      continue;
    }

    if (starts_with(p, "==") || starts_with(p, "!=") ||
        starts_with(p, ">=") || starts_with(p, "<=") ||
        starts_with(p, "++") || starts_with(p, "--") ||
        starts_with(p, "+=") || starts_with(p, "-=") ||
        starts_with(p, "*=") || starts_with(p, "/=") ||
        starts_with(p, "|=") || starts_with(p, "^=") ||
        starts_with(p, "&=") || starts_with(p, "&&") ||
        starts_with(p, "||") || starts_with(p, "<<") ||
        starts_with(p, ">>") || starts_with(p, "->") ||
        starts_with(p, "##")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (is_alpha(*p)) {
      char *q = p;
      while (is_alnum(*p)) {
        p++;
      }
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    if (*p == '"') {
      cur = string_literal_token(cur, p);
      p += cur->len;
      continue;
    }

    if (*p == '\'') {
      cur = char_literal_token(cur, p);
      p += cur->len;
      continue;
    }

    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isdigit(*p)) {
      char *q = p;

      int base = 10;
      if (p[1] == 'x') {
        base = 16;
      }

      enum {
        LONG = 1 << 0,
        UNSIGNED = 1 << 2,
      };

      long val = strtoull(p, &p, base);

      int type = 0;

      bool long_ = false;
      bool unsigned_ = false;
      if (starts_with(p, "llu") || starts_with(p, "llU") ||
          starts_with(p, "LLu") || starts_with(p, "LLU") ||
          starts_with(p, "ull") || starts_with(p, "uLL") ||
          starts_with(p, "Ull") || starts_with(p, "ULL")) {
        type = UNSIGNED + LONG + LONG;
        long_ = unsigned_ = true;
        p += 3;
      } else if (strncasecmp(p, "lu", 2) == 0 ||
                 strncasecmp(p, "ul", 2) == 0) {
        type = UNSIGNED + LONG;
        long_ = unsigned_ = true;
        p += 2;
      } else if (starts_with(p, "ll") || starts_with(p, "LL")) {
        type = LONG + LONG;
        long_ = true;
        p += 2;
      } else if (*p == 'L' || *p == 'l') {
        type = LONG;
        long_ = true;
        p++;
      } else if (*p == 'U' || *p == 'u') {
        type = UNSIGNED;
        unsigned_ = true;
        p++;
      }

      struct type *ty = ty_int;
      if (base == 10) {
        if (long_ && unsigned_) {
          ty = ty_ulong;
        } else if (long_) {
          ty = ty_long;
        } else if (unsigned_) {
          ty = (val >> 32) ? ty_ulong : ty_uint;
        } else {
          ty = (val >> 31) ? ty_long : ty_int;
        }
      } else {
        if (long_ && unsigned_) {
          ty = ty_ulong;
        } else if (long_) {
          ty = (val >> 63) ? ty_ulong : ty_long;
        } else if (unsigned_) {
          ty = (val >> 32) ? ty_ulong : ty_uint;
        } else if (val >> 63) {
          ty = ty_ulong;
        } else if (val >> 32) {
          ty = ty_long;
        } else if (val >> 31) {
          ty = ty_uint;
        } else {
          ty = ty_int;
        }
      }

      cur = new_token(TK_NUM, cur, q, p - q);
      cur->val = val;
      cur->len = p - q;
      cur->str = strndup(q, cur->len);
      cur->type = copy_type(ty);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  convert_ident_to_reserved(head.next);

  for (struct token *t = head.next; t; t = t->next) t->file_num = file_num;
  add_line_info(head.next);

  return head.next;
}

char *readfile(char *path) {
  size_t size;
  FILE *f = fopen(path, "r");
  if (!f) {
    error("cannot open %s: %s", path, strerror(errno));
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    error("%s: fseek: %s", path, strerror(errno));
  }

  size = ftell(f);

  if (fseek(f, 0, SEEK_SET) != 0) {
    error("%s: fseek: %s", path, strerror(errno));
  }

  char *buf = calloc(1, size + 2);

  fread(buf, size, 1, f);

  if (size == 0 || buf[size - 1] != '\n') buf[size++] = '\n';

  buf[size] = '\0';
  fclose(f);

  return buf;
}

void remove_backslash(char *input) {
  char *q = input;
  char *p = input;
  int cnt = 0;
  while (*p) {
    if (starts_with(p, "\\\n")) {
      p += 2;
      cnt++;
    } else if (*p == '\n') {
      *q++ = *p++;
      for (; cnt > 0; cnt--) {
        *q++ = '\n';
      }
    } else {
      *q++ = *p++;
    }
  }

  *q = '\0';
}

char **get_input_files() {
  return input_files;
}

bool is_loaded_file(char *file) {
  if (!input_files) return false;

  for (int i = 0; input_files[i]; i++) {
    if (strlen(file) == strlen(input_files[i]) &&
        !strncmp(input_files[i], file, strlen(file))) {
      return true;
    }
  }
  return false;
}

struct token *tokenize_file(char *file) {
  char *input = readfile(file);

  remove_backslash(input);

  if (!is_loaded_file(file)) {
    input_files = realloc(input_files, sizeof(char *) * (file_num + 2));
    input_files[file_num++] = file;
    input_files[file_num] = NULL;
  }

  struct token *token = tokenize(file, input, file_num);
  return token;
}

