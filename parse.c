#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcc.h"

struct function *functions;
struct var *locals;
struct var *globals;

struct function *funcdef();
struct type *declarator(struct type *);
struct type *funcdef_args(struct token *, struct type *);
struct type *type_suffix(struct token *, struct type *);
struct node *compound_stmt();
struct node *assign();
struct node *stmt();
struct node *expr();
struct node *equality();
struct node *relational();
struct node *add();
struct node *mul();
struct node *unary();
struct node *postfix();
struct node *primary();

struct node *funcall(struct token *token);

#define MAX_LEN (int) (256)

void print_param(struct var *params, bool is_next, struct function *fn) {
    for (struct var *v = params; v; v = v->next) {
        if (is_next)
            printf("| ");
        else
            printf("  ");

        if (v->next)
            printf("|-Param %s\n", v->name);
        else {
            if (fn->stmt)
                printf("|-Param %s\n", v->name);
            else
                printf("`-Param %s\n", v->name);
        }
    }
}

void print_globals(bool has_function) {
    for (struct var *v = globals; v; v = v->next) {
        if (v->next)
            printf("|-Param %s\n", v->name);
        else {
            if (has_function)
                printf("|-Param %s\n", v->name);
            else
                printf("`-Param %s\n", v->name);
        }
    }
}

void print_stmt(struct node *n,
                bool is_next_stmt,
                bool is_next_node,
                char *prefix) {
    if (!n) return;

    char *local_prefix;
    char *scope_prefix;
    char buf[MAX_LEN];

    local_prefix = strndup(prefix, MAX_LEN);
    scope_prefix = strndup(prefix, MAX_LEN);

    if (n->next || is_next_node) {
        is_next_stmt = true;
        snprintf(buf, MAX_LEN, "%s |", local_prefix);
    } else {
        is_next_stmt = false;
        snprintf(buf, MAX_LEN, "%s `", local_prefix);
    }

    local_prefix = strndup(buf, MAX_LEN);

    switch (n->kind) {
        case ND_RETURN:
            printf("%s-Return\n", local_prefix);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
            print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
            break;
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
            printf("%s-Calc '%s'\n", local_prefix, n->str);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
            print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
            print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
            break;
        case ND_NUM:
            printf("%s-Num '%d'\n", local_prefix, n->val);
            break;
        case ND_VAR:
            printf("%s-Var '%s'\n", local_prefix, n->str);
            break;
        case ND_ADDR:
            printf("%s-Addr '%s'\n", local_prefix, n->str);
            break;
        case ND_DEREF:
            printf("%s-Deref '%s'\n", local_prefix, n->str);
            break;
        case ND_EXPR_STMT:
            printf("%s-ExprStmt\n", local_prefix);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
            print_stmt(n->lhs, is_next_stmt, false, scope_prefix);
            break;
        case ND_ASSIGN:
            printf("%s-Assign '%s'\n", local_prefix, n->str);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
            print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
            print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
            break;
        case ND_IF:
            printf("%s-If '%s'\n", local_prefix, n->str);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
            print_stmt(n->cond, is_next_stmt, true, scope_prefix);
            print_stmt(n->then, is_next_stmt, true, scope_prefix);
            print_stmt(n->els, is_next_stmt, false, scope_prefix);
        case ND_FOR:
            printf("%s-Loop '%s'\n", local_prefix, n->str);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
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
            printf("%s-Cond '%s'\n", local_prefix, n->str);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }

            scope_prefix = strndup(buf, MAX_LEN);
            print_stmt(n->lhs, is_next_stmt, true, scope_prefix);
            print_stmt(n->rhs, is_next_stmt, false, scope_prefix);
            break;
        case ND_BLOCK:
            printf("%s-Block '%s'\n", local_prefix, n->str);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
            for (struct node *stmt = n->body; stmt; stmt = stmt->next) {
                if (stmt->next == NULL) {
                    print_stmt(stmt, is_next_stmt, false, scope_prefix);
                } else {
                    print_stmt(stmt, is_next_stmt, true, scope_prefix);
                }
            }
            break;
        case ND_FUNCALL:

            printf("%s-Funcall '%s'\n", local_prefix, n->str);
            if (is_next_node) {
                snprintf(buf, MAX_LEN, "%s |", scope_prefix);
            } else {
                snprintf(buf, MAX_LEN, "%s  ", scope_prefix);
            }
            scope_prefix = strndup(buf, MAX_LEN);
            for (struct node *arg = n->args; arg; arg = arg->next) {
                if (arg->next == NULL) {
                    print_stmt(arg, is_next_stmt, false, scope_prefix);
                } else {
                    print_stmt(arg, is_next_stmt, true, scope_prefix);
                }
            }
            break;
        default:
            printf("%s-none\n", local_prefix);
            break;
    }
}

