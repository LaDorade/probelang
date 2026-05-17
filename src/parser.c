#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

#define MAX_ARGS 10
#define BUF_SIZE 1024

String_View sv_copy(String_View *src, Areno *areno)
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
        String_View line       = sv_get_line(parser->sv, prev.row);

        // TODO: use areno instead of malloc
        // Set areno inside the Parser ?
        char *err_msg = malloc(BUF_SIZE);
        if (err_msg == NULL) goto error;

        int cx = 0;
        cx += snprintf(err_msg + cx, BUF_SIZE - cx, "\n");
        if (cx < 0)     goto error;
        if (prev.kind != Lex_Invalid && prev.row != current.row) {
            cx += snprintf(err_msg + cx, BUF_SIZE - cx, ">  %.*s\n", (int)line.len, line.items);
            if (cx < 0) goto error;
        }
        line = sv_get_line(parser->sv, current.row);
        cx += snprintf(err_msg + cx, BUF_SIZE - cx, ">  %.*s\n", (int)line.len, line.items);
        if (cx < 0)     goto error;
        cx += snprintf(err_msg + cx, BUF_SIZE - cx, ">  %*c", (int)current.col, '^');
        if (cx < 0)     goto error;
        for (size_t i = 1; i < strlen(curr_str); i++) {
            cx += snprintf(err_msg + cx, BUF_SIZE - cx, "^");
            if (cx < 0) goto error;
        }
        cx += snprintf(err_msg + cx, BUF_SIZE - cx, "\n");
        if (cx < 0)     goto error;
        cx += snprintf(err_msg + cx, BUF_SIZE - cx, "\n");
        if (cx < 0)     goto error;

        snprintf(err_msg + cx, BUF_SIZE - cx,
                "Expected '%s' after '%s' at %zu:%zu, got '%s'\n",
                lexeme_str,
                prev_str,
                current.row,
                current.col,
                curr_str);
        if (cx < 0) goto error;
        parser->err = (Parse_Error) {
            .code = Parse_Err_UnexpectedToken,
            .formatted = err_msg,
            .row = current.row,
            .col = current.col,
        };
        return false;
error:
        parser->err = (Parse_Error) {
            .code = Parse_Err_AllocError,
            .formatted = "Error allocating space",
            .row = current.row,
            .col = current.col,
        };
        err_msg = "Error allocating space";
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////

Node *parser_parse(Parser *parser, Areno *areno)
{
    Node *program;
    program = areno_alloc(areno, sizeof(Node)),
    *program = (Node) {
        .kind = NodeKind_Block,
        .statements = (Node_Block) {
            .count = 0,
            .items = (Node*) areno_alloc(areno, sizeof(Node) * BUF_SIZE),
        },
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_EOF) {
        Node *func = parse_top_funcdef(parser, areno);
        if (func == NULL) return NULL;
        program->statements.items[program->statements.count++] = *func;
    }
    return program;
}

