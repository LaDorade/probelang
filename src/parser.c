#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

#define MAX_ARGS 10
#define BUF_SIZE 1024

String_View sv_copy(Areno *areno, String_View *src)
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
Token parser_prev(Parser *parser)
{
    if (parser->cursor <= 0) {
        return (Token) {
            .kind = Lex_Invalid,
        };
    }
    return parser->tokens[parser->cursor - 1];
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

#define parser_match(...) __parser_match_impl(__VA_ARGS__, Lex_Invalid)
Token __parser_match_impl(Parser *parser, ...)
{
    va_list args;
    va_start(args, parser);
    Lexeme arg;
    while ((arg = va_arg(args, Lexeme)) != Lex_Invalid) {
        Token current = parser->tokens[parser->cursor];
        if (parser_peek(parser).kind == arg) {
            parser_advance(parser);
            return current;
        }
    }
    return (Token) {
        .kind = Lex_Invalid 
    };
}

#define parser_expect(parser, lexeme) do {if (!__parser_expect_impl(parser, lexeme)) return NULL; } while (0);
bool __parser_expect_impl(Parser *parser, Lexeme lexeme)
{
    if (parser_match(parser, lexeme).kind == Lex_Invalid) {
        Token current          = parser_peek(parser);
        Token prev             = parser_prev(parser);
        const char* lexeme_str = lex_print(lexeme);
        const char* prev_str   = token_print(&prev, &parser->areno);
        const char *curr_str   = token_print(&current, &parser->areno);

        char *err_msg = areno_printf(&parser->areno,
                "Expected '%s' after '%s', got '%s'\n",
                lexeme_str,
                prev_str,
                curr_str);

        parser_prepare_error(parser, err_msg, Parse_Err_UnexpectedToken);
        return false;
    }
    return true;
}

// Take current token, and wrap error
void parser_prepare_error(Parser *parser, char *msg, Parse_Error_Kind kind)
{
    Token current = parser_peek(parser);
    parser->err = (Parse_Error) {
        .guilty    = current,
        .code      = msg == NULL ? Parse_Err_AllocError : kind,
        .formatted = msg == NULL ? "Error alloc memory" : msg
    };
}

void parser_free(Parser *parser)
{
    areno_free(&parser->areno);
}

Stmt *parser_create_stmts(Parser *parser, size_t nb)
{
    Stmt *stmts = (Stmt*) areno_alloc(&parser->areno, sizeof(Stmt) * nb);
    memset(stmts, 0, sizeof(Stmt) * nb);
    return stmts;
}
Stmt *parser_create_stmt(Parser *parser, Stmt_Kind kind)
{
    Stmt *stmt = (Stmt*) parser_create_stmts(parser, 1);
    stmt->kind = kind;
    return stmt;
}
Expr *parser_create_exprs(Parser *parser, size_t nb)
{
    Expr *exprs = (Expr*) areno_alloc(&parser->areno, sizeof(Expr) * nb);
    memset(exprs, 0, sizeof(Stmt) * nb);
    return exprs;
}
Expr *parser_create_expr(Parser *parser, Expr_Kind kind)
{
    Expr *expr = (Expr*) areno_alloc(&parser->areno, sizeof(Expr));
    memset(expr, 0, sizeof(Expr));
    expr->kind = kind;
    return expr;
}

/////////////////////// PARSING ////////////////////////////////

Stmt *parser_parse(Parser *parser)
{
    Stmt *program = parser_create_stmt(parser, StmtKind_Block);
    program->as.block = (Stmt_Block) {
        .count = 0,
        .items = parser_create_stmts(parser, BUF_SIZE),
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_EOF) {
        Stmt *func = parse_statement(parser);
        if (func == NULL) return NULL;
        program->as.block.items[program->as.block.count++] = *func;
    }
    return program;
}

Stmt *parse_statement(Parser *parser)
{
    Token current = parser_peek(parser);
    if (current.kind == Lex_Open_brace) { // { ... } -- block
        Stmt_Block *block = parse_block(parser); 
        if (block == NULL) return NULL;
        Stmt *stmt = parser_create_stmt(parser, StmtKind_Block);
        stmt->as.block = *block;
        // TODO: make fields in stmt, pointers
        return stmt;

    } else if (current.kind == Lex_Ident
            && parser_lookahead(parser, 1).kind == Lex_Equal
        ) {// x = ...; -- reassign
        Stmt *stmt = parse_stmt_assign(parser);
        if (stmt == NULL) return NULL;
        return stmt;

    } else if (current.kind == Lex_fun) {
        // fun xx: (...) -> ... = { ... }
        Stmt *stmt = parse_func_def(parser);
        if (stmt == NULL) return NULL;
        return stmt;

    } else if (current.kind == Lex_let || current.kind == Lex_const) { // const/let x = ...;
        Stmt *stmt = parse_stmt_assign(parser);
        if (stmt == NULL) return NULL;
        return stmt;

    } else if (current.kind == Lex_if) { // if ...
        Stmt *stmt = parse_stmt_if(parser);
        if (stmt == NULL) return NULL;
        return stmt;

    } else if (current.kind == Lex_while) { // while ...
        Stmt *stmt = parse_stmt_while(parser);
        if (stmt == NULL) return NULL;
        return stmt;

    }

    Expr *expr = parse_expression(parser);
    if (expr == NULL) return NULL;
    Stmt *expr_stmt = parser_create_stmt(parser, StmtKind_Expression);
    expr_stmt->as.expr_stmt = expr;
    return expr_stmt;
}

// func-decl = 'fun' ident ':' '(' ident ':' type { ',' ident ':' type } ')' '->' type '=' '{' { statements } '}';
Stmt *parse_func_def(Parser *parser)
{
    parser_expect(parser, Lex_fun);
    Token funcname = parser_peek(parser);
    parser_expect(parser, Lex_Ident);
    parser_expect(parser, Lex_Colon);

    // args
    parser_expect(parser, Lex_Open_bracket);

    Args args = {0};
    args.items = areno_alloc(&parser->areno, sizeof(Arg) * MAX_ARGS);
    while (!parser_match(parser, Lex_Close_bracket).kind) {
        Token arg_name = parser_peek(parser);
        parser_expect(parser, Lex_Ident);

        parser_expect(parser, Lex_Colon);
        Stmt_Type *type = parse_type_expr(parser);
        if (type == NULL) return NULL;

        args.items[args.count++] = (Arg) {
            .name = arg_name,
            .type = type,
        };

        // this allow trailing coma
        if (!parser_match(parser, Lex_Comma).kind) {
            parser_expect(parser, Lex_Close_bracket);
            break;
        }
    }

    parser_expect(parser, Lex_Arrow_Right);
    Stmt_Type *type = parse_type_expr(parser);
    if (type == NULL) return NULL;

    parser_expect(parser, Lex_Equal);
    
    // expect a block
    Stmt_Block *body = parse_block(parser);
    if (body == NULL) return NULL;
    
    Stmt *stmt = parser_create_stmt(parser, StmtKind_Funcdef);
    stmt->as.funcdef = (Stmt_Funcdef) {
        .name  = funcname,
        .args  = args,
        .block = body,
        .return_type_stmt = type,
    };
    return stmt;
}

Stmt_Block *parse_block(Parser *parser)
{
    parser_expect(parser, Lex_Open_brace);

    Stmt_Block *block = areno_calloc(&parser->areno, sizeof(Stmt_Block));
    *block = (Stmt_Block) {
        .count = 0,
        .items = parser_create_stmts(parser, BUF_SIZE)
    };

    Token current;
    while ((current = parser_peek(parser)).kind != Lex_Close_brace) {
        if (current.kind == Lex_Close_brace) {
            break;
        } else {
            Stmt *stmt = parse_statement(parser);
            if (stmt == NULL) return NULL;
            block->items[block->count++] = *stmt;
        }
    }

    parser_expect(parser, Lex_Close_brace);

    return block;
}

Stmt *parse_stmt_if(Parser *parser)
{
    Stmt *stmt = parser_create_stmt(parser, StmtKind_If);

    /* Parse if exmpression */
    parser_expect(parser, Lex_if);
    Expr *expr = parse_expression(parser);
    if (expr == NULL) return NULL;
    stmt->as.if_stmt.condition = expr;

    /* Parse if nodé */
    Stmt *if_stmt = parse_statement(parser);
    if (if_stmt == NULL) return NULL;
    stmt->as.if_stmt.ok_stmt = if_stmt;

    /* Parse else nodé */
    if (parser_match(parser, Lex_else).kind != Lex_Invalid) {
        Stmt *else_block = parse_statement(parser);
        if (else_block == NULL) return NULL;
        stmt->as.if_stmt.ko_stmt = else_block;
    }

    return stmt;
}

Stmt *parse_stmt_while(Parser *parser)
{
    Stmt *stmt = parser_create_stmt(parser, StmtKind_While);

    /* Parse while exmpression */
    parser_expect(parser, Lex_while);
    Expr *expr = parse_expression(parser);
    if (expr == NULL) return NULL;
    stmt->as.while_stmt.condition = expr;

    /* Parse while nodé */
    Stmt *while_stmt = parse_statement(parser);
    if (while_stmt == NULL) return NULL;
    stmt->as.while_stmt.ok_stmt = while_stmt;

    return stmt;
}

// type-expr     ::= term-optional [ '!' type-optional ] ;
// type-optional ::= [ '?' ] term-ident ;
Stmt_Type *parse_type_expr(Parser *parser)
{
    Stmt_Type *type = areno_calloc(&parser->areno, sizeof(Stmt_Type));

    Token current;
    bool nullable = parser_match(parser, Lex_Question).kind != Lex_Invalid;
    if ((current = parser_match(parser, Lex_Ident)).kind != Lex_Invalid) {
        type->success_set      = true;
        type->success.name     = current;
        type->success.nullable = nullable;
    } else if (nullable) {
        parser_expect(parser, Lex_Ident); // crash if nullable but no ident
    }

    if (parser_match(parser, Lex_Bang).kind != Lex_Invalid) {
        bool nullable = parser_match(parser, Lex_Question).kind != Lex_Invalid;
        if ((current = parser_match(parser, Lex_Ident)).kind != Lex_Invalid) {
            type->error_set      = true;
            type->error.name    = current;
            type->error.nullable = nullable;
        } else if (nullable) {
            parser_expect(parser, Lex_Ident);
        }
    }

    if (!type->success_set && !type->error_set) {
        current = parser_peek(parser);
        char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected type-expression, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
        return NULL;
    }

    return type;
}

// stmt-assign   ::= [ 'let' | 'const' ] term-ident [ ':' type-expr ] '=' expression | block ;
Stmt *parse_stmt_assign(Parser *parser)
{
    Token current = parser_match(parser, Lex_let, Lex_const, Lex_Ident);
    if (current.kind == Lex_Invalid) {
        current = parser_peek(parser);
        char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected assignation, found: '%s'\n",
                current.row,
                current.col,
                lex_print(current.kind));
        parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
        return NULL;
    }

    Assign_Kind assign_kind = AssignKind_Invalid;
    if (current.kind == Lex_let) {
        assign_kind = AssignKind_Let;
    } else if (current.kind == Lex_const) {
        assign_kind = AssignKind_Const;
    } else if (current.kind == Lex_Ident) {
        assign_kind = AssignKind_Reassign;
    } else {
        printf("[UNREACHABLE] stmt-assign");
        assert(0 && "[UNREACHABLE] stmt-assign");
    }

    Token ident = current;
    if (current.kind != Lex_Ident) {
        ident = parser_peek(parser);
        parser_expect(parser, Lex_Ident);
    }
    Stmt_Type *type = NULL;
    if (parser_match(parser, Lex_Colon).kind != Lex_Invalid) {
        type = parse_type_expr(parser);
        if (type == NULL) return NULL;
    }

    parser_expect(parser, Lex_Equal);

    Stmt *value = NULL;
    // parse as block
    if (parser_peek(parser).kind == Lex_Open_brace) {
        Stmt_Block *block = parse_block(parser);
        if (block == NULL) return NULL;
        Stmt *stmt = parser_create_stmt(parser, StmtKind_Block);
        stmt->as.block = *block;
        value = stmt;
    } else { // parse as expression
        Expr *expr = parse_expression(parser);
        if (expr == NULL) return NULL;
        Stmt *expr_stmt = parser_create_stmt(parser, StmtKind_Expression);
        expr_stmt->as.expr_stmt = expr;
        value = expr_stmt;
    }

    Stmt *stmt = parser_create_stmt(parser, StmtKind_Assignement);
    stmt->as.assignement = (Stmt_Assignement) {
        .name       = ident,
        .kind       = assign_kind,
        .value_stmt = value,
        .type_stmt  = type
    };

    return stmt;
}

