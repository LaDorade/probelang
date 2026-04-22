#ifndef  PARSER_H
#define  PARSER_H

#include "lexer.h"
#include "areno.h"

typedef struct {
    Token  current;
    Token *tokens;

    size_t cursor;
    size_t count;
} Parser;

Token parser_peek   (Parser *parser);
void  parser_advance(Parser *parser);

typedef struct Unary_Op  Unary_Op;
typedef struct Binary_Op Binary_Op;
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

typedef enum {
    Binary_Plus = Lex_Plus
} Binary_Operator;

typedef enum {
    Unary = Lex_Plus
} Unary_Operator;

struct Expr {
    Expr_Kind kind;
    union {
        int         number;
        String_View str;
        String_View ident;

        Binary_Op  *binary_op;
        Unary_Op   *unary_op;
    };
};

struct Unary_Op {
    Expr           *expr;
    Unary_Operator  operand;
};

struct Binary_Op {
    Expr           *lhs;
    Expr           *rhs;
    Binary_Operator operand;
};

Expr *parse_expression(Parser *parser, Areno *areno);
Expr *parse_addition  (Parser *parser, Areno *areno);
Expr *parse_mul       (Parser *parser, Areno *areno);
Expr *parse_terminal  (Parser *parser, Areno *areno);

#endif //PARSER_H
