#ifndef  PARSER_H_
#define  PARSER_H_

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "areno.h"
#include "lexer.h"

typedef struct Expr Expr;
typedef struct Node Node;

typedef enum {
    Parse_Err_NoError = 0,
    Parse_Err_UnexpectedToken,
    Parse_Err_AllocError
} Parse_Error_Kind;

typedef struct {
    Parse_Error_Kind code;
    size_t row, col;
    char  *foramatted;
} Parse_Error;

typedef struct {
    String_View sv;
    Token *tokens;
    size_t cursor;
    Node  *prog;
    Parse_Error err;
} Parser;

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
    Node       *statements;
} Node_Funcdef;

typedef struct {
    Node *condition; // expression
    Node *block; // 1 statement or block
} Node_If;

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
    Node  *expr;
    Lexeme operand;
} Unary_Op ;

typedef struct {
    Node  *lhs;
    Node  *rhs;
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


Node *parse_funcdef    (Parser *parser, Areno *areno);
Node *parse_block      (Parser *parser, Areno *areno);
Node *parse_funcall    (Parser *parser, Areno *areno);
Node *parse_expression (Parser *parser, Areno *areno);
Node *parse_assignation(Parser *parser, Areno *areno);

Node *parse_comparaison(Parser *parser, Areno *areno);
Node *parse_addition   (Parser *parser, Areno *areno);
Node *parse_mul        (Parser *parser, Areno *areno);
Node *parse_unary      (Parser *parser, Areno *areno);
Node *parse_terminal   (Parser *parser, Areno *areno);

Node *parser_parse  (Parser *parser, Areno *areno);

Token parser_peek   (Parser *parser);
Token parser_prev   (Parser *parser);
void  parser_advance(Parser *parser);
bool  __parser_expect_impl (Parser *parser, Lexeme lexeme);
Token __parser_match_impl(Parser *parser, ...);


void  dump_expression (Expr *expr, int level);
void  dump_node(Node *node, int level);

#endif //PARSER_H_