Expr *parse_expression(Parser *parser)
{
    return parse_expr_equal(parser);
}

// x == y >= z
Expr *parse_expr_equal(Parser *parser)
{
    Expr *lhs = parse_expr_add(parser);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser,
            Lex_Equal_Equal, Lex_Not_Equal,
            Lex_Greater_Equal, Lex_Lower_Equal)
        ).kind != Lex_Invalid)
    {
        Expr *rhs = parse_expr_add(parser);
        if (rhs == NULL) return NULL;

        Expr *expr = parser_create_expr(parser, Expr_Binary);
        expr->as.binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };

        lhs = expr;
    }
    return lhs;
}

// x + y - 23
Expr *parse_expr_add(Parser *parser)
{
    Expr *lhs = parse_expr_mul(parser);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser, Lex_Plus, Lex_Minus)).kind != Lex_Invalid) {
        Expr *rhs = parse_expr_mul(parser);
        if (rhs == NULL) return NULL;

        Expr *expr = parser_create_expr(parser, Expr_Binary);
        expr->as.binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };

        lhs = expr;
    }
    return lhs;
}

// expr-mul     ::= expr-unary { ('*' | '/' | '%') expr-unary }
Expr *parse_expr_mul(Parser *parser)
{
    Expr *lhs = parse_expr_unary(parser);
    if (lhs == NULL) return NULL;

    Token current;
    while ((current = parser_match(parser, Lex_Mul, Lex_Divide, Lex_Modulo)).kind != Lex_Invalid) {
        Expr *rhs = parse_expr_unary(parser);
        if (rhs == NULL) return NULL;

        Expr *expr = parser_create_expr(parser, Expr_Binary);
        expr->as.binary_op = (Binary_Op) {
            .lhs     = lhs,
            .rhs     = rhs,
            .operand = current.kind,
        };

        lhs = expr;
    }
    return lhs;
}

