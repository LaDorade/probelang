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
    Parse_Err_AllocError,
    Parse_Err_Unreachable,
    Parse_Err_Todo,
} Parse_Error_Kind;

typedef struct {
    Parse_Error_Kind code;
    size_t row, col;
    char  *formatted;
} Parse_Error;

typedef struct {
    Areno  areno;
    String_View sv;
    Token *tokens;
    size_t cursor;
    Node  *prog;
    Parse_Error err;
} Parser;

typedef struct {
    Node  *items;
    size_t count;
} Node_Block;

typedef struct {
    String_View name;
    String_View type;
} Arg;

typedef struct {
    Arg   *items;
    size_t count;
} Args;

typedef struct {
    bool success_set;
    struct {
        bool        nullable;
        String_View ident;
    } success;

    bool error_set;
    struct {
        bool        nullable;
        String_View ident;
    } error;
} Node_Type;

typedef struct {
    String_View name;
    Args        args;
    Node       *return_type_node;
    Node       *block;
} Node_Funcdef;

typedef enum {
    AssignKind_Invalid = 0,

    AssignKind_Let,
    AssignKind_Const,
    AssignKind_Reassign,
} Assign_Kind;

typedef struct {
    Assign_Kind kind;
    String_View ident;
    Node       *type_node;
    Node       *value_node;
} Node_Assignement;

typedef struct {
    Node *condition; // expression
    Node *ok_node; // 1 statement
    Node *ko_node; // 1 statement (NULL if no else)
} Node_If;

typedef struct {
    Node *condition; // expression
    Node *ok_node; // 1 statement
} Node_While;

typedef enum {
    NodeKind_Invalid = 0,

    NodeKind_Funcdef,
    NodeKind_Block,
    NodeKind_Assignement,
    NodeKind_If,
    NodeKind_While,
    NodeKind_Expression,
    NodeKind_Type,

    NodeKind_LastNode,
} Node_Kind;

struct Node {
    Node_Kind kind;
    union {
        Node_Funcdef     funcdef;
        Node_Block       block;
        Node_Assignement assignement;
        Node_If          if_node;
        Node_While       while_node;
        Node_Type        type;
        Expr            *expression;
    } as;
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
    } as;
};

Node *parser_parse (Parser *parser);
void  parser_free  (Parser *parser);

Node *parser_create_nodes(Parser *parser, size_t nb);
Node *parser_create_node (Parser *parser, Node_Kind kind);
Expr *parser_create_expr (Parser *parser, Expr_Kind kind);

// Top level statements
Node *parse_top_statement(Parser *parser);
Node *parse_top_funcdef  (Parser *parser);
Node *parse_top_assign   (Parser *parser); // TODO!

// Block
Node *parse_block        (Parser *parser);
Node *parse_bloc_defer   (Parser *parser); // TODO!

Node *parse_type_expr    (Parser *parser);

// Statement
Node *parse_statement    (Parser *parser);
Node *parse_stmt_assign  (Parser *parser);
Node *parse_stmt_if      (Parser *parser);
Node *parse_stmt_while   (Parser *parser);
Node *parse_stmt_for     (Parser *parser); // TODO!

// Expressions
Node *parse_expression   (Parser *parser);
Node *parse_expr_equal   (Parser *parser);
Node *parse_expr_add     (Parser *parser);
Node *parse_expr_mul     (Parser *parser);
Node *parse_expr_unary   (Parser *parser);
Node *parse_expr_primary (Parser *parser); // TODO!
Node *parse_expr_funcall (Parser *parser);

// Terminal
Node *parse_terminal     (Parser *parser);

Token parser_peek   (Parser *parser);
Token parser_prev   (Parser *parser);
void  parser_advance(Parser *parser);
// parser_expect()
bool  __parser_expect_impl (Parser *parser, Lexeme lexeme);
// parser_match()
Token __parser_match_impl(Parser *parser, ...);

void  parser_prepare_error(Parser *parser, char *msg, Parse_Error_Kind kind);


void  dump_expression (Expr *expr, int level);
void  dump_node(Node *node, int level);

#endif //PARSER_H_
