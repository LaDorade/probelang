#ifndef  PARSER_H_
#define  PARSER_H_

#include <stdarg.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "areno.h"
#include "lexer.h"

typedef struct {
    Token *tokens;

    size_t cursor;
} Parser;

Token parser_peek   (Parser *parser);
void  parser_advance(Parser *parser);
int   parser_match  (Parser *parser, Lexeme lexeme);
void  parser_expect (Parser *parser, Lexeme lexeme);

typedef struct Expr      Expr;

typedef enum {
    Expr_Invalid = 0,

    // Terminal
    Expr_Number,
    Expr_String,
    Expr_Ident,

    // Ope
    Expr_Binary,
    Expr_Unary,

    Expr_LastExpr = Expr_Unary,
} Expr_Kind;

typedef struct {
    Expr  *expr;
    Lexeme operand;
} Unary_Op ;

typedef struct {
    Expr  *lhs;
    Expr  *rhs;
    Lexeme operand;
} Binary_Op;

struct Expr {
    Expr_Kind kind;
    union {
        int         number;
        String_View str;
        String_View ident;
        Binary_Op   binary_op;
        Unary_Op    unary_op;
    };
};


Expr *parse_expression(Parser *parser, Areno *areno);
Expr *parse_addition  (Parser *parser, Areno *areno);
Expr *parse_mul       (Parser *parser, Areno *areno);
Expr *parse_terminal  (Parser *parser, Areno *areno);

Token parser_peek(Parser *parser);
void  parser_advance(Parser *parser);
void  parser_expect(Parser *parser, Lexeme lexeme);

// Do not use, use the variadic macro who appends the Lex_Invalid Lexeme
// @returns {Token}, Token of kind Lex_Invalid if not found
Token match(Parser *parser, ...);
#define parser_match(...) match(__VA_ARGS__, Lex_Invalid)

void  dump_expression (Expr *expr, int level);

#endif //PARSER_H_