//expr-unary   ::= { '!' | '-' } expr-unary
// | expr-funcall ;
Expr *parse_expr_unary(Parser *parser)
{
    Token current = parser_match(parser, Lex_Minus, Lex_Bang);
    if (current.kind != Lex_Invalid) {
        Expr *expr = parser_create_expr(parser, Expr_Unary);
        expr->as.unary_op = (Unary_Op) {
            .operand = current.kind,
            .expr = parse_expr_unary(parser),
        };

        return expr;
    } else {
        Expr *expr = parse_expr_funcall(parser);
        if (expr == NULL) return NULL;
        return expr;
    }
}

Expr *parse_expr_funcall(Parser *parser)
{
    Expr *expr = parse_expr_primary(parser);
    if (expr == NULL) return NULL;

    while (parser_match(parser, Lex_Open_bracket).kind != Lex_Invalid) {
        Expr *funcall = parser_create_expr(parser, Expr_Funcall);
        funcall->as.funcall = (Funcall) {
            .callee = expr,
            .args = {
                .count = 0,
                .items = parser_create_exprs(parser, MAX_ARGS),
            }
        };

        // args
        while (parser_match(parser, Lex_Close_bracket).kind == Lex_Invalid) {
            Expr *arg = parse_expression(parser);
            if (arg == NULL) return NULL;
            funcall->as.funcall.args.items[funcall->as.funcall.args.count++] = *arg;

            // this allow trailing coma
            if (!parser_match(parser, Lex_Comma).kind) {
                parser_expect(parser, Lex_Close_bracket);
                break;
            }
        }
        // store to handle muti call "fn()()()"
        expr = funcall;
    }

    return expr;
}

