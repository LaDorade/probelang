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
typedef struct Stmt Stmt;
typedef struct Stmt_Type Stmt_Type;

typedef enum {
    Parse_Err_NoError = 0,

    Parse_Err_UnexpectedToken,
    Parse_Err_AllocError,
    Parse_Err_Unreachable,
    Parse_Err_Todo,
} Parse_Error_Kind;

typedef struct {
    Token  guilty;
    size_t row, col;
    char  *formatted;
    Parse_Error_Kind code;
} Parse_Error;

typedef struct {
    Parse_Error err;
    Areno      *areno;
    size_t      cursor;
    Token      *tokens;
    Stmt       *prog;
} Parser;

typedef struct {
    Stmt  *items;
    size_t count;
} Stmt_Block;

typedef struct {
    Token      name;
    Stmt_Type *type;
} Arg;

typedef struct {
    Arg   *items;
    size_t count;
} Args;

struct Stmt_Type {
    struct {
        Token name_tok;
        bool  nullable;
    } success;

    struct {
        Token name_tok;
        bool  nullable;
    } error;

    bool success_set;
    bool error_set;
};

typedef struct {
    Token name_tok; // can be of kind Tok_Invalid (anonymous function)
    Args  args;
    Stmt_Type  *return_type_stmt;
    Stmt_Block *block;
} Stmt_Funcdef;

typedef enum {
    AssignKind_Invalid = 0,

    AssignKind_Let,
    AssignKind_Const,
    AssignKind_Reassign,
} Assign_Kind;

typedef struct {
    Token      name_tok;
    Stmt      *value_stmt; // expr or block
    Stmt_Type *type_stmt;
    Assign_Kind kind;
} Stmt_Assignement;

typedef struct {
    Expr *condition; // expression
    Stmt *ok_stmt; // 1 statement
    Stmt *ko_stmt; // 1 statement (NULL if no else)
} Stmt_If;

typedef struct {
    Expr *condition; // expression
    Stmt *ok_stmt; // 1 statement
} Stmt_While;

typedef enum {
    StmtKind_Invalid = 0,

    StmtKind_Funcdef,
    StmtKind_Block,
    StmtKind_Assignement,
    StmtKind_If,
    StmtKind_While,
    StmtKind_Expression,
    StmtKind_Type,

    StmtKind_LastStmt,
} Stmt_Kind;

struct Stmt {
    union {
        Stmt_Funcdef     funcdef;
        Stmt_Block       block;
        Stmt_Assignement assignement;
        Stmt_If          if_stmt;
        Stmt_While       while_stmt;
        Stmt_Type        type;
        Expr            *expr_stmt;
    } as;
    Stmt_Kind kind;
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
    Expr  *expr;
    Lexeme operand;
} Unary_Op ;

typedef struct {
    Expr  *lhs;
    Expr  *rhs;
    Lexeme operand;
} Binary_Op;

typedef struct {
    Expr *callee;
    struct {
        Expr  *items;
        size_t count;
    } args;
} Funcall;

struct Expr {
    union {
        Binary_Op binary_op;
        Unary_Op  unary_op;
        Funcall   funcall;
        Token     str_tok;
        Token     ident_tok;
        Token     number_tok;
    } as;
    Expr_Kind kind;
};

/////////////////////// PARSING ////////////////////////////////

Stmt *parser_parse (Parser *parser);

// Statement
Stmt *parse_statement  (Parser *parser);
Stmt *parse_stmt_assign(Parser *parser);
Stmt *parse_stmt_if    (Parser *parser);
Stmt *parse_stmt_while (Parser *parser);
Stmt *parse_stmt_for   (Parser *parser); // TODO!

// Function
Stmt *parse_func_def(Parser *parser);
Stmt *parse_func_arg(Parser *parser);

// Block
Stmt_Block *parse_block  (Parser *parser);

Stmt_Type  *parse_type_expr    (Parser *parser);

// Expressions
Expr *parse_expression   (Parser *parser);
Expr *parse_expr_equal   (Parser *parser);
Expr *parse_expr_add     (Parser *parser);
Expr *parse_expr_mul     (Parser *parser);
Expr *parse_expr_unary   (Parser *parser);
Expr *parse_expr_primary (Parser *parser);
Expr *parse_expr_funcall (Parser *parser);

// Terminal
Expr *parse_terminal     (Parser *parser);

/////////////////////// UTILS ////////////////////////////////

static inline Stmt *parser_create_stmts(Parser *parser, size_t nb);
static inline Stmt *parser_create_stmt (Parser *parser, Stmt_Kind kind);
static inline Expr *parser_create_exprs(Parser *parser, size_t nb);
static inline Expr *parser_create_expr (Parser *parser, Expr_Kind kind);

static inline Token parser_peek   (Parser *parser);
static inline Token parser_prev   (Parser *parser);
static inline void  parser_advance(Parser *parser);
static inline Token parser_lookahead(Parser *parser, size_t nb);

static inline bool  __parser_expect_impl (Parser *parser, Lexeme lexeme);
#define parser_expect(parser, lexeme) do {if (!__parser_expect_impl(parser, lexeme)) return NULL; } while (0);

static inline Token __parser_match_impl(Parser *parser, ...);
#define parser_match(...) __parser_match_impl(__VA_ARGS__, Lex_Invalid)


static inline void  parser_prepare_error(Parser *parser, char *msg, Parse_Error_Kind kind);

/////////////////////// DUMPING ////////////////////////////////

void dump_stmt_block(Stmt_Block *block, int level);
void dump_stmt_type(Stmt_Type *type, int level);
void dump_expression (Expr *expr, int level);
void dump_stmt(Stmt *stmt, int level);

#endif //PARSER_H_