Node *parse_top_statement(Parser *parser, Areno *areno)
{
    Token current = parser_match(parser, Lex_let, Lex_const, Lex_Ident);
    if (current.kind == Lex_Invalid) {
        current = parser_peek(parser);
        printf("ERROR at %zu:%zu: Expected top-level-statement, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        assert(0 && "[TODO] ERROR HANDLING");
    } else if (current.kind == Lex_const || current.kind == Lex_let) {
        Node *node = parse_top_assign(parser, areno);
        if (node == NULL) return NULL;
        return node;
    } else if (current.kind == Lex_Ident){ // ident
        Node *node = parse_top_funcdef(parser, areno);
        if (node == NULL) return NULL;
        return node;
    }
    assert(0 && "[UNREACHABLE] top level statement");
    return NULL;
}

Node *parse_top_assign(Parser *parser, Areno *areno) 
{
    assert(0 && "[TODO] top level assign");
    return NULL;
}

Node *parse_top_funcdef(Parser *parser, Areno *areno)
{
    Token funcname = parser_peek(parser);
    parser_expect(parser, Lex_Ident);
    parser_expect(parser, Lex_Colon_Colon);

    // args
    parser_expect(parser, Lex_Open_bracket);

    Args args = {0};
    args.items = areno_alloc(areno, sizeof(Arg) * MAX_ARGS);
    while (!parser_match(parser, Lex_Close_bracket).kind) {
        Token arg_name = parser_peek(parser);
        parser_expect(parser, Lex_Ident);
        parser_expect(parser, Lex_Colon);

        Token arg_type = parser_peek(parser);
        parser_expect(parser, Lex_Ident);

        args.items[args.count++] = (Arg) {
            .name = sv_copy(&arg_name.ident, areno),
            .type = sv_copy(&arg_type.ident, areno),
        };

        // this allow trailing coma
        if (!parser_match(parser, Lex_Comma).kind) {
            parser_expect(parser, Lex_Close_bracket);
            break;
        }
    }

    // TODO: return type
    
    // expect a block
    Node *body = parse_block(parser, areno);
    if (body == NULL) return NULL;
    
    Node *function = areno_alloc(areno, sizeof(Node));
    *function = (Node) {
        .kind = NodeKind_Funcdef,
        .funcdef = (Node_Funcdef) {
            .name  = sv_copy(&funcname.ident, areno),
            .args  = args,
            .block = body,
        }
    };
    return function;
}

Node *parse_block(Parser *parser, Areno *areno)
{
    parser_expect(parser, Lex_Open_brace);

    Node *block = areno_alloc(areno, sizeof(Node));
    *block = (Node) {
        .kind = NodeKind_Block,
        .statements = (Node_Block) {
            .count = 0,
            .items = (Node*) areno_alloc(areno, sizeof(Node) * BUF_SIZE),
        },
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_Close_brace) {
        if (current.kind == Lex_Close_brace) {
            break;
        } else if (current.kind == Lex_defer) {
            assert(0 && "TODO: block defer");
        } else {
            Node *node = parse_statement(parser, areno);
            if (node == NULL) return NULL;
            block->statements.items[block->statements.count++] = *node;
        }
    }

    parser_expect(parser, Lex_Close_brace);

    return block;
}

Node *parse_block_defer(Parser *parser, Areno *areno)
{
    assert("[TODO] block defer");
    return NULL;
}

Node *parse_statement(Parser *parser, Areno *areno)
{
    Token current = parser_peek(parser);
    if (current.kind == Lex_Open_brace) { // { ... } -- block
        Node *node = parse_block(parser, areno);
        if (node == NULL) return NULL;
        return node;
    } else if (current.kind == Lex_Ident) {
        Token next = parser_lookahead(parser, 1);
        if (next.kind == Lex_Equal) { // x = ...; -- reassign
            Node *node = parse_stmt_assign(parser, areno);
            if (node == NULL) return NULL;
            parser_match(parser, Lex_Semicolon);
            return node;
        } else if (next.kind == Lex_Colon_Colon) {
            // fn :: (): type {} -- function definition
            Node *node = parse_top_funcdef(parser, areno);
            if (node == NULL) return NULL;
            return node;
        } else { // expr or funcall
            Node *expr = parse_expression(parser, areno);
            if (expr == NULL) return NULL;
            // to allow call of funciton raw like "printf(...);"
            // or just raw expressions
            parser_match(parser, Lex_Semicolon);
            return expr;
        }
    } else if (current.kind == Lex_let) { // let x = ...;
        Node *node = parse_stmt_assign(parser, areno);
        if (node == NULL) return NULL;
        parser_match(parser, Lex_Semicolon);
        return node;
    } else if (current.kind == Lex_if) { // if ...
        Node *node = parse_stmt_if(parser, areno);
        if (node == NULL) return NULL;
        return node;
    } else if (current.kind == Lex_while) { // while ...
        Node *node = parse_stmt_while(parser, areno);
        if (node == NULL) return NULL;
        return node;
    }

    Node *expr = parse_expression(parser, areno);
    if (expr == NULL) return NULL;
    parser_match(parser, Lex_Semicolon);
    return expr;
}

Node *parse_stmt_if(Parser *parser, Areno *areno)
{
    Node tmp = {
        .kind = NodeKind_If,
        .if_node = (Node_If) {
            .condition = NULL,
            .ok_node   = NULL,
            .ko_node   = NULL, // sem analysis must check if there is an else
        },
    };

    /* Parse if exmpression */
    parser_expect(parser, Lex_if);
    bool used_bracket = parser_match(parser, Lex_Open_bracket).kind != Lex_Invalid;
    Node *expr = parse_expression(parser, areno);
    if (expr == NULL) return NULL;
    tmp.if_node.condition = expr;
    if (used_bracket) parser_expect(parser, Lex_Close_bracket);

    /* Parse if nodé */
    Node *if_node = parse_statement(parser, areno);
    if (if_node == NULL) return NULL;
    tmp.if_node.ok_node = if_node;

    /* Parse else nodé */
    if (parser_match(parser, Lex_else).kind != Lex_Invalid) {
        Node *else_block = parse_statement(parser, areno);
        if (else_block == NULL) return NULL;
        tmp.if_node.ko_node = else_block;
    }

    Node *node = areno_alloc(areno, sizeof(Node));
    *node = tmp;
    return node;
}

Node *parse_stmt_while(Parser *parser, Areno *areno)
{
    Node tmp = {
        .kind = NodeKind_While,
        .while_node = (Node_While) {
            .condition = NULL,
            .ok_node   = NULL,
        },
    };

    /* Parse while exmpression */
    parser_expect(parser, Lex_while);
    bool used_bracket = parser_match(parser, Lex_Open_bracket).kind != Lex_Invalid;
    Node *expr = parse_expression(parser, areno);
    if (expr == NULL) return NULL;
    tmp.while_node.condition = expr;
    if (used_bracket) parser_expect(parser, Lex_Close_bracket);

    /* Parse while nodé */
    Node *while_node = parse_statement(parser, areno);
    if (while_node == NULL) return NULL;
    tmp.while_node.ok_node = while_node;

    Node *node = areno_alloc(areno, sizeof(Node));
    *node = tmp;
    return node;
}

// stmt-assign  ::= [ 'let' ] ident '=' expression | block
Node *parse_stmt_assign(Parser *parser, Areno *areno)
{
    parser_match(parser, Lex_let);
    Token ident = parser_peek(parser);
    parser_expect(parser, Lex_Ident);
    parser_expect(parser, Lex_Equal);

    Node *expr = NULL;
    // parse as block
    if (parser_peek(parser).kind == Lex_Open_brace) {
        Node *block = parse_block(parser, areno);
        if (block == NULL) return NULL;
        expr = block;
    } else { // parse as expression
        Node *node_expr = parse_expression(parser, areno);
        if (node_expr == NULL) return NULL;
        expr = node_expr;
    }

    Node *node = areno_alloc(areno, sizeof(Node));
    *node = (Node) {
        .kind = NodeKind_Assignement,
        .assignement = (Node_Assignement) {
            .ident = sv_copy(&ident.ident, areno),
            .value = expr,
        },
    };

    return node;
}

Node *parse_expression(Parser *parser, Areno *areno)
{
    return parse_expr_equal(parser, areno);
}

// x == y >= z
Node *parse_expr_equal(Parser *parser, Areno *areno)
{
    Node *lhs = parse_expr_add(parser, areno);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser,
            Lex_Equal_Equal, Lex_Not_Equal,
            Lex_Greater_Equal, Lex_Lower_Equal)
        ).kind != Lex_Invalid)
    {
        Node *rhs = parse_expr_add(parser, areno);
        if (rhs == NULL) return NULL;

        Expr *binop = (Expr*) areno_alloc(areno, sizeof(Expr));
        *binop = (Expr) {
            .kind = Expr_Binary,
            .binary_op = (Binary_Op) {
                .lhs     = lhs,
                .rhs     = rhs,
                .operand = current.kind,
            }
        };

        Node *comp = areno_alloc(areno, sizeof(Node));
        *comp = (Node) {
            .kind = NodeKind_Expression,
            .expression = binop,
        };

        lhs = comp;
    }
    return lhs;
}

