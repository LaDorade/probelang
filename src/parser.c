#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

#define MAX_ARGS 10
#define BUF_SIZE 1024

String_View sv_copy(Areno *areno, String_View *src)
{
    size_t len  = src->len;
    char *items = (char *) areno_alloc(areno, len);
    memccpy(items, src->items, 0, len);

    return (String_View) {
        .len   = len,
        .items = items,
    };
}

Token parser_peek(Parser *parser)
{
    return parser->tokens[parser->cursor];
}
Token parser_prev(Parser *parser)
{
    if (parser->cursor <= 0) {
        return (Token) {
            .kind = Lex_Invalid,
        };
    }
    return parser->tokens[parser->cursor - 1];
}
Token parser_lookahead(Parser *parser, size_t nb)
{
    for (size_t i = 0; i <= nb; i++) {
        if ((parser->tokens[parser->cursor + i]).kind == Lex_EOF) {
            return (Token) {
                .kind = Lex_EOF
            };
        }
    }
    return parser->tokens[parser->cursor + nb];
}

void parser_advance(Parser *parser)
{
    if (parser_peek(parser).kind == Lex_EOF) return;
    parser->cursor += 1;
}

#define parser_match(...) __parser_match_impl(__VA_ARGS__, Lex_Invalid)
Token __parser_match_impl(Parser *parser, ...)
{
    va_list args;
    va_start(args, parser);
    Lexeme arg;
    while ((arg = va_arg(args, Lexeme)) != Lex_Invalid) {
        Token current = parser->tokens[parser->cursor];
        if (parser_peek(parser).kind == arg) {
            parser_advance(parser);
            return current;
        }
    }
    return (Token) {
        .kind = Lex_Invalid 
    };
}

#define parser_expect(parser, lexeme) do {if (!__parser_expect_impl(parser, lexeme)) return NULL; } while (0);
bool __parser_expect_impl(Parser *parser, Lexeme lexeme)
{
    if (parser_match(parser, lexeme).kind == Lex_Invalid) {
        Token current          = parser_peek(parser);
        Token prev             = parser_prev(parser);
        const char* lexeme_str = lex_print(lexeme);
        const char* prev_str   = lex_print(prev.kind);
        const char* curr_str   = lex_print(current.kind);

        char *err_msg = areno_printf(&parser->areno,
                "Expected '%s' after '%s' at %zu:%zu, got '%s'\n",
                lexeme_str,
                prev_str,
                current.row,
                current.col,
                curr_str);

        parser_prepare_error(parser, err_msg, Parse_Err_UnexpectedToken);
        return false;
    }
    return true;
}

// Take current token, and wrap error
void parser_prepare_error(Parser *parser, char *msg, Parse_Error_Kind kind)
{
    Token current = parser_peek(parser);
    parser->err = (Parse_Error) {
        .code      = msg == NULL ? Parse_Err_AllocError : kind,
        .formatted = msg == NULL ? "Error alloc memory" : msg,
        .row = current.row,
        .col = current.col,
    };
}

void parser_free(Parser *parser)
{
    areno_free(&parser->areno);
}

Node *parser_create_nodes(Parser *parser, size_t nb)
{
    Node *nodes = (Node*) areno_alloc(&parser->areno, sizeof(Node) * nb);
    memset(nodes, 0, sizeof(Node) * nb);
    return nodes;
}
Node *parser_create_node(Parser *parser, Node_Kind kind)
{
    Node *node = (Node*) parser_create_nodes(parser, 1);
    node->kind = kind;
    return node;
}
Expr *parser_create_expr(Parser *parser, Expr_Kind kind)
{
    Expr *expr = (Expr*) areno_alloc(&parser->areno, sizeof(Expr));
    memset(expr, 0, sizeof(Expr));
    expr->kind = kind;
    return expr;
}

//////////////////////////////////////////////////////////

Node *parser_parse(Parser *parser)
{
    Node *program = parser_create_node(parser, NodeKind_Block);
    program->as.block = (Node_Block) {
        .count = 0,
        .items = parser_create_nodes(parser, BUF_SIZE),
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_EOF) {
        Node *func = parse_statement(parser);
        if (func == NULL) return NULL;
        program->as.block.items[program->as.block.count++] = *func;
    }
    return program;
}