void print_compound_stmt(struct node *stmt, bool is_next) {
    if (!stmt) return;

    char buf[MAX_LEN];
    char *global_prefix;
    char *prefix;
    if (is_next) {
        global_prefix = "| ";
    } else {
        global_prefix = "  ";
    }
    if (stmt) {
        prefix = strndup(global_prefix, MAX_LEN);
        printf("%s`-CompoundStmt\n", prefix);
        snprintf(buf, MAX_LEN, "%s ", prefix);

        prefix = buf;

        for (struct node *n = stmt->body; n; n = n->next) {
            if (n->next == NULL) {
                print_stmt(n, true, false, prefix);
            } else {
                print_stmt(n, true, true, prefix);
            }
        }
    }
}

void print_ast(struct program *pr) {
    struct function *last;

    bool has_function = pr->functions ? true : false;
    print_globals(has_function);

    for (struct function *fn = pr->functions; fn; fn = fn->next) last = fn;

    for (struct function *fn = pr->functions; fn; fn = fn->next) {
        if (fn == last) {
            printf("`-Function '%s'\n", fn->name);
            print_param(fn->params, false, fn);
            print_compound_stmt(fn->stmt, false);
        } else {
            printf("|-Function '%s'\n", fn->name);
            print_param(fn->params, true, fn);
            print_compound_stmt(fn->stmt, true);
        }
    }
    printf("\n");
}

void skip(struct token *token, char *op) {
    if (!equal(token, op)) {
        error_at(token->loc, "expected '%s', but op is '%s'", op, token->str);
    }

    tk = token->next;
}

void expect(char *op) {
    if (tk->kind != TK_RESERVED || !equal(tk, op)) {
        error_at(tk->loc, "'%s'ではありません", op);
    }
    tk = tk->next;
}

int expect_number() {
    if (tk->kind != TK_NUM) {
        error_at(tk->loc, "数ではありません");
    }
    int val = tk->val;
    tk      = tk->next;
    return val;
}

int get_number(struct token *tok) {
    if (tok->kind != TK_NUM) {
        error_at(tok->loc, "expected an number");
    }
    return tok->val;
}

char *get_ident(struct token *tok) {
    if (tok->kind != TK_IDENT) {
        error_at(tok->loc, "expected an identifier");
    }

    return strndup(tok->str, tok->len);
}