// x + y - 23
Node *parse_expr_add(Parser *parser, Areno *areno)
{
    Node *lhs = parse_expr_mul(parser, areno);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser, Lex_Plus, Lex_Minus)).kind != Lex_Invalid) {
        Node *rhs = parse_expr_mul(parser, areno);
        if (rhs == NULL) return NULL;

        Expr *binop = (Expr*) areno_alloc(areno, sizeof(Expr));
        *binop = (Expr) {
            .kind = Expr_Binary,
            .binary_op = (Binary_Op) {
                .lhs     = lhs,
                .rhs     = rhs,
                .operand = current.kind,
            }
        };
        Node *add = areno_alloc(areno, sizeof(Node));
        *add = (Node) {
            .kind = NodeKind_Expression,
            .expression = binop,
        };

        lhs = add;
    }
    return lhs;
}

// expr-mul     ::= expr-unary { ('*' | '/' | '%') expr-unary }
Node *parse_expr_mul(Parser *parser, Areno *areno)
{
    Node *lhs = parse_expr_unary(parser, areno);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser, Lex_Mul, Lex_Divide, Lex_Modulo)).kind != Lex_Invalid) {
        Node *rhs = parse_expr_unary(parser, areno);
        if (rhs == NULL) return NULL;

        Expr *binop = (Expr*) areno_alloc(areno, sizeof(Expr));
        *binop = (Expr) {
            .kind = Expr_Binary,
            .binary_op = (Binary_Op) {
                .lhs     = lhs,
                .rhs     = rhs,
                .operand = current.kind,
            }
        };
        Node *mul = areno_alloc(areno, sizeof(Node));
        *mul = (Node) {
            .kind = NodeKind_Expression,
            .expression = binop,
        };

        lhs = mul;
    }
    return lhs;
}

