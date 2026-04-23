#include "parser.h"


Token parser_peek(Parser *parser)
{
    return parser->tokens[parser->cursor];
}
void parser_advance(Parser *parser)
{
    if (parser_peek(parser).kind == Lex_EOF) return;
    parser->cursor += 1;
}

int parser_match_imp(Parser *parser, Lexeme lexeme)
{
    Token current = parser->tokens[parser->cursor];
    if (current.kind == lexeme) {
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
        assert(0 && "Exepcted failed");
    }
}

Expr *parse_expression(Parser *parser, Areno *areno)
{
    Expr *expr = parse_addition(parser, areno);
    return expr;
}

Expr *parse_addition(Parser *parser, Areno *areno)
{
    Expr *lhs = parse_mul(parser, areno);

    Token current;
    while ((current = parser_match(parser, Lex_Plus, Lex_Minus)).kind != Lex_Invalid) {
        Expr *rhs = parse_mul(parser, areno);

        Binary_Op *bin_op = (Binary_Op *) areno_alloc(areno, sizeof(Binary_Op));

        bin_op->lhs     = lhs;
        bin_op->rhs     = rhs;
        bin_op->operand = current.kind;
        
        Expr *expr      = (Expr *) areno_alloc(areno, sizeof(Expr));
        expr->kind      = Expr_Binary;
        expr->binary_op = bin_op;

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

        Binary_Op *bin_op = (Binary_Op *) areno_alloc(areno, sizeof(Binary_Op));

        bin_op->lhs     = lhs;
        bin_op->rhs     = rhs;
        bin_op->operand = current.kind;
        
        Expr *expr      = (Expr *) areno_alloc(areno, sizeof(Expr));
        expr->kind      = Expr_Binary;
        expr->binary_op = bin_op;
        
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

            size_t len  = current.string.len;
            char *ident = (char *) areno_alloc(areno, len);
            memccpy(ident, current.string.items, 0, len);
            expr->ident = (String_View) {
                .items = ident,
                .len   = len,
            };

            parser_advance(parser);
            break;
        }
        case Lex_Open_bracket: { // ( expr )
            parser_expect(parser, Lex_Open_bracket);
            expr = parse_expression(parser, areno);
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

            dump_expression(expr->binary_op->lhs, level + 1);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->binary_op->operand));

            dump_expression(expr->binary_op->rhs, level + 1);

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