// expr-primary ::= '(' expression ')' | terminal ;
Expr *parse_expr_primary(Parser *parser)
{
    Expr *expr = NULL;
    Token current = parser_peek(parser);
    if (current.kind == Lex_Open_bracket) {
        parser_expect(parser, Lex_Open_bracket);
        expr = parse_expression(parser);
        if (expr == NULL) return NULL;
        parser_expect(parser, Lex_Close_bracket);
    } else {
        expr = parse_terminal(parser);
        if (expr == NULL) return NULL;
    }
    if (expr == NULL) return NULL;
    return expr;
}

// terminal ::= term-ident | term-string | term-number
Expr *parse_terminal(Parser *parser)
{
    Token current = parser_peek(parser);
    if (current.kind == Lex_Ident) { // x
        parser_expect(parser, Lex_Ident);
        Expr *expr = parser_create_expr(parser, Expr_Ident);
        expr->as.ident = sv_copy(&parser->areno, &current.as.ident);
        return expr;

    } else if (current.kind == Lex_String) { // "snoup"
        parser_expect(parser, Lex_String);
        // "snoup"
        Expr *expr = parser_create_expr(parser, Expr_String);
        expr->as.str = sv_copy(&parser->areno, &current.as.string);
        return expr;

    } else if (current.kind == Lex_Number) { // 23
        parser_expect(parser, Lex_Number);
        // 23
        Expr *expr = parser_create_expr(parser, Expr_Number);
        expr->as.number = current.as.number;
        return expr;

    }
    char *err = areno_printf(&parser->areno, "ERROR at %zu:%zu: Expected expression, found: '%s'\n",
            current.row,
            current.col,
            lex_print(current.kind));
    parser_prepare_error(parser, err, Parse_Err_UnexpectedToken);
    return NULL;
}