// expr-unary   ::= { '!' | '-' } expr-primary
Node *parse_expr_unary(Parser *parser, Areno *areno)
{
    Node *node = NULL;
    Token current = parser_match(parser, Lex_Minus, Lex_Bang);
    if (current.kind != Lex_Invalid) {
        node = areno_alloc(areno, sizeof(Node));
        *node = (Node) {
            .kind = NodeKind_Expression,
        };

        Expr *expr = (Expr*) areno_alloc(areno, sizeof(Expr));
        *expr = (Expr) {
            .kind  = Expr_Unary,
            .unary_op = {
                .operand = current.kind,
                .expr = parse_expr_primary(parser, areno),
            },
        };
        node->expression = expr;
    } else {
        node = parse_expr_primary(parser, areno);
    }

    if (node == NULL) return NULL;
    return node;
}

// expr-primary ::= expr-funcall | '(' expression ')' | terminal
Node *parse_expr_primary(Parser *parser, Areno *areno)
{
    Node *node = NULL;
    Token current = parser_peek(parser);
    if (current.kind == Lex_Ident && parser_lookahead(parser, 1).kind == Lex_Open_bracket) {
        node = parse_expr_funcall(parser, areno);
        if (node == NULL) return NULL;
    } else if (current.kind == Lex_Open_bracket) {
        parser_expect(parser, Lex_Open_bracket);
        node = parse_expression(parser, areno);
        if (node == NULL) return NULL;
        parser_expect(parser, Lex_Close_bracket);
    } else {
        node = parse_terminal(parser, areno);
        if (node == NULL) return NULL;
    }
    if (node == NULL) return NULL;
    return node;
}

