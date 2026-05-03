#include <stddef.h>
#include <stdio.h>

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

int parser_match_imp(Parser *parser, Lexeme lexeme)
{
    if (parser_peek(parser).kind == lexeme) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}
Token match(Parser *parser, ...)
{
    va_list args;
    va_start(args, parser);
    Lexeme arg;
    while ((arg = va_arg(args, Lexeme)) != Lex_Invalid) {
        Token current = parser->tokens[parser->cursor];
        if (parser_match_imp(parser, arg)) {
            return current;
        }
    }
    return (Token) {
        .kind = Lex_Invalid 
    };
}

void parser_expect(Parser *parser, Lexeme lexeme)
{
    if (parser_match(parser, lexeme).kind == Lex_Invalid) {
        Token current = parser_peek(parser);
        Token prev    = parser_prev(parser);
        printf("Expected '%s' after '%s' at %zu:%zu, got '%s'\n",
                lex_print(lexeme),
                lex_print(prev.kind),
                prev.row,
                prev.col,
                lex_print(current.kind));
        assert(0 && "Exepected failed");
    }
}

//////////////////////////////////////////////////////////

Node parser_parse(Parser *parser, Areno *areno)
{
    Node program = parse_funcdef(parser, areno);
    return program;
}

Node parse_funcdef(Parser *parser, Areno *areno)
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
    
    Node body = parse_block(parser, areno);
    
    return (Node) {
        .kind = NodeKind_Funcdef,
        .funcdef = (Node_Funcdef) {
            .name       = sv_copy(&funcname.ident, areno),
            .args       = args,
            .statements = body.statements,
        }
    };
}

Node parse_block(Parser *parser, Areno *areno)
{
    parser_expect(parser, Lex_Open_brace);

    Node block = (Node) {
        .kind = NodeKind_Block,
        .statements = (Node_Block) {
            .count = 0,
            .items = (Node*) areno_alloc(areno, sizeof(Node) * BUF_SIZE),
        },
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_EOF) {
        if (current.kind == Lex_Close_brace) {
            break;
        } else if (current.kind == Lex_Ident) {
            Token next = parser_lookahead(parser, 1);
            if (next.kind == Lex_Equal) {
                // x = ...; -- reassign
                Node node = parse_assignation(parser, areno);
                block.statements.items[block.statements.count++] = node;
            } else if (next.kind == Lex_Open_bracket) {
                // fn(); -- function call -> expression
                Node node = parse_expression(parser, areno);
                block.statements.items[block.statements.count++] = node;
                // to allow call of funciton raw like "printf(...);"
                parser_match(parser, Lex_Semicolon);
            } else if (next.kind == Lex_Colon_Colon) {
                // fn :: (): type {} -- function definition
                Node node = parse_funcdef(parser, areno);
                block.statements.items[block.statements.count++] = node;
            } else {
                // maybe an expr ?
                Node expr = parse_expression(parser, areno);
                block.statements.items[block.statements.count++] = expr;
            }
        } else if (current.kind == Lex_let) {
            // let x = ...;
            Node node = parse_assignation(parser, areno);
            block.statements.items[block.statements.count++] = node;
            parser_match(parser, Lex_Semicolon);
        } else { // ...
            Node expr = parse_expression(parser, areno);
            block.statements.items[block.statements.count++] = expr;
            parser_match(parser, Lex_Semicolon);
        }
    }

    parser_expect(parser, Lex_Close_brace);
    printf("[DEBUG] Parsed a block, %zu statements\n", block.statements.count);

    return block;
}

Node parse_assignation(Parser *parser, Areno *areno)
{
    parser_match(parser, Lex_let);
    Token ident = parser_peek(parser);
    parser_expect(parser, Lex_Ident);

    parser_expect(parser, Lex_Equal);
    Node expr = parse_expression(parser, areno);

    Node node = (Node) {
        .kind = NodeKind_Assignation,
        .assignement = (Node_Assignement) {
            .ident = sv_copy(&ident.ident, areno),
            .value = expr.expression,
        },
    };

    return node;
}

Node parse_expression(Parser *parser, Areno *areno)
{
    return parse_addition(parser, areno);
}

// x + y - 23
Node parse_addition(Parser *parser, Areno *areno)
{
    Node lhs = parse_mul(parser, areno);

    Token current;
    while ((current = parser_match(parser, Lex_Plus, Lex_Minus)).kind != Lex_Invalid) {
        Node rhs = parse_mul(parser, areno);

        Expr *binop = (Expr*) areno_alloc(areno, sizeof(Expr));
        *binop = (Expr) {
            .kind = Expr_Binary,
            .binary_op = (Binary_Op) {
                .lhs     = lhs,
                .rhs     = rhs,
                .operand = current.kind,
            }
        };
        Node add = (Node) {
            .kind = NodeKind_Expression,
            .expression = binop,
        };

        lhs = add;
    }
    return lhs;
}