Node *parse_statement(Parser *parser)
{
    Token current = parser_peek(parser);
    if (current.kind == Lex_Open_brace) { // { ... } -- block
        Node *node = parse_block(parser);
        if (node == NULL) return NULL;
        return node;
    } else if (current.kind == Lex_Ident) {
        Token next = parser_lookahead(parser, 1);
        if (next.kind == Lex_Equal) { // x = ...; -- reassign
            Node *node = parse_stmt_assign(parser);
            if (node == NULL) return NULL;
            parser_match(parser, Lex_Semicolon);
            return node;
        } else { // expr or funcall
            Node *expr = parse_expression(parser);
            if (expr == NULL) return NULL;
            // to allow call of funciton raw like "printf(...);"
            // or just raw expressions
            parser_match(parser, Lex_Semicolon);
            return expr;
        }
    } else if (current.kind == Lex_fun) {
        // fun xx: (...) -> ... = { ... }
        Node *node = parse_func_def(parser);
        if (node == NULL) return NULL;
        return node;
    } else if (current.kind == Lex_let || current.kind == Lex_const) { // const/let x = ...;
        Node *node = parse_stmt_assign(parser);
        if (node == NULL) return NULL;
        parser_match(parser, Lex_Semicolon);
        return node;
    } else if (current.kind == Lex_if) { // if ...
        Node *node = parse_stmt_if(parser);
        if (node == NULL) return NULL;
        return node;
    } else if (current.kind == Lex_while) { // while ...
        Node *node = parse_stmt_while(parser);
        if (node == NULL) return NULL;
        return node;
    }

    Node *expr = parse_expression(parser);
    if (expr == NULL) return NULL;
    parser_match(parser, Lex_Semicolon);
    return expr;
}

// func-decl = 'fun' ident ':' '(' ident ':' type { ',' ident ':' type } ')' '->' type '=' '{' { statements } '}';
Node *parse_func_def(Parser *parser)
{
    parser_expect(parser, Lex_fun);
    Token funcname = parser_peek(parser);
    parser_expect(parser, Lex_Ident);
    parser_expect(parser, Lex_Colon);

    // args
    parser_expect(parser, Lex_Open_bracket);

    Args args = {0};
    args.items = areno_alloc(&parser->areno, sizeof(Arg) * MAX_ARGS);
    while (!parser_match(parser, Lex_Close_bracket).kind) {
        Token arg_name = parser_peek(parser);
        parser_expect(parser, Lex_Ident);

        parser_expect(parser, Lex_Colon);
        Node *type = parse_type_expr(parser);
        if (type == NULL) return NULL;

        args.items[args.count++] = (Arg) {
            .name = sv_copy(&parser->areno, &arg_name.as.ident),
            .type = type,
        };

        // this allow trailing coma
        if (!parser_match(parser, Lex_Comma).kind) {
            parser_expect(parser, Lex_Close_bracket);
            break;
        }
    }

    parser_expect(parser, Lex_Arrow_Right);
    Node *type = parse_type_expr(parser);
    if (type == NULL) return NULL;

    parser_expect(parser, Lex_Equal);
    
    // expect a block
    Node *body = parse_block(parser);
    if (body == NULL) return NULL;
    
    Node *node = parser_create_node(parser, NodeKind_Funcdef);
    node->as.funcdef = (Node_Funcdef) {
        .name        = sv_copy(&parser->areno, &funcname.as.ident),
        .args        = args,
        .block       = body,
        .return_type_node = type,
    };
    return node;
}

Node *parse_block(Parser *parser)
{
    parser_expect(parser, Lex_Open_brace);

    Node *node = parser_create_node(parser, NodeKind_Block);
    node->as.block = (Node_Block) {
        .count = 0,
        .items = parser_create_nodes(parser, BUF_SIZE)
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_Close_brace) {
        if (current.kind == Lex_Close_brace) {
            break;
        } else if (current.kind == Lex_defer) {
            Node *defer = parse_bloc_defer(parser);
            if (defer == NULL) return NULL;
            node->as.block.items[node->as.block.count++] = *defer;
        } else {
            Node *stmt = parse_statement(parser);
            if (stmt == NULL) return NULL;
            node->as.block.items[node->as.block.count++] = *stmt;
        }
    }

    parser_expect(parser, Lex_Close_brace);

    return node;
}