struct var *find_var(struct token *tok) {
    for (struct var *var = locals; var; var = var->next) {
        if (strncmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    for (struct var *var = globals; var; var = var->next) {
        if (strncmp(var->name, tok->str, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}

bool at_eof() {
    return tk->kind == TK_EOF;
}

struct node *new_node(enum node_kind kind, struct token *token) {
    struct node *n = calloc(1, sizeof(struct node));
    n->kind        = kind;
    n->str         = token->str;
    return n;
}

struct node *new_node_binary(enum node_kind kind,
                             struct node *lhs,
                             struct node *rhs) {
    struct node *n = calloc(1, sizeof(struct node));
    n->kind        = kind;
    n->lhs         = lhs;
    n->rhs         = rhs;
    return n;
}

struct node *new_node_num(int val) {
    struct node *n = calloc(1, sizeof(struct node));
    n->kind        = ND_NUM;
    n->val         = val;
    return n;
}

struct node *new_node_var(struct var *var, struct type *ty) {
    struct node *n = calloc(1, sizeof(struct node));
    n->kind        = ND_VAR;
    n->var         = var;
    n->str         = var->name;
    n->type        = ty;
    return n;
}

struct node *new_node_unary(enum node_kind kind,
                            struct node *expr,
                            struct token *token) {
    struct node *n = new_node(kind, token);
    n->lhs         = expr;
    return n;
}

struct node *new_node_expr(struct token *tk) {
    struct node *n = new_node(ND_EXPR_STMT, tk);
    n->lhs         = expr();
    return n;
}

struct node *new_add(struct node *lhs, struct node *rhs) {
    add_type(lhs);
    add_type(rhs);

    // num + num
    if (is_integer(lhs->type) && is_integer(rhs->type)) {
        return new_node_binary(ND_ADD, lhs, rhs);
    }

    // num + ptr to ptr + num
    if (!lhs->type->ptr_to && rhs->type->ptr_to) {
        struct node *tmp;
        tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    // num * sizeof(type)
    rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->type->ptr_to->size));
    return new_node_binary(ND_ADD, lhs, rhs);
}

struct node *new_sub(struct node *lhs, struct node *rhs) {
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->type) && is_integer(rhs->type)) {
        return new_node_binary(ND_SUB, lhs, rhs);
    }

    // ptr - num
    if (lhs->type->ptr_to && rhs->type->kind == INT) {
        rhs = new_node_binary(
            ND_MUL, rhs, new_node_num(lhs->type->ptr_to->size));
        return new_node_binary(ND_SUB, lhs, rhs);
    }

    // ptr - ptr
    if (lhs->type->ptr_to && rhs->type->ptr_to) {
        struct node *sub = new_node_binary(ND_SUB, lhs, rhs);
        return new_node_binary(
            ND_DIV, sub, new_node_num(lhs->type->ptr_to->size));
    }

    error_at(tk->loc, "invalid operands");
    return NULL;
}

struct var *new_lvar(struct type *type) {
    struct var *v = calloc(1, sizeof(struct var));
    v->name       = type->name;
    v->next       = locals;
    v->type       = type;
    v->is_local   = true;
    locals        = v;
    return v;
}

char *gvar_name() {
    char *buf      = calloc(24, sizeof(char));
    static int inc = 0;
    snprintf(buf, 24, ".LC%d", inc++);
    return buf;
}
struct var *new_gvar(struct type *type) {
    struct var *v = calloc(1, sizeof(struct var));
    v->name       = type->name;
    v->next       = globals;
    v->type       = type;
    v->is_local   = false;
    globals       = v;
    v->data       = NULL;
    return v;
}

struct var *new_string_literal(char *data, int len) {
    struct type *type = array_to(ty_char, len);
    struct var *v     = new_gvar(type);
    v->name           = gvar_name();
    v->data           = data;
    return v;
}

struct type *typespec(struct token *tok) {
    if (consume("int")) {
        return copy_type(ty_int);
    }

    if (consume("char")) {
        return copy_type(ty_char);
    }

    return NULL;
}

struct function *funcdef(struct type *type) {
    locals = NULL;

    struct function *fn = calloc(1, sizeof(struct function));

    fn->name = type->name;
    fn->type = type->return_type;

    for (struct type *ty = type->params; ty; ty = ty->next) {
        new_lvar(ty);
    }

    fn->params = locals;

    fn->stmt   = compound_stmt();
    fn->locals = locals;

    return fn;
}

struct type *declarator(struct type *base) {
    struct type *type = base;

    while (consume("*")) {
        type = pointer_to(type);
    }
    // ident
    if (tk->kind != TK_IDENT) {
        error_at(tk->loc, "expected a variable name");
    }

    char *type_name = get_ident(tk);

    type       = type_suffix(tk->next, type);
    type->name = type_name;

    return type;
}

struct type *type_suffix(struct token *tok, struct type *type) {
    tk = tok;

    if (consume("(")) {
        return funcdef_args(tk, type);
    }

    if (consume("[")) {
        int size = 0;
        if (!equal(tk, "]")) {
            size = get_number(tk);
            tk   = tk->next;
        }
        skip(tk, "]");
        type = type_suffix(tk, type);
        type = array_to(type, size);
        return type;
    }

    return type;
}

struct type *funcdef_args(struct token *tok, struct type *type) {

    struct type head = {};
    struct type *cur = &head;
    while (!equal(tk, ")")) {
        if (cur != &head) {
            skip(tk, ",");
        }
        struct type *ty;
        ty = typespec(tk);
        ty = declarator(ty);

        cur = cur->next = copy_type(ty);
    }
    skip(tk, ")");

    type->return_type = type;
    type->params      = head.next;
    return type;
}

// declaration = typespec
//   ( declarator ( "=" expr )? ( "," declarator ( "=" expr)? )* )? ";"
struct node *declaration() {

    struct node *node;
    struct var *var;

    struct type *base = typespec(tk);

    struct node head = {};
    struct node *cur = &head;
    int count        = 0;
    while (!equal(tk, ";")) {
        if (0 < count++) {
            skip(tk, ",");
        }

        struct type *type = declarator(base);

        var = new_lvar(type);

        if (consume("=")) {
            struct node *lvar = new_node_var(var, var->type);
            if (type->kind == ARRAY) {
                if (equal(tk, "{")) {
                    int cnt = 0;
                    tk      = tk->next;
                    struct token *start;
                    while (!equal(tk, "}")) {
                        if (cnt) skip(tk, ",");
                        start = tk;
                        struct node *deref =
                            new_node_unary(ND_DEREF,
                                           new_add(lvar, new_node_num(cnt)),
                                           start);
                        node = new_node(ND_EXPR_STMT, start);
                        node->lhs =
                            new_node_binary(ND_ASSIGN, deref, unary());
                        cur = cur->next = node;
                        cnt++;
                    }
                    if (type->array_size == 0) {
                        type->array_size = cnt;
                    }
                    skip(tk, "}");
                    if (type->array_size && type->array_size < cnt) {
                        error_at(start->loc,
                                 "Excess elements in array initializer\n");
                    }
                }
                if (tk->kind == TK_STR) {
                    struct token *start = tk;

                    for (int i = 0; i < tk->len; i++) {
                        struct node *deref = new_node_unary(
                            ND_DEREF, new_add(lvar, new_node_num(i)), start);
                        node      = new_node(ND_EXPR_STMT, start);
                        node->lhs = new_node_binary(
                            ND_ASSIGN, deref, new_node_num(tk->str[i]));
                        cur = cur->next = node;
                    }

                    if (type->array_size == 0) {
                        type->array_size = start->len;
                    }

                    if (type->array_size < start->len) {
                        error_at(
                            start->loc,
                            "Initializer-string for char array is too long\n");
                    }
                    tk = tk->next;
                }
            } else {
                node      = new_node(ND_EXPR_STMT, tk);
                node->lhs = new_node_binary(ND_ASSIGN, lvar, assign());
                cur = cur->next = node;
            }
        }
    }
    skip(tk, ";");

    node       = new_node(ND_BLOCK, tk);
    node->body = head.next;
    return node;
}

struct node *assign() {
    struct node *n = equality();
    if (consume("=")) {
        n      = new_node_binary(ND_ASSIGN, n, assign());
        n->str = "=";
    }

    return n;
}

struct node *compound_stmt() {
    struct node *n = NULL;

    if (!equal(tk, "{")) return NULL;

    n      = new_node(ND_BLOCK, tk);
    n->str = "{}";

    skip(tk, "{");

    struct node head = {};
    struct node *cur = &head;

    while (!equal(tk, "}")) {
        if (equal(tk, "int") || equal(tk, "char")) {
            cur = cur->next = declaration();
        } else {
            cur = cur->next = stmt();
        }
        add_type(cur);
    }

    n->body = head.next;
    skip(tk, "}");

    return n;
}

struct node *expr() {
    return assign();
}

struct node *stmt() {
    struct node *n;

    if (equal(tk, "return")) {
        tk = tk->next;
        n  = new_node_binary(ND_RETURN, expr(), NULL);

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
            tk     = tk->next;
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
        return compound_stmt();
    }

    n = new_node_expr(tk);
    expect(";");

    return n;
}

struct node *equality() {
    struct node *n = relational();
    while (true) {
        if (consume("==")) {
            n      = new_node_binary(ND_EQ, n, relational());
            n->str = "==";
        } else if (consume("!=")) {
            n      = new_node_binary(ND_NE, n, relational());
            n->str = "!=";
        } else {
            return n;
        }
    }
}

struct node *relational() {
    struct node *n = add();

    while (true) {
        if (consume("<")) {
            n      = new_node_binary(ND_LT, n, add());
            n->str = "<";
        } else if (consume("<=")) {
            n      = new_node_binary(ND_LE, n, add());
            n->str = "<=";
        } else if (consume(">")) {
            n      = new_node_binary(ND_GT, n, add());
            n->str = ">";
        } else if (consume(">=")) {
            n      = new_node_binary(ND_GE, n, add());
            n->str = ">=";
        } else {
            return n;
        }
    }
}

struct node *add() {
    struct node *n = mul();
    while (true) {
        if (consume("+")) {
            n      = new_add(n, mul());
            n->str = "+";
        } else if (consume("-")) {
            n      = new_sub(n, mul());
            n->str = "-";
        } else {
            return n;
        }
    }
}

struct node *mul() {
    struct node *n = unary();
    while (true) {
        if (consume("*")) {
            n      = new_node_binary(ND_MUL, n, unary());
            n->str = "*";
        } else if (consume("/")) {
            n      = new_node_binary(ND_DIV, n, unary());
            n->str = "/";
        } else {
            return n;
        }
    }
}

struct node *unary() {

    if (tk->kind == TK_SIZEOF) {
        skip(tk, "sizeof");
        struct node *node = unary();
        add_type(node);
        return new_node_num(node->type->size);
    }

    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node_binary(ND_SUB, new_node_num(0), unary());
    }

    if (consume("*")) {
        return new_node_unary(ND_DEREF, unary(), tk);
    }
    if (consume("&")) {
        return new_node_unary(ND_ADDR, unary(), tk);
    }
    return postfix();
}

struct node *postfix() {
    struct node *n = primary();