// terminal     ::= term-ident | term-string | term-number
Node *parse_terminal(Parser *parser, Areno *areno)
{
    Node *node = NULL;
    Token current = parser_peek(parser);
    if (current.kind == Lex_Ident) {
        node = areno_alloc(areno, sizeof(Node));
        *node = (Node) {
            .kind = NodeKind_Expression,
        };
        // x
        Expr *expr = (Expr*) areno_alloc(areno, sizeof(Expr));
        *expr = (Expr) {
            .kind  = Expr_Ident,
            .ident = sv_copy(&current.ident, areno)
        };
        node->expression = expr;
        parser_expect(parser, Lex_Ident);
    } else if (current.kind == Lex_String) {
        node = areno_alloc(areno, sizeof(Node));
        *node = (Node) {
            .kind = NodeKind_Expression,
        };
        // "snoup"
        Expr *expr = (Expr*) areno_alloc(areno, sizeof(Expr));
        *expr = (Expr) {
            .kind  = Expr_String,
            .ident = sv_copy(&current.string, areno)
        };
        node->expression = expr;
        parser_expect(parser, Lex_String);
    } else if (current.kind == Lex_Number) {
        node = areno_alloc(areno, sizeof(Node));
        *node = (Node) {
            .kind = NodeKind_Expression,
        };
        Expr *expr = (Expr*) areno_alloc(areno, sizeof(Expr));
        *expr = (Expr) {
            .kind = Expr_Number,
            .number = current.number,
        };
        node->expression = expr;
        parser_expect(parser, Lex_Number);
    } else {
        printf("ERROR at %zu:%zu: Expected terminal, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        assert(0 && "[TODO] ERROR HANDLING");
    }

    if (node == NULL) return NULL;
    return node;
}

// expr-funcall ::= term-indent '(' [ expression { ',' expression } [ ',' ] ] ')' ;
Node *parse_expr_funcall(Parser *parser, Areno *areno)
{
    Token function = parser_peek(parser);
    parser_expect(parser, Lex_Ident);

    parser_expect(parser, Lex_Open_bracket);

    Expr *funcall = (Expr*) areno_alloc(areno, sizeof(Expr));
    *funcall = (Expr) {
        .kind = Expr_Funcall,
        .funcall = (Funcall) {
            .name = sv_copy(&function.string, areno),
            .args = {
                .count = 0,
                .items = areno_alloc(areno, sizeof(Node) * MAX_ARGS),
            }
        }
    };

    Node *node = (Node*) areno_alloc(areno, sizeof(Node));
    *node = (Node) {
        .kind = NodeKind_Expression,
        .expression = funcall,
    };

    while (!parser_match(parser, Lex_Close_bracket).kind) {
        Node *arg = parse_expression(parser, areno);
        if (arg == NULL) return NULL;
        funcall->funcall.args.items[funcall->funcall.args.count++] = *arg;

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

        case NodeKind_Funcdef: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Function Def:\n");
            // name
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %.*s\n",
                    (int)node->funcdef.name.len,
                    node->funcdef.name.items);
            // args
            if (node->funcdef.args.count <= 0) {
                for (int i = 0; i < level + 2; i++) printf(" ");
                printf("(no arguments)\n");
            } else {
                for (size_t i = 0; i < node->funcdef.args.count; i++) {
                    Arg arg = node->funcdef.args.items[i];
                    for (int i = 0; i < level + 2; i++) printf(" ");
                    printf("Args %d\n", (int)i + 1);
                    for (int i = 0; i < level + 3; i++) printf(" ");
                    printf("Name: %.*s\n", (int)arg.name.len, arg.name.items);
                    for (int i = 0; i < level + 3; i++) printf(" ");
                    printf("Type: %.*s\n", (int)arg.type.len, arg.type.items);
                }
            }
            // body
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Body:\n");
            dump_node(node->funcdef.block, level + 2);
        } break;

        case NodeKind_Block: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Block:\n");
            if (node->statements.count <= 0) {
                for (int i = 0; i < level + 2; i++) printf(" ");
                printf("(no statements)\n");
            } else {
                for (size_t i = 0; i < node->statements.count; i++) {
                    dump_node(&node->statements.items[i], level + 1);
                }
            }
        } break;

        case NodeKind_If: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("If:\n");
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Cond:\n");
            dump_node(node->if_node.condition, level + 2);
            dump_node(node->if_node.ok_node, level + 1);
            if (node->if_node.ko_node != NULL) {
                for (int i = 0; i < level + 1; i++) printf(" ");
                printf("Else\n");
                dump_node(node->if_node.ko_node, level + 2);
            }
        } break;

        case NodeKind_While: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("While:\n");
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Cond:\n");
            dump_node(node->if_node.condition, level + 2);
            dump_node(node->if_node.ok_node, level + 1);
        } break;

        case NodeKind_Expression: {
            dump_expression(node->expression, level);
        } break;

        case NodeKind_Assignement: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Assignation:\n");

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %.*s\n",
                    (int)node->assignement.ident.len,
                    node->assignement.ident.items);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Value:\n");
            dump_node(node->assignement.value, level + 2);
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
            printf("Name: %.*s\n", (int)expr->funcall.name.len, expr->funcall.name.items);

            if (expr->funcall.args.count == 0) {
                for (int i = 0; i < level + 1; i++) printf(" ");
                printf("(no arguments)\n");
            } else {
                for (size_t i = 0; i < expr->funcall.args.count; i++) {
                    for (int i = 0; i < level + 1; i++) printf(" ");
                    printf("Arg %zu\n", i + 1);
                    dump_node(&expr->funcall.args.items[i], level + 2);
                }
            }
        } break;
        case Expr_Number:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Number: %d\n", expr->number);
            break;
        case Expr_Binary:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Binary:\n");

            dump_node(expr->binary_op.lhs, level + 1);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->binary_op.operand));

            dump_node(expr->binary_op.rhs, level + 1);

            break;
        case Expr_Ident:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Ident: %.*s\n", (int) expr->ident.len, expr->ident.items);
            break;
        case Expr_String:
            for (int i = 0; i < level; i++) printf(" ");
            printf("String: %.*s\n", (int) expr->str.len, expr->str.items);
            break;
        case Expr_Unary:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Unary:\n");

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->unary_op.operand));

            dump_node(expr->unary_op.expr, level + 1);

            break;
        case Expr_Invalid:
            assert(0 && "[TODO] dump_expression");
            break;
    }
}
