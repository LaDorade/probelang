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

typedef struct Expr Expr;
typedef struct Node Node;

typedef struct {
    String_View name;
    String_View type;
} Arg;

typedef struct {
    Arg   *items;
    size_t count;
} Args;

typedef struct {
    Node  *items;
    size_t count;
} Node_Block;

typedef struct {
    String_View name;
    Args        args;
    Node_Block  statements;
} Node_Funcdef;

typedef struct {
    String_View ident;
    Node       *value; // expression or block
                       // sem analysis will check if block ok
} Node_Assignement;

typedef enum {
    NodeKind_Invalid = 0,

    NodeKind_Funcdef,
    NodeKind_Block,

    NodeKind_Expression,
    NodeKind_Assignation,

    NodeKind_LastNode,
} Node_Kind;

struct Node {
    Node_Kind kind;
    union {
        Expr            *expression;
        Node_Block       statements;
        Node_Funcdef     funcdef;
        Node_Assignement assignement;
    };
};


typedef enum {
    Expr_Invalid = 0,

    // Terminal
    Expr_Number,
    Expr_String,
    Expr_Ident,

    // Ope
    Expr_Funcall,
    Expr_Binary,
    Expr_Unary,

    Expr_LastExpr = Expr_Unary,
} Expr_Kind;

typedef struct {
    Node   expr;
    Lexeme operand;
} Unary_Op ;

typedef struct {
    Node   lhs;
    Node   rhs;
    Lexeme operand;
} Binary_Op;

typedef struct {
    String_View name;
    struct {
        Node  *items;
        size_t count;
    } args;
} Funcall;

struct Expr {
    Expr_Kind kind;
    union {
        String_View str;
        String_View ident;
        Binary_Op   binary_op;
        Unary_Op    unary_op;
        Funcall     funcall;
        int         number;
    };
};


Node  parse_funcdef    (Parser *parser, Areno *areno);
Node  parse_block      (Parser *parser, Areno *areno);
Node  parse_assignation(Parser *parser, Areno *areno);
Node  parse_expression (Parser *parser, Areno *areno);
Node  parse_funcall    (Parser *parser, Areno *areno);

Node  parse_addition   (Parser *parser, Areno *areno);
Node  parse_mul        (Parser *parser, Areno *areno);
Node  parse_terminal   (Parser *parser, Areno *areno);

Node  parser_parse  (Parser *parser, Areno *areno);

Token parser_peek   (Parser *parser);
Token parser_prev   (Parser *parser);
void  parser_advance(Parser *parser);
void  parser_expect (Parser *parser, Lexeme lexeme);

// Do not use, use the variadic macro who appends the Lex_Invalid Lexeme
// @returns {Token}, Token of kind Lex_Invalid if not found
Token match(Parser *parser, ...);
#define parser_match(...) match(__VA_ARGS__, Lex_Invalid)

void  dump_expression (Expr *expr, int level);
void  dump_node(Node *node, int level);

#endif //PARSER_H_