/////////////////////// DUMPING ////////////////////////////////

void dump_stmt_block(Stmt_Block *block, int level)
{
    for (int i = 0; i < level; i++) printf(" ");
    printf("Block:\n");
    if (block->count <= 0) {
        for (int i = 0; i < level + 2; i++) printf(" ");
        printf("(no statements)\n");
    } else {
        for (size_t i = 0; i < block->count; i++) {
            dump_stmt(&block->items[i], level + 1);
        }
    }
}

void dump_stmt_type(Stmt_Type *type, int level)
{
    for (int i = 0; i < level; i++) printf(" ");
    printf("Type: ");

    if (type->success_set) {
        if (type->success.nullable) {
            printf("?");
        }
        printf("%.*s",
                (int) type->success.name.as.ident.len, // TODO: fix unsafe
                type->success.name.as.ident.items);
    }
    if (type->error_set) {
        printf("!");
        if (type->error.nullable) {
            printf("?");
        }
        printf("%.*s",
                (int) type->error.name.as.ident.len, // TODO: fix unsafe
                type->error.name.as.ident.items);
    }
    printf("\n");
}

void dump_stmt(Stmt *stmt, int level)
{
    switch (stmt->kind) {
        case StmtKind_Invalid:
        case StmtKind_LastStmt:
            printf("[ERROR] Printing invalid stmt\n");
            abort();

        case StmtKind_Type: {
            dump_stmt_type(&stmt->as.type, level);
        } break;

        case StmtKind_Block: {
            dump_stmt_block(&stmt->as.block, level);
        } break;

        case StmtKind_Expression: {
            dump_expression(stmt->as.expr_stmt, level);
        } break;

        case StmtKind_Funcdef: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Function Def:\n");
            // name
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %.*s\n",
                    (int)stmt->as.funcdef.name.as.ident.len, // TODO: fix unsafe
                    stmt->as.funcdef.name.as.ident.items);
            // args
            if (stmt->as.funcdef.args.count <= 0) {
                for (int i = 0; i < level + 2; i++) printf(" ");
                printf("(no arguments)\n");
            } else {
                for (size_t i = 0; i < stmt->as.funcdef.args.count; i++) {
                    Arg arg = stmt->as.funcdef.args.items[i];
                    for (int i = 0; i < level + 2; i++) printf(" ");
                    printf("Args %d\n", (int)i + 1);
                    for (int i = 0; i < level + 3; i++) printf(" ");
                    printf("Name: %.*s\n",
                            (int)arg.name.as.ident.len,
                            arg.name.as.ident.items); // TODO: fix unsafe
                    dump_stmt_type(arg.type, level + 3);
                }
            }
            // type
            dump_stmt_type(stmt->as.funcdef.return_type_stmt, level + 1);
            // body
            dump_stmt_block(stmt->as.funcdef.block, level + 1);
        } break;

        case StmtKind_Assignement: {
            const char *kind = stmt->as.assignement.kind == AssignKind_Let
                ? "let"
                : stmt->as.assignement.kind == AssignKind_Const
                ? "const"
                : "reassign";
            for (int i = 0; i < level; i++) printf(" ");
            printf("Assignation (%s):\n", kind);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Name: %.*s\n",
                    (int)stmt->as.assignement.name.as.ident.len,// TODO: fix unsafe
                    stmt->as.assignement.name.as.ident.items);

            if (stmt->as.assignement.type_stmt != NULL
                    && (stmt->as.assignement.type_stmt->success_set
                        || stmt->as.assignement.type_stmt->error_set)
            ) {
                dump_stmt_type(stmt->as.assignement.type_stmt, level + 1);
            }

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Value:\n");
            dump_stmt(stmt->as.assignement.value_stmt, level + 2);
        } break;

        case StmtKind_If: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("If:\n");

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Cond:\n");
            dump_expression(stmt->as.if_stmt.condition, level + 2);

            dump_stmt(stmt->as.if_stmt.ok_stmt, level + 1);
            if (stmt->as.if_stmt.ko_stmt != NULL) {
                for (int i = 0; i < level + 1; i++) printf(" ");
                printf("Else\n");
                dump_stmt(stmt->as.if_stmt.ko_stmt, level + 2);
            }
        } break;

        case StmtKind_While: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("While:\n");
            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Cond:\n");
            dump_expression(stmt->as.if_stmt.condition, level + 2);
            dump_stmt(stmt->as.if_stmt.ok_stmt, level + 1);
        } break;

    }
}