Node *parse_bloc_defer(Parser *parser)
{
    parser_prepare_error(parser, "[TODO] block defer\n", Parse_Err_Todo);
    return NULL;
}

Node *parse_stmt_if(Parser *parser)
{
    Node *node = parser_create_node(parser, NodeKind_If);

    /* Parse if exmpression */
    parser_expect(parser, Lex_if);
    bool used_bracket = parser_match(parser, Lex_Open_bracket).kind != Lex_Invalid;
    Node *expr = parse_expression(parser);
    if (expr == NULL) return NULL;
    node->as.if_node.condition = expr;
    if (used_bracket) parser_expect(parser, Lex_Close_bracket);

    /* Parse if nodé */
    Node *if_node = parse_statement(parser);
    if (if_node == NULL) return NULL;
    node->as.if_node.ok_node = if_node;

    /* Parse else nodé */
    if (parser_match(parser, Lex_else).kind != Lex_Invalid) {
        Node *else_block = parse_statement(parser);
        if (else_block == NULL) return NULL;
        node->as.if_node.ko_node = else_block;
    }

    return node;
}

Node *parse_stmt_while(Parser *parser)
{
    Node *node = parser_create_node(parser, NodeKind_While);

    /* Parse while exmpression */
    parser_expect(parser, Lex_while);
    bool used_bracket = parser_match(parser, Lex_Open_bracket).kind != Lex_Invalid;
    Node *expr = parse_expression(parser);
    if (expr == NULL) return NULL;
    node->as.while_node.condition = expr;
    if (used_bracket) parser_expect(parser, Lex_Close_bracket);

    /* Parse while nodé */
    Node *while_node = parse_statement(parser);
    if (while_node == NULL) return NULL;
    node->as.while_node.ok_node = while_node;

    return node;
}

// type-expr     ::= term-optional [ '!' type-optional ] ;
// type-optional ::= [ '?' ] term-ident ;
Node *parse_type_expr(Parser *parser)
{
    Node *node = parser_create_node(parser, NodeKind_Type);

    Token current;
    bool nullable = parser_match(parser, Lex_Question).kind != Lex_Invalid;
    if (nullable && parser_peek(parser).kind != Lex_Ident) {
        current = parser_peek(parser);
        char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected identifier, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
        return NULL;
    }
    if ((current = parser_match(parser, Lex_Ident)).kind != Lex_Invalid) {
        node->as.type.success_set      = true;
        node->as.type.success.ident    = sv_copy(&parser->areno, &current.as.ident);
        node->as.type.success.nullable = nullable;
    }

    if (parser_match(parser, Lex_Bang).kind != Lex_Invalid) {
        bool nullable = parser_match(parser, Lex_Question).kind != Lex_Invalid;
        if (nullable && parser_peek(parser).kind != Lex_Ident) {
            current = parser_peek(parser);
            char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected identifier, found: '%s'\n",
                    current.row,
                    current.col,
                    lex_print(current.kind));
            parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
            return NULL;
        }
        if ((current = parser_match(parser, Lex_Ident)).kind != Lex_Invalid) {
            node->as.type.error_set      = true;
            node->as.type.error.ident    = sv_copy(&parser->areno, &current.as.ident);
            node->as.type.error.nullable = nullable;
        }
    }

    if (!node->as.type.success_set && !node->as.type.error_set) {
        current = parser_peek(parser);
        char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected type-expression, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
        return NULL;
    }

    return node;
}