    if (n->kind != ND_NUM) {
        while (consume("[")) {
            struct token *start = tk;
            struct node *index  = expr();
            skip(tk, "]");
            n = new_node_unary(ND_DEREF, new_add(n, index), start);
        }
    }
    return n;
}

struct node *primary() {
    struct node *n;

    if (equal(tk, "(") && equal(tk->next, "{")) {
        n       = new_node(ND_EXPR_STMT, tk);
        tk      = tk->next;
        n->body = compound_stmt();

        skip(tk, ")");

        return n;
    }

    if (consume("(")) {
        n = expr();
        expect(")");
        return n;
    }

    struct token *tok = consume_ident();
    if (tok) {
        if (equal(tk, "(")) {
            return funcall(tok);
        }

        struct var *var = find_var(tok);
        if (!var) {
            error_at(tok->loc, "変数%sは定義されていません", tok->str);
        }

        return new_node_var(var, var->type);
    }

    if (tk->kind == TK_STR) {
        struct var *v = new_string_literal(tk->str, tk->len);
        tk            = tk->next;
        return new_node_var(v, v->type);
    }

    return new_node_num(expect_number());
}

struct node *funcall(struct token *token) {

    struct node head = {};
    struct node *cur = &head;

    skip(tk, "(");

    while (!equal(tk, ")")) {
        if (cur != &head) skip(tk, ",");

        cur = cur->next = assign();
    }
    skip(tk, ")");