void dump_expression (Expr *expr, int level)
{
    switch (expr->kind) {
        case Expr_Funcall: {
            for (int i = 0; i < level; i++) printf(" ");
            printf("Funcall:\n");

            dump_expression(expr->as.funcall.callee, level + 1);

            if (expr->as.funcall.args.count == 0) {
                for (int i = 0; i < level + 1; i++) printf(" ");
                printf("(no arguments)\n");
            } else {
                for (size_t i = 0; i < expr->as.funcall.args.count; i++) {
                    for (int i = 0; i < level + 1; i++) printf(" ");
                    printf("Arg %zu\n", i + 1);
                    dump_expression(&expr->as.funcall.args.items[i], level + 2);
                }
            }
        } break;
        case Expr_Number:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Number: %f\n", expr->as.number);
            break;
        case Expr_Binary:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Binary:\n");

            dump_expression(expr->as.binary_op.lhs, level + 1);

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->as.binary_op.operand));

            dump_expression(expr->as.binary_op.rhs, level + 1);

            break;
        case Expr_Ident:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Ident: %.*s\n", (int) expr->as.ident.len, expr->as.ident.items);
            break;
        case Expr_String:
            for (int i = 0; i < level; i++) printf(" ");
            printf("String: %.*s\n", (int) expr->as.str.len, expr->as.str.items);
            break;
        case Expr_Unary:
            for (int i = 0; i < level; i++) printf(" ");
            printf("Unary:\n");

            for (int i = 0; i < level + 1; i++) printf(" ");
            printf("Operator: %s\n", lex_print((Lexeme) expr->as.unary_op.operand));

            dump_expression(expr->as.unary_op.expr, level + 1);

            break;
        case Expr_Invalid:
            assert(0 && "[INVALID] dump invalid expression");
            break;
    }
}