// stmt-assign   ::= [ 'let' | 'const' ] term-ident [ ':' type-expr ] '=' expression | block ;
Node *parse_stmt_assign(Parser *parser)
{
    Token current = parser_match(parser, Lex_let, Lex_const, Lex_Ident);
    if (current.kind == Lex_Invalid) {
        current = parser_peek(parser);
        char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected assignation, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
        return NULL;
    }

    Assign_Kind assign_kind = AssignKind_Invalid;
    if (current.kind == Lex_let) {
        assign_kind = AssignKind_Let;
    } else if (current.kind == Lex_const) {
        assign_kind = AssignKind_Const;
    } else if (current.kind == Lex_Ident) {
        assign_kind = AssignKind_Reassign;
    } else {
        printf("[UNREACHABLE] stmt-assign");
        assert(0 && "[UNREACHABLE] stmt-assign");
    }

    Token ident = current;
    if (current.kind != Lex_Ident) {
        ident = parser_peek(parser);
        parser_expect(parser, Lex_Ident);
    }
    Node *type = NULL;
    if (parser_match(parser, Lex_Colon).kind != Lex_Invalid) {
        type = parse_type_expr(parser);
        if (type == NULL) return NULL;
    }

    parser_expect(parser, Lex_Equal);

    Node *expr = NULL;
    // parse as block
    if (parser_peek(parser).kind == Lex_Open_brace) {
        Node *block = parse_block(parser);
        if (block == NULL) return NULL;
        expr = block;
    } else { // parse as expression
        Node *node_expr = parse_expression(parser);
        if (node_expr == NULL) return NULL;
        expr = node_expr;
    }

    Node *node = parser_create_node(parser, NodeKind_Assignement);
    node->as.assignement = (Node_Assignement) {
        .kind       = assign_kind,
        .ident      = sv_copy(&parser->areno, &ident.as.ident),
        .value_node = expr,
        .type_node  = type
    };

    return node;
}

Node *parse_expression(Parser *parser)
{
    return parse_expr_equal(parser);
}

// x == y >= z
Node *parse_expr_equal(Parser *parser)
{
    Node *lhs = parse_expr_add(parser);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser,
            Lex_Equal_Equal, Lex_Not_Equal,
            Lex_Greater_Equal, Lex_Lower_Equal)
        ).kind != Lex_Invalid)
    {
        Node *rhs = parse_expr_add(parser);
        if (rhs == NULL) return NULL;

        Expr *expr = parser_create_expr(parser, Expr_Binary);
        expr->as.binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };

        Node *node = parser_create_node(parser, NodeKind_Expression);
        node->as.expression = expr;

        lhs = node;
    }
    return lhs;
}

// x + y - 23
Node *parse_expr_add(Parser *parser)
{
    Node *lhs = parse_expr_mul(parser);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser, Lex_Plus, Lex_Minus)).kind != Lex_Invalid) {
        Node *rhs = parse_expr_mul(parser);
        if (rhs == NULL) return NULL;

        Expr *expr = parser_create_expr(parser, Expr_Binary);
        expr->as.binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };

        Node *node = parser_create_node(parser, NodeKind_Expression);
        node->as.expression = expr;

        lhs = node;
    }
    return lhs;
}

// expr-mul     ::= expr-unary { ('*' | '/' | '%') expr-unary }
Node *parse_expr_mul(Parser *parser)
{
    Node *lhs = parse_expr_unary(parser);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser, Lex_Mul, Lex_Divide, Lex_Modulo)).kind != Lex_Invalid) {
        Node *rhs = parse_expr_unary(parser);
        if (rhs == NULL) return NULL;

        Expr *expr = parser_create_expr(parser, Expr_Binary);
        expr->as.binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };

        Node *node = parser_create_node(parser, NodeKind_Expression);
        node->as.expression = expr;

        lhs = node;
    }
    return lhs;
}

// expr-unary   ::= { '!' | '-' } expr-primary
Node *parse_expr_unary(Parser *parser)
{
    Token current = parser_match(parser, Lex_Minus, Lex_Bang);
    if (current.kind != Lex_Invalid) {

        Expr *expr = parser_create_expr(parser, Expr_Unary);
        expr->as.unary_op = (Unary_Op) {
            .operand = current.kind,
            .expr = parse_expr_primary(parser),
        };

        Node *node = parser_create_node(parser, NodeKind_Expression);
        if (node == NULL) return NULL;

        node->as.expression = expr;
        return node;
    } else {
        Node *node = parse_expr_primary(parser);
        if (node == NULL) return NULL;
        return node;
    }
}

