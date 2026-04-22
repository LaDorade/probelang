#ifndef  PARSER_H
#define  PARSER_H

#include "lexer.h"

typedef struct {
    Lexeme current;
    Token *tokens;

    size_t cursor;
    size_t count;
} Parser;

typedef struct Unary_Op  Unary_Op;
typedef struct Binary_Op Binary_Op;
typedef struct Expr      Expr;

typedef enum {
    Expr_Invalid = 0,

    // Expr_Terminal
    Expr_Number,
    Expr_String,
    Expr_Ident,

    Expr_Binary,
    Expr_Unary,
} Expr_Kind;

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
    Lexeme operand;
    Expr  *expr;
};

struct Binary_Op {
    Expr  *lhs;
    Lexeme operand;
    Expr  *rhs;
};


#endif //PARSER_H
