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
    char  *formatted;
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
    Node       *block;
} Node_Funcdef;

typedef struct {
    Node *condition; // expression
    Node *ok_node; // 1 statement
    Node *ko_node; // 1 statement (NULL if no else)
} Node_If;

typedef struct {
    Node *condition; // expression
    Node *ok_node; // 1 statement
} Node_While;

typedef struct {
    String_View ident;
    Node       *value; // expression or block
                       // sem analysis will check if block ok
} Node_Assignement;

typedef enum {
    NodeKind_Invalid = 0,

    NodeKind_Funcdef,
    NodeKind_Block,
    NodeKind_Assignement,
    NodeKind_If,
    NodeKind_While,
    NodeKind_Expression,

    NodeKind_LastNode,
} Node_Kind;

struct Node {
    Node_Kind kind;
    union {
        Node_Funcdef     funcdef;
        Node_Block       statements;
        Node_Assignement assignement;
        Node_If          if_node;
        Node_While       while_node;
        Expr            *expression;
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

Node *parser_parse  (Parser *parser, Areno *areno);

// Top level statements
Node *parse_top_statement(Parser *parser, Areno *areno);
Node *parse_top_funcdef  (Parser *parser, Areno *areno);
Node *parse_top_assign   (Parser *parser, Areno *areno); // TODO!

// Block
Node *parse_block        (Parser *parser, Areno *areno);
Node *parse_bloc_defer   (Parser *parser, Areno *areno); // TODO!

// Statement
Node *parse_statement    (Parser *parser, Areno *areno);
Node *parse_stmt_assign  (Parser *parser, Areno *areno);
Node *parse_stmt_if      (Parser *parser, Areno *areno);
Node *parse_stmt_while   (Parser *parser, Areno *areno);
Node *parse_stmt_for     (Parser *parser, Areno *areno); // TODO!

// Expressions
Node *parse_expression   (Parser *parser, Areno *areno);
Node *parse_expr_equal   (Parser *parser, Areno *areno);
Node *parse_expr_add     (Parser *parser, Areno *areno);
Node *parse_expr_mul     (Parser *parser, Areno *areno);
Node *parse_expr_unary   (Parser *parser, Areno *areno);
Node *parse_expr_primary (Parser *parser, Areno *areno); // TODO!
Node *parse_expr_funcall (Parser *parser, Areno *areno);

// Terminal
Node *parse_terminal     (Parser *parser, Areno *areno);

Token parser_peek   (Parser *parser);
Token parser_prev   (Parser *parser);
void  parser_advance(Parser *parser);
// parser_expect()
bool  __parser_expect_impl (Parser *parser, Lexeme lexeme);
// parser_match()
Token __parser_match_impl(Parser *parser, ...);


void  dump_expression (Expr *expr, int level);
void  dump_node(Node *node, int level);

#endif //PARSER_H_