// expr-primary ::= expr-funcall | '(' expression ')' | terminal
Node *parse_expr_primary(Parser *parser)
{
    Node *node = NULL;
    Token current = parser_peek(parser);
    if (current.kind == Lex_Ident && parser_lookahead(parser, 1).kind == Lex_Open_bracket) {
        node = parse_expr_funcall(parser);
        if (node == NULL) return NULL;
    } else if (current.kind == Lex_Open_bracket) {
        parser_expect(parser, Lex_Open_bracket);
        node = parse_expression(parser);
        if (node == NULL) return NULL;
        parser_expect(parser, Lex_Close_bracket);
    } else {
        node = parse_terminal(parser);
        if (node == NULL) return NULL;
    }
    if (node == NULL) return NULL;
    return node;
}

// terminal     ::= term-ident | term-string | term-number
Node *parse_terminal(Parser *parser)
{
    Node *node = NULL;
    Token current = parser_peek(parser);
    if (current.kind == Lex_Ident) {
        parser_expect(parser, Lex_Ident);
        node = parser_create_node(parser, NodeKind_Expression);
        // x
        Expr *expr = parser_create_expr(parser, Expr_Ident);
        expr->as.ident = sv_copy(&parser->areno, &current.as.ident);

        node->as.expression = expr;
    } else if (current.kind == Lex_String) {
        parser_expect(parser, Lex_String);
        node = parser_create_node(parser, NodeKind_Expression);
        // "snoup"
        Expr *expr = parser_create_expr(parser, Expr_String);
        expr->as.str = sv_copy(&parser->areno, &current.as.string);

        node->as.expression = expr;
    } else if (current.kind == Lex_Number) {
        parser_expect(parser, Lex_Number);
        node = parser_create_node(parser, NodeKind_Expression);
        // 23
        Expr *expr = parser_create_expr(parser, Expr_Number);
        expr->as.number = current.as.number;

        node->as.expression = expr;
    } else {
        char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected terminal, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
        return NULL;
    }

    if (node == NULL) return NULL;
    return node;
}

// expr-funcall ::= term-indent '(' [ expression { ',' expression } [ ',' ] ] ')' ;
Node *parse_expr_funcall(Parser *parser)
{
    Token function = parser_peek(parser);
    parser_expect(parser, Lex_Ident);

    parser_expect(parser, Lex_Open_bracket);

    Expr *expr = parser_create_expr(parser, Expr_Funcall);
    expr->as.funcall = (Funcall) {
        .name = sv_copy(&parser->areno, &function.as.string),
        .args = {
            .count = 0,
            .items = parser_create_nodes(parser, MAX_ARGS),
        }
    };

    Node *node = parser_create_node(parser, NodeKind_Expression);
    node->as.expression = expr;

    while (!parser_match(parser, Lex_Close_bracket).kind) {
        Node *arg = parse_expression(parser);
        if (arg == NULL) return NULL;
        expr->as.funcall.args.items[expr->as.funcall.args.count++] = *arg;

        // this allow trailing coma
        if (!parser_match(parser, Lex_Comma).kind) {
            parser_expect(parser, Lex_Close_bracket);
            break;
        }
    }

    return node;
}