    struct node *n = new_node(ND_FUNCALL, token);
    n->str         = token->str;
    n->args        = head.next;

    return n;
}

// program = ( funcdef | declaration )*
// typespec = "int"
// funcdef = typespec declarator compound-stmt
// declarator = "*"* ident type-suffix
// type-suffix = ( "[" num "]" )*
//              | "(" funcdef-args? ")"
// funcdef-args = param ( "," param )*
// param = typespec declarator
// declaration = typespec declarator ( "=" initializer )? ( "," declarator (
// "=" initializer )? )* ";" initialize = expr | "{" unary ( "," unary )* "}"
// compound-stmt = "{" ( declaration | stmt )* "}" stmt = expr ";"
//      | compound-stmt
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
// unary = "sizeof" unary | ( "+" | "-" | "*" | "&" )? unary | postfix
// postfix = primary ( "[" expr "]" )*
// primary = "(" "{" compound-stmt "}" ")"
//         | num
//         | ident funcall-args?
//         | "(" expr ")"
//         | string-literal
// funcall = ident "(" funcall-args ")"
// funcall-args = assign ( "," assign )*

struct program *parse() {
    globals = NULL;

    struct function head = {};
    struct function *cur = &head;
    struct type *type;
    while (!at_eof()) {

        type = typespec(tk);
        type = declarator(type);

        if (equal(tk, "{")) {
            cur = cur->next = funcdef(type);
            continue;
        }

        if (equal(tk, ";")) {
            struct var *gvar = new_gvar(type);
        } else {
            struct var *gvar = new_gvar(type);
            while (!equal(tk, ";")) {
                if (consume("=")) {
                    if (type->kind == ARRAY && type->ptr_to->kind == CHAR) {
                        gvar->data = tk->str;
                        tk         = tk->next;
                        goto check_comma;
                    }

                    struct node *node = assign();

                    if (node->kind == ND_NUM) {
                        gvar->data          = calloc(1, type->size);
                        *(int *) gvar->data = node->val;
                    }
                    if (node->kind == ND_VAR) {
                        gvar->data = node->var->name;
                    }
                    if (node->kind == ND_ADDR) {
                        gvar->data = node->lhs->var->name;
                    }
                    // tk = tk->next;
                }
            check_comma:
                if (consume(",")) {
                    type = declarator(type);
                    gvar = new_gvar(type);
                }
            }
        }
        skip(tk, ";");
    }

    struct program *programs = calloc(1, sizeof(struct program));
    programs->functions      = head.next;
    programs->globals        = globals;
    return programs;
}
