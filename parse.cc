#include <cstdio>
#include <cstring>

#include <string>

#include "kcc.h"

std::vector<function *> functions;
var *locals;

void print_param(var *params, bool is_next, function *fn) {
  for (var *v = params; v; v = v->next) {
    if (is_next)
      printf("| ");
    else
      printf("  ");

    if (v->next)
      printf("|-Param %s\n", v->name.c_str());
    else {
      if (fn->stmt)
        printf("|-Param %s\n", v->name.c_str());
      else
        printf("`-Param %s\n", v->name.c_str());
    }
  }
}

void print_stmt(node *n, bool is_next_stmt, bool is_next_node,
                std::string prefix) {
  if (!n)
    return;

  std::string local_prefix = prefix;
  if (n->next || is_next_node) {
    is_next_stmt = true;
    local_prefix += " |";
  } else {
    is_next_stmt = false;
    local_prefix += " `";
  }

  std::string scope_prefix = prefix;
  switch (n->kind) {
  case ND_RETURN:
    printf("%s-Return\n", local_prefix.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
    printf("%s-Calc '%s'\n", local_prefix.c_str(), n->str.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
    print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_NUM:
    printf("%s-Num '%d'\n", local_prefix.c_str(), n->val);
    break;
  case ND_VAR:
    printf("%s-Var '%s'\n", local_prefix.c_str(), n->str.c_str());
    break;
  case ND_ADDR:
    printf("%s-Addr '%s'\n", local_prefix.c_str(), n->str.c_str());
    break;
  case ND_DEREF:
    printf("%s-Deref '%s'\n", local_prefix.c_str(), n->str.c_str());
    break;
  case ND_EXPR_STMT:
    printf("%s-ExprStmt\n", local_prefix.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_ASSIGN:
    printf("%s-Assign '%s'\n", local_prefix.c_str(), n->str.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
    print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_IF:
    printf("%s-If '%s'\n", local_prefix.c_str(), n->str.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    print_stmt(n->cond, is_next_stmt, true, scope_prefix);
    print_stmt(n->then, is_next_stmt, true, scope_prefix);
    print_stmt(n->els, is_next_stmt, false, scope_prefix);
  case ND_FOR:
    printf("%s-Loop '%s'\n", local_prefix.c_str(), n->str.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    print_stmt(n->init, is_next_stmt, true, scope_prefix);
    print_stmt(n->cond, is_next_stmt, true, scope_prefix);
    print_stmt(n->inc, is_next_stmt, true, scope_prefix);
    print_stmt(n->then, is_next_stmt, false, scope_prefix);
    break;
  case ND_EQ:
  case ND_NE:
  case ND_LE:
  case ND_LT:
  case ND_GE:
  case ND_GT:
    printf("%s-Cond '%s'\n", local_prefix.c_str(), n->str.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";

    print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
    print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
    break;
  case ND_BLOCK:
    printf("%s-Block '%s'\n", local_prefix.c_str(), n->str.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    for (node *stmt = n->body; stmt; stmt = stmt->next) {
      if (stmt->next == nullptr) {
        print_stmt(stmt, is_next_stmt, false, scope_prefix);
      } else {
        print_stmt(stmt, is_next_stmt, true, scope_prefix);
      }
    }
    break;
  case ND_FUNCALL:

    printf("%s-Funcall '%s'\n", local_prefix.c_str(), n->str.c_str());
    scope_prefix += (is_next_stmt) ? " |" : "  ";
    for (node *arg = n->args; arg; arg = arg->next) {
      if (arg->next == nullptr) {
        print_stmt(arg, is_next_stmt, false, scope_prefix);
      } else {
        print_stmt(arg, is_next_stmt, true, scope_prefix);
      }
    }
    break;
  default:
    printf("%s-none\n", local_prefix.c_str());
    break;
  }
}

void print_compound_stmt(node *stmt, bool is_next) {
  if (!stmt)
    return;

  std::string global_prefix;
  if (is_next) {
    global_prefix = "| ";
  } else {
    global_prefix = "  ";
  }
  std::string prefix;
  if (stmt) {
    prefix = global_prefix;
    printf("%s`-CompoundStmt\n", prefix.c_str());
    prefix += " ";

    for (node *n = stmt->body; n; n = n->next) {
      if (n->next == nullptr) {
        print_stmt(n, true, false, prefix);
      } else {
        print_stmt(n, true, true, prefix);
      }
    }
  }
}

void print_ast(std::vector<function *> &functions) {

  for (auto &&fn : functions) {
    if (fn == functions.back()) {
      printf("`-Function '%s'\n", fn->name.c_str());
      print_param(fn->params, false, fn);
      print_compound_stmt(fn->stmt, false);
    } else {
      printf("|-Function '%s'\n", fn->name.c_str());
      print_param(fn->params, true, fn);
      print_compound_stmt(fn->stmt, true);
    }
  }
  printf("\n");
}

node *assign();
node *stmt();
node *expr();
node *equality();
node *relational();
node *add();
node *mul();
node *unary();
node *primary();

node *funcall(token *token);

void skip(token *token, std::string const &op) {
  if (!equal(token, op)) {
    error_at(token->pos, "expected '%s', but op is '%s'", op.c_str(),
             token->str.c_str());
  }

  tk = token->next;
}

void expect(const char *op) {
  if (tk->kind != TK_RESERVED || tk->str.size() != std::strlen(op) ||
      tk->str.compare(op)) {
    error_at(tk->pos, "'%s'ではありません", op);
  }
  tk = tk->next;
}

int expect_number() {
  if (tk->kind != TK_NUM) {
    error_at(tk->pos, "数ではありません");
  }
  int val = tk->val;
  tk = tk->next;
  return val;
}

void print_lvar(var *lv) {
  printf("lvar name(%s)\n", lv->name.c_str());
  printf("lvar offset(%d)\n", lv->offset);
  printf("lvar next(0x%p)\n", lv->next);
}

var *find_lvar(token *tok) {
  for (var *var = locals; var; var = var->next) {
    if (var->name.size() == tok->str.size() &&
        var->name.compare(tok->str) == 0) {
      return var;
    }
  }
  return nullptr;
}

bool at_eof() { return tk->kind == TK_EOF; }

node *new_node(node_kind kind, token *token) {
  node *n = new node;
  n->kind = kind;
  n->str = token->str;
  return n;
}

node *new_node_binary(node_kind kind, node *lhs, node *rhs) {
  node *n = new node;
  n->kind = kind;
  n->lhs = lhs;
  n->rhs = rhs;
  return n;
}

node *new_node_num(int val) {
  node *n = new node;
  n->kind = ND_NUM;
  n->val = val;
  n->str = std::to_string(val);
  return n;
}

node *new_node_lvar(var *var) {
  node *n = new node;
  n->kind = ND_VAR;
  n->var = var;
  n->str = var->name;
  return n;
}

node *new_node_unary(node_kind kind, token *token) {
  node *n = new_node(kind, token);
  n->lhs = unary();
  return n;
}

node *new_node_expr(token *tk) {
  node *n = new_node(ND_EXPR_STMT, tk);
  n->lhs = expr();
  return n;
}

var *new_lvar(token *token) {
  var *v = new var;
  v->name = token->str;
  // v->offset = locals->offset + 8;
  v->next = locals;
  locals = v;
  return v;
}

node *assign() {
  node *n = equality();
  if (consume("=")) {
    n = new_node_binary(ND_ASSIGN, n, assign());
    n->str = "=";
  }

  return n;
}

var *funcdef_args() {
  var *var = nullptr;

  skip(tk, "(");

  while (!equal(tk, ")")) {
    var = new_lvar(tk);

    tk = tk->next;
    if (equal(tk, ","))
      tk = tk->next;
  }
  skip(tk, ")");

  return var;
}

node *compound_stmt() {
  node *n = nullptr;

  if (!equal(tk, "{"))
    return nullptr;

  skip(tk, "{");

  n = new_node(ND_BLOCK, tk);
  n->str = "{}";

  node head = {};
  node *cur = &head;

  while (!equal(tk, "}")) {
    cur = cur->next = stmt();
  }

  n->body = head.next;
  skip(tk, "}");

  return n;
}

node *expr() { return assign(); }

node *stmt() {
  node *n;

  if (equal(tk, "return")) {
    tk = tk->next;
    n = new_node_binary(ND_RETURN, expr(), nullptr);

    skip(tk, ";");
    return n;
  }

  // "if" "(" expr ")" stmt ("else" stmt)?
  if (equal(tk, "if")) {
    n = new_node(ND_IF, tk);

    skip(tk->next, "(");
    n->cond = expr();
    skip(tk, ")");

    n->then = stmt();

    if (equal(tk, "else")) {
      tk = tk->next;
      n->els = stmt();
    }

    return n;
  }

  // "while" "(" expr ")" stmt
  if (equal(tk, "while")) {
    n = new_node(ND_FOR, tk);
    skip(tk->next, "(");
    n->cond = expr();
    skip(tk, ")");
    n->then = stmt();

    return n;
  }

  // "for" "(" expr? ";" expr? ";" expr? ")" stmt
  if (equal(tk, "for")) {
    n = new_node(ND_FOR, tk);
    skip(tk->next, "(");
    if (!equal(tk, ";")) {
      n->init = new_node_expr(tk);
    }
    skip(tk, ";");

    if (!equal(tk, ";")) {
      n->cond = expr();
    }
    skip(tk, ";");

    if (!equal(tk, ")")) {
      n->inc = new_node_expr(tk);
    }
    skip(tk, ")");

    n->then = stmt();

    return n;
  }

  if (equal(tk, "{")) {
    tk = tk->next;
    n = new_node(ND_BLOCK, tk);
    n->str = "{}";

    node head = {};
    node *cur = &head;

    while (!equal(tk, "}")) {
      cur = cur->next = stmt();
    }

    n->body = head.next;

    skip(tk, "}");
    return n;
  }

  n = new_node_expr(tk);
  expect(";");
  // if (equal(tk, ";")) {
  //   skip(tk, ";");
  // } else {
  // }

  return n;
}

node *equality() {
  node *n = relational();
  while (true) {
    if (consume("==")) {
      n = new_node_binary(ND_EQ, n, relational());
      n->str = "==";
    } else if (consume("!=")) {
      n = new_node_binary(ND_NE, n, relational());
      n->str = "!=";
    } else {
      return n;
    }
  }
}

node *relational() {
  node *n = add();

  while (true) {
    if (consume("<")) {
      n = new_node_binary(ND_LT, n, add());
      n->str = "<";
    } else if (consume("<=")) {
      n = new_node_binary(ND_LE, n, add());
      n->str = "<=";
    } else if (consume(">")) {
      n = new_node_binary(ND_GT, n, add());
      n->str = ">";
    } else if (consume(">=")) {
      n = new_node_binary(ND_GE, n, add());
      n->str = ">=";
    } else {
      return n;
    }
  }
}

node *add() {
  node *n = mul();
  while (true) {
    if (consume("+")) {
      n = new_node_binary(ND_ADD, n, mul());
      n->str = "+";
    } else if (consume("-")) {
      n = new_node_binary(ND_SUB, n, mul());
      n->str = "-";
    } else {
      return n;
    }
  }
}

node *mul() {
  node *n = unary();
  while (true) {
    if (consume("*")) {
      n = new_node_binary(ND_MUL, n, unary());
      n->str = "*";
    } else if (consume("/")) {
      n = new_node_binary(ND_DIV, n, unary());
      n->str = "/";
    } else {
      return n;
    }
  }
}

node *primary() {
  node *n;
  if (consume("(")) {
    n = expr();
    expect(")");
    n->str = "(" + n->str + ")";
    return n;
  }

  token *tok = consume_ident();
  if (tok) {
    if (equal(tk, "("))
      return funcall(tok);

    var *lvar = find_lvar(tok);
    n = new_node(ND_VAR, tok);
    if (lvar) {
      n->var = lvar;
    } else {
      n->var = new_lvar(tok);
    }
    return n;
  }

  return new_node_num(expect_number());
}

node *unary() {
  if (consume("+")) {
    return unary();
  }
  if (consume("-")) {
    return new_node_binary(ND_SUB, new_node_num(0), unary());
  }

  if (consume("*")) {
    return new_node_unary(ND_DEREF, tk);
  }
  if (consume("&")) {
    return new_node_unary(ND_ADDR, tk);
  }
  return primary();
}

node *funcall(token *token) {

  node head = {};
  node *cur = &head;

  skip(tk, "(");

  if (tk->kind == TK_IDENT || tk->kind == TK_NUM) {
    while (!equal(tk, ")")) {
      if (cur != &head)
        skip(tk, ",");

      cur = cur->next = assign();
    }
  }
  skip(tk, ")");

  node *n = new_node(ND_FUNCALL, token);
  n->str = token->str;
  n->args = head.next;

  return n;
}

// funcdef = ident "(" funcdef-args ")" compound-stmt
function *funcdef() {
  function *fn = nullptr;
  node *n = nullptr;
  token *func_name = nullptr;

  locals = nullptr;

  // ident
  func_name = consume_ident();
  if (func_name) {
    fn = new function;
    fn->name = func_name->str;
    fn->params = funcdef_args();
    fn->stmt = compound_stmt();
    fn->locals = locals;
  } else {
  }

  return fn;
}

// program = funcdef*
// funcdef = ident "(" funcdef-args ")" compound-stmt
// funcdef-args = param ( "," param )*
// param = ident
// compound-stmt = "{" stmt* "}"
// stmt = expr ";"
//      | "{" stmt* "}"
//      | "if" "(" expr ")" stmt ( "else" stmt )?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
// expr = assign
// assign = equality ( "=" assign )?
// equality = relational ( "==" relational | "!=" relational )*
// relational = add ( "<" add | "<=" add | ">" add | ">=" add )*
// add = mul ( "+" mul | "-" mul )*
// mul = unary ( "*" unary | "/" unary )*
// unary = ( "+" | "-" | "*" | "&" )? unary | primary
// primary = num | ident funcall-args?  | "(" expr ")"
// funcall = ident "(" funcall-args ")"
// funcall-args = assign ( "," assign )*
void program() {
  int i = 0;
  while (!at_eof()) {
    functions.push_back(funcdef());
  }

  // print_ast(functions);
}