// x * y / z % 23
Node parse_mul(Parser *parser, Areno *areno)
{
    Node lhs = parse_terminal(parser, areno);

    Token current;
    while ((current = parser_match(parser, Lex_Mul, Lex_Divide, Lex_Modulo)).kind != Lex_Invalid) {
        Node rhs= parse_terminal(parser, areno);

        Expr *binop = (Expr*) areno_alloc(areno, sizeof(Expr));
        *binop = (Expr) {
            .kind = Expr_Binary,
            .binary_op = (Binary_Op) {
                .lhs     = lhs,
                .rhs     = rhs,
                .operand = current.kind,
            }
        };
        Node mul = (Node) {
            .kind = NodeKind_Expression,
            .expression = binop,
        };

        lhs = mul;
    }
    return lhs;
}

// x | "snoup" | 223 | ( ... ) | func(...)
Node parse_terminal(Parser *parser, Areno *areno)
{
    Node node = (Node) {
        .kind = NodeKind_Expression,
    };

    Token current = parser_peek(parser);
    switch (current.kind) {
        case Lex_Ident: {
            // func(...)
            if (parser_lookahead(parser, 1).kind == Lex_Open_bracket) {
                node = parse_funcall(parser, areno);
                break;
            }

            // x
            Expr *expr = (Expr*) areno_alloc(areno, sizeof(Expr));
            *expr = (Expr) {
                .kind  = Expr_Ident,
                .ident = sv_copy(&current.ident, areno)
            };
            node.expression = expr;

            parser_advance(parser);
        } break;
        case Lex_Number: {
            Expr *expr = (Expr*) areno_alloc(areno, sizeof(Expr));
            *expr = (Expr) {
                .kind = Expr_Number,
                .number = current.number,
            };
            node.expression = expr;

            parser_advance(parser);
        } break;
        case Lex_Open_bracket: { // ( expr )
            parser_expect(parser, Lex_Open_bracket);
            node = parse_expression(parser, areno);
            parser_expect(parser, Lex_Close_bracket);
        } break;
        case Lex_String: {
            // "snoup"
            Expr *expr = (Expr*) areno_alloc(areno, sizeof(Expr));
            *expr = (Expr) {
                .kind  = Expr_String,
                .ident = sv_copy(&current.string, areno)
            };
            node.expression = expr;

            parser_advance(parser);
        } break;
        default:
            printf("ERROR at %zu:%zu: Expected expression, found: '%s'\n",
                    current.row,
                    current.col,
                    lex_print(current.kind));
            assert(0 && "[TODO] ERROR HANDLING");
    }

    return node;
}

// func(...)
Node parse_funcall(Parser *parser, Areno *areno)
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

    Node node = (Node) {
        .kind = NodeKind_Expression,
        .expression = funcall,
    };

    while (!parser_match(parser, Lex_Close_bracket).kind) {
        Node arg = parse_expression(parser, areno);
        funcall->funcall.args.items[funcall->funcall.args.count++] = arg;

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
            printf("Name: %*s\n",
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
                    printf("Name: %*s\n", (int)arg.name.len, arg.name.items);
                    for (int i = 0; i < level + 3; i++) printf(" ");
                    printf("Type: %*s\n", (int)arg.type.len, arg.type.items);
                }
            }
            // body
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Body:\n");
            if (node->funcdef.statements.count <= 0) {
                for (int i = 0; i < level + 2; i++) printf(" ");
                printf("(no body)\n");
            } else {
                for (size_t i = 0; i < node->funcdef.statements.count; i++) {
                    dump_node(&node->funcdef.statements.items[i], level + 2);
                }
            }
        } break;

        case NodeKind_Block: {
            for (size_t i = 0; i < node->statements.count; i++) {
                dump_node(&node->statements.items[i], level);
            }
        } break;

        case NodeKind_Expression: {
            dump_expression(node->expression, level);
        } break;

        case NodeKind_Assignation: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Assignation:\n");

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %*s\n",
                    (int)node->assignement.ident.len,
                    node->assignement.ident.items);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Value:\n");
            dump_expression(node->assignement.value, level + 2);
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
            printf("Name: %*s\n", (int)expr->funcall.name.len, expr->funcall.name.items);

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

            dump_node(&expr->binary_op.lhs, level + 1);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->binary_op.operand));

            dump_node(&expr->binary_op.rhs, level + 1);

            break;
        case Expr_Ident:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Ident: %*s\n", (int) expr->ident.len, expr->ident.items);
            break;
        case Expr_String:
            for (int i = 0; i < level; i++) printf(" ");
            printf("String: %*s\n", (int) expr->str.len, expr->str.items);
            break;
        case Expr_Unary:
        case Expr_Invalid:
            assert(0 && "[TODO] dump_expression");
            break;
    }
}