void dump_node(Node *node, int level)
{
    switch (node->kind) {
        case NodeKind_Invalid:
        case NodeKind_LastNode:
            printf("[ERROR] Printing invalid node\n");
            abort();

        case NodeKind_Type: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Type: ");

            if (node->as.type.success_set) {
                if (node->as.type.success.nullable) {
                    printf("?");
                }
                printf("%.*s", (int) node->as.type.success.ident.len, node->as.type.success.ident.items);
            }
            if (node->as.type.error_set) {
                printf("!");
                if (node->as.type.error.nullable) {
                    printf("?");
                }
                printf("%.*s", (int) node->as.type.error.ident.len, node->as.type.error.ident.items);
            }
            printf("\n");
        } break;

        case NodeKind_Funcdef: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Function Def:\n");
            // name
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %.*s\n",
                    (int)node->as.funcdef.name.len,
                    node->as.funcdef.name.items);
            // args
            if (node->as.funcdef.args.count <= 0) {
                for (int i = 0; i < level + 2; i++) printf(" ");
                printf("(no arguments)\n");
            } else {
                for (size_t i = 0; i < node->as.funcdef.args.count; i++) {
                    Arg arg = node->as.funcdef.args.items[i];
                    for (int i = 0; i < level + 2; i++) printf(" ");
                    printf("Args %d\n", (int)i + 1);
                    for (int i = 0; i < level + 3; i++) printf(" ");
                    printf("Name: %.*s\n", (int)arg.name.len, arg.name.items);
                    dump_node(arg.type, level + 3);
                }
            }
            // type
            dump_node(node->as.funcdef.return_type_node, level + 1);
            // body
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Body:\n");
            dump_node(node->as.funcdef.block, level + 2);
        } break;

        case NodeKind_Assignement: {
            const char *kind = node->as.assignement.kind == AssignKind_Let
                ? "let"
                : node->as.assignement.kind == AssignKind_Const
                ? "const"
                : "reassign";
            for (int i = 0; i < level; i++) printf(" ");
            printf("Assignation (%s):\n", kind);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %.*s\n",
                    (int)node->as.assignement.ident.len,
                    node->as.assignement.ident.items);

            if (node->as.assignement.type_node != NULL && (node->as.assignement.type_node->as.type.success_set || node->as.assignement.type_node->as.type.error_set)) {
                dump_node(node->as.assignement.type_node, level + 1);
            }

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Value:\n");
            dump_node(node->as.assignement.value_node, level + 2);
        } break;

        case NodeKind_Block: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Block:\n");
            if (node->as.block.count <= 0) {
                for (int i = 0; i < level + 2; i++) printf(" ");
                printf("(no statements)\n");
            } else {
                for (size_t i = 0; i < node->as.block.count; i++) {
                    dump_node(&node->as.block.items[i], level + 1);
                }
            }
        } break;

        case NodeKind_If: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("If:\n");
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Cond:\n");
            dump_node(node->as.if_node.condition, level + 2);
            dump_node(node->as.if_node.ok_node, level + 1);
            if (node->as.if_node.ko_node != NULL) {
                for (int i = 0; i < level + 1; i++) printf(" ");
                printf("Else\n");
                dump_node(node->as.if_node.ko_node, level + 2);
            }
        } break;

        case NodeKind_While: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("While:\n");
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Cond:\n");
            dump_node(node->as.if_node.condition, level + 2);
            dump_node(node->as.if_node.ok_node, level + 1);
        } break;

        case NodeKind_Expression: {
            dump_expression(node->as.expression, level);
        } break;

    }
}

void dump_expression (Expr *expr, int level)
{
    switch (expr->kind) {
        case Expr_Funcall: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Funcall:\n");
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %.*s\n", (int)expr->as.funcall.name.len, expr->as.funcall.name.items);

            if (expr->as.funcall.args.count == 0) {
                for (int i = 0; i < level + 1; i++) printf(" ");
                printf("(no arguments)\n");
            } else {
                for (size_t i = 0; i < expr->as.funcall.args.count; i++) {
                    for (int i = 0; i < level + 1; i++) printf(" ");
                    printf("Arg %zu\n", i + 1);
                    dump_node(&expr->as.funcall.args.items[i], level + 2);
                }
            }
        } break;
        case Expr_Number:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Number: %d\n", expr->as.number);
            break;
        case Expr_Binary:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Binary:\n");

            dump_node(expr->as.binary_op.lhs, level + 1);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->as.binary_op.operand));

            dump_node(expr->as.binary_op.rhs, level + 1);

            break;
        case Expr_Ident:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Ident: %.*s\n", (int) expr->as.ident.len, expr->as.ident.items);
            break;
        case Expr_String:
            for (int i = 0; i < level; i++) printf(" ");
            printf("String: %.*s\n", (int) expr->as.str.len, expr->as.str.items);
            break;
        case Expr_Unary:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Unary:\n");

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->as.unary_op.operand));

            dump_node(expr->as.unary_op.expr, level + 1);

            break;
        case Expr_Invalid:
            assert(0 && "[INVALID] dump invalid expression");
            break;
    }
}
