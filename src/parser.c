#include "parser.h"
#include "lexer.h"
#include <stddef.h>
#include <stdio.h>

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
        printf("Expected '%s', got '%s'\n",
                lex_print(lexeme),
                lex_print(parser_peek(parser).kind));
        assert(0 && "Exepected failed");
    }
}

Node *parser_parse(Parser *parser, Areno *areno)
{
    Node *root = (Node*) areno_alloc(areno, sizeof(Node));
    root->kind = NodeKind_Block;
    root->statements = (Node_Block) {
        .count = 0,
        .items = (Node*) areno_alloc(areno, sizeof(Node) * 1024),
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_EOF) {
        printf("[DEBUG] Parsing: %s %zu:%zu\n", lex_print(current.kind), current.row, current.col);
        if (current.kind == Lex_Ident) {
            Token next = parser_lookahead(parser, 1);
            if (next.kind == Lex_Equal) {
                // x = ...; -- reassign
                Node *node = parse_assignation(parser, areno);
                root->statements.items[root->statements.count++] = *node;
            } else if (next.kind == Lex_Open_bracket) {
                // fn(); -- function call
                assert(0 && "[TODO] parse function call");
            } else if (next.kind == Lex_Colon_Colon) {
                // fn :: (): type {} -- function definition
                assert(0 && "[TODO] parse function def");
            } else {
                // maybe an expr ?
                Node *expr = parse_expression(parser, areno);
                root->statements.items[root->statements.count++] = *expr;
            }
        } else if (current.kind == Lex_let) {
            // let x = ...;
            Node *node = parse_assignation(parser, areno);
            root->statements.items[root->statements.count++] = *node;
        } else { // ...
            Node *expr = parse_expression(parser, areno);
            root->statements.items[root->statements.count++] = *expr;
        }

    }
    printf("[DEBUG] end parsing, %zu statements\n", root->statements.count);

    return root;
}

Node *parse_assignation(Parser *parser, Areno *areno)
{
    parser_match(parser, Lex_let);
    Token ident = parser_peek(parser);
    parser_advance(parser);
    assert(ident.kind == Lex_Ident && "Expected Identifier");

    parser_expect(parser, Lex_Equal);
    Node *expr = parse_expression(parser, areno);

    Node *node = (Node*) areno_alloc(areno, sizeof(Node));
    node->kind = NodeKind_Assignation;
    node->assignement = (Node_Assignement) {
        .ident = sv_copy(&ident.ident, areno),
        .value = &expr->expression,
    };

    parser_expect(parser, Lex_Semicolon);

    return node;
}

Node *parse_expression(Parser *parser, Areno *areno)
{
    Node *node = (Node*) areno_alloc(areno, sizeof(Node));
    node->kind = NodeKind_Expression;
    node->expression = *parse_addition(parser, areno);
    return node;
}

Expr *parse_addition(Parser *parser, Areno *areno)
{
    Expr *lhs = parse_mul(parser, areno);

    Token current;
    while ((current = parser_match(parser, Lex_Plus, Lex_Minus)).kind != Lex_Invalid) {
        Expr *rhs = parse_mul(parser, areno);

        Expr *expr      = (Expr *) areno_alloc(areno, sizeof(Expr));

        expr->kind      = Expr_Binary;
        expr->binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };

        lhs = expr;
    }
    return lhs;
}

Expr *parse_mul(Parser *parser, Areno *areno)
{
    Expr *lhs = parse_terminal(parser, areno);

    Token current;
    while ((current = parser_match(parser, Lex_Mul, Lex_Divide, Lex_Modulo)).kind != Lex_Invalid) {
        Expr *rhs = parse_terminal(parser, areno);

        Expr *expr      = (Expr *) areno_alloc(areno, sizeof(Expr));

        expr->kind      = Expr_Binary;
        expr->binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };
        
        lhs = expr;
    }
    return lhs;
}

Expr *parse_terminal(Parser *parser, Areno *areno)
{

    Expr *expr = (Expr *) areno_alloc(areno, sizeof(Expr));

    Token current = parser_peek(parser);
    switch (current.kind) {
        case Lex_Number: {
            expr->kind = Expr_Number;
            expr->number = current.number;

            parser_advance(parser);
            break;
        }
        case Lex_Ident: {
            expr->kind  = Expr_Ident;
            expr->ident = sv_copy(&current.string, areno);

            parser_advance(parser);
            break;
        }
        case Lex_Open_bracket: { // ( expr )
            parser_expect(parser, Lex_Open_bracket);
            expr = &parse_expression(parser, areno)->expression;
            parser_expect(parser, Lex_Close_bracket);
            break;
        }
        default:
            printf("Error, found lexeme: '%s'\n", lex_print(current.kind));
            assert(0 && "[TODO] parse_terminal");
            break;
    }
    return expr;
}

void dump_node(Node *node, int level)
{
    switch (node->kind) {
        case NodeKind_Block: {
            for (size_t i = 0; i < node->statements.count; i++) {
                dump_node(&node->statements.items[i], level);
            }
        } break;
        case NodeKind_Expression: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Expr:\n");
            dump_expression(&node->expression, level + 1);
        } break;
        case NodeKind_Assignation: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Assignation:\n");

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %*s\n", (int)node->assignement.ident.len, node->assignement.ident.items);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Value:\n");
            dump_expression(node->assignement.value, level + 2);

        } break;
    }
}

void dump_expression (Expr *expr, int level)
{
    switch (expr->kind) {
        case Expr_Number:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Number: %d\n", expr->number);
            break;
        case Expr_Binary:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Binary:\n");

            dump_expression(expr->binary_op.lhs, level + 1);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->binary_op.operand));

            dump_expression(expr->binary_op.rhs, level + 1);

            break;
        case Expr_Ident:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Ident: %*s\n", (int) expr->ident.len, expr->ident.items);
            break;
        case Expr_Unary:
        case Expr_String:
        case Expr_Invalid:
            assert(0 && "[TODO] dump_expression");
            break;
    }
}
