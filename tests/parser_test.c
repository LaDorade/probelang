#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

#include "../src/parser.h"
#include "../src/lexer.h"

#define STRING_VIEW_IMPLEMENTATION
#include "../include/string_view.h"
#define ARENO_IMPLEMENTATION
#include "../include/areno.h"

#define BUF_SIZE 1024 * 1024

typedef struct {
    size_t nb_test;
} Test;
Test test = {0};

Lexer  lexer       = {0};
Parser parser      = {0};
Token *tokens      = NULL;
Stmt  *prog        = NULL;
Areno  lex_areno   = {0};

void setup_test(char *buf)
{
    lexer  = (Lexer)  {0};
    parser_free(&parser);
    parser = (Parser) {0};
    tokens = NULL;
    prog   = NULL;

    lexer.sv = (String_View) {
        .items = buf,
        .len   = strlen(buf),
    };
    tokens = lexer_lex(&lexer, &lex_areno);
    parser.tokens = tokens;
    prog = parser_parse(&parser);

    test.nb_test++;
}

void TEST_ASSERT(int expected, const char* msg) {
    if (!expected) {
        fprintf(stderr, "> %s\n", msg);

        if (parser.err.code == Parse_Err_NoError) {
            abort();
        }

        fprintf(stderr, "Error while Testing snippet:\n");
        fprintf(stderr, "%.*s\n", (int)lexer.sv.len, lexer.sv.items);

        size_t start = parser.err.guilty.col - parser.err.guilty.lex_size;
        size_t row = parser.err.guilty.row;
        size_t lex_size = parser.err.guilty.lex_size; 
        printf("\e[1m" "%zu:%zu: " "\033[31m" "error:" "\033[m" " %s" "\e[m",
                row, start,
                parser.err.formatted
                );
        String_View line = sv_get_line(lexer.sv, row);
        printf("%.*s\n", (int)line.len, line.items);

        printf("%*s""\033[32m" "^" "\033[m", (int)start, "");
        for (size_t i = 1; i < lex_size; i++) printf("\033[32m" "~" "\033[m");
        printf("\n");

        fflush(stderr);
        abort();
    }
}

int main()
{
    printf("-- %s\n", __FILE__);
    {   // should support nothing
        setup_test("");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        printf("[TEST] nothing\n");
    }

    {
        setup_test("fun main: () -> void = {yop(\"bijour\", 32) let b =  (1 + -23) * 2}"
                    "fun bijour: () -> void = {top(\"aled  \", 32) let b =  23}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->kind == StmtKind_Block , "Root stmt should be of kind StmtKind_Block");
        TEST_ASSERT(prog->as.block.count == 2 , "Should have 2 statements");
        printf("[TEST] Root stmt ok\n");
    }

    {   // should support 0 statements
        setup_test("fun main: () -> void = {}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].kind == StmtKind_Funcdef ,
                "first statement should be a function");
        // verbeux
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 0 , "should have no statements");
        printf("[TEST] Empty function\n");
    }

    {   // assignement
        setup_test("fun main: () -> void = {let a = 42}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        Stmt *statements  = prog->as.block.items[0].as.funcdef.block->items;
        Stmt  assignement = statements[0];

        TEST_ASSERT(assignement.kind == StmtKind_Assignement , "Statement should be an assignement");
        TEST_ASSERT(assignement.as.assignement.kind == AssignKind_Let , "Assignement kind should be let");
        TEST_ASSERT(sv_eq_string("a", assignement.as.assignement.name.as.ident) , "Assignement ident name should be a");
        Stmt *value = assignement.as.assignement.value_stmt;
        TEST_ASSERT(value->kind == StmtKind_Expression , "Assignement value should be an expression");
        TEST_ASSERT(value->as.expr_stmt->kind == Expr_Number , "Assignement value kind should be a number");
        TEST_ASSERT(value->as.expr_stmt->as.number == 42 , "Assignement value value should be 42");
        printf("[TEST] let assignement\n");
    }

    {   // const assignement
        setup_test("fun main: () -> void = {const a = 42}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt *statements  = prog->as.block.items[0].as.funcdef.block->items;
        Stmt  assignement = statements[0];

        TEST_ASSERT(assignement.kind == StmtKind_Assignement , "Statement should be an assignement");
        TEST_ASSERT(assignement.as.assignement.kind == AssignKind_Const , "Assignement kind should be const");
        TEST_ASSERT(sv_eq_string("a", assignement.as.assignement.name.as.ident) , "Assignement ident name should be a");
        Stmt *value = assignement.as.assignement.value_stmt;
        TEST_ASSERT(value->kind == StmtKind_Expression , "Assignement value should be an expression");
        TEST_ASSERT(value->as.expr_stmt->kind == Expr_Number , "Assignement value kind should be a number");
        TEST_ASSERT(value->as.expr_stmt->as.number == 42 , "Assignement value value should be 42");
        printf("[TEST] const assignement\n");
    }

    {   // reassignement
        setup_test("fun main: () -> void = { a = 42}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt *statements  = prog->as.block.items[0].as.funcdef.block->items;
        Stmt  assignement = statements[0];

        TEST_ASSERT(assignement.kind == StmtKind_Assignement , "Statement should be an assignement");
        TEST_ASSERT(assignement.as.assignement.kind == AssignKind_Reassign , "Assignement kind should be a reassignement");
        TEST_ASSERT(sv_eq_string("a", assignement.as.assignement.name.as.ident) , "Assignement ident name should be a");
        Stmt *value = assignement.as.assignement.value_stmt;
        TEST_ASSERT(value->kind == StmtKind_Expression , "Assignement value should be an expression");
        TEST_ASSERT(value->as.expr_stmt->kind == Expr_Number , "Assignement value kind should be a number");
        TEST_ASSERT(value->as.expr_stmt->as.number == 42 , "Assignement value value should be 42");
        printf("[TEST] reassignement\n");
    }

    {   // bad assignement
        setup_test("fun main: () -> void = {let 32 = 42}");
        TEST_ASSERT(prog == NULL , "Prog should be null");
        TEST_ASSERT(parser.err.code == Parse_Err_UnexpectedToken , "Unenexpected error should be expected");
        TEST_ASSERT(parser.err.formatted != NULL , "Error message should not be null");
        printf("[TEST] Error assignement\n");
    }

    {   // block assignement TODO: Test inside of block
        setup_test("fun main: () -> void = {let a = { 32 }}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt *statements  = prog->as.block.items[0].as.funcdef.block->items;
        Stmt  assignement = statements[0];
        TEST_ASSERT(assignement.kind == StmtKind_Assignement , "Statement should be an assignement");
        TEST_ASSERT(sv_eq_string("a", assignement.as.assignement.name.as.ident) , "Assignement ident name should be a");
        Stmt *value = assignement.as.assignement.value_stmt;
        TEST_ASSERT(value->kind == StmtKind_Block , "Assignement value should be a block");
        printf("[TEST] Block assignement\n");
    }

    {
        setup_test("fun main: () -> void = {1 + 2}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt *statements  = prog->as.block.items[0].as.funcdef.block->items;
        TEST_ASSERT(statements[0].kind == StmtKind_Expression , "Expected an expression");
        Expr  *expression = statements[0].as.expr_stmt;
        TEST_ASSERT(expression->kind == Expr_Binary , "Expected a binary expression");
        TEST_ASSERT(expression->as.binary_op.lhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(expression->as.binary_op.rhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(expression->as.binary_op.lhs->as.number == 1 , "Expected number to be 1");
        TEST_ASSERT(expression->as.binary_op.rhs->as.number == 2 , "Expected number to be 2");
        printf("[TEST] Basic Expression\n");
    }

    {
        setup_test("fun main: () -> void = {\"bijour\"}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt *statements = prog->as.block.items[0].as.funcdef.block->items;
        Stmt  expression = statements[0];
        TEST_ASSERT(expression.kind == StmtKind_Expression , "Expected an expression");
        TEST_ASSERT(expression.as.expr_stmt->kind == Expr_String , "Expected a string expression");
        TEST_ASSERT(sv_eq_string("bijour", expression.as.expr_stmt->as.str) , "Expected string to be \"bijour\"");
        printf("[TEST] String expression (with ;)\n");
    }

    {
        setup_test("fun main: () -> void = {\"bijour\" 1 + 2}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 2 , "should have 2 statement");
        Stmt *statements  = prog->as.block.items[0].as.funcdef.block->items;
        Stmt  string = statements[0];
        TEST_ASSERT(string.kind == StmtKind_Expression , "Expected an expression");
        TEST_ASSERT(string.as.expr_stmt->kind == Expr_String , "Expected a string expression");
        TEST_ASSERT(sv_eq_string("bijour", string.as.expr_stmt->as.str) , "Expected string to be \"bijour\"");
        Stmt  binop = statements[1];
        TEST_ASSERT(binop.kind == StmtKind_Expression , "Expected an expression");
        TEST_ASSERT(binop.as.expr_stmt->kind == Expr_Binary , "Expected a binary expression");
        TEST_ASSERT(binop.as.expr_stmt->as.binary_op.lhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(binop.as.expr_stmt->as.binary_op.rhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(binop.as.expr_stmt->as.binary_op.lhs->as.number == 1 , "Expected number to be 1");
        TEST_ASSERT(binop.as.expr_stmt->as.binary_op.rhs->as.number == 2 , "Expected number to be 2");
        printf("[TEST] Two expression without ';' between\n");
    }

    {
        setup_test("fun main: () -> void = { fn() }");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt *statements = prog->as.block.items[0].as.funcdef.block->items;
        Stmt  expression = statements[0];
        TEST_ASSERT(expression.kind == StmtKind_Expression , "Expected an expression");
        TEST_ASSERT(expression.as.expr_stmt->kind == Expr_Funcall , "Expected a funcall expression");
        TEST_ASSERT(sv_eq_string("fn", expression.as.expr_stmt->as.funcall.callee->as.ident) , "Expected a function to be \"fn\"");
        TEST_ASSERT(expression.as.expr_stmt->as.funcall.args.count == 0 , "Should have no argument passed");
        printf("[TEST] Funcall expression\n");

        setup_test("fun main: () -> void = { fn(x, 32) }");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        statements = prog->as.block.items[0].as.funcdef.block->items;
        expression = statements[0];
        TEST_ASSERT(expression.kind == StmtKind_Expression , "Expected an expression");
        TEST_ASSERT(expression.as.expr_stmt->kind == Expr_Funcall , "Expected a funcall expression");
        TEST_ASSERT(sv_eq_string("fn", expression.as.expr_stmt->as.funcall.callee->as.ident) , "Expected a function to be \"fn\"");
        TEST_ASSERT(expression.as.expr_stmt->as.funcall.args.count == 2 , "Should have 2 arguments passed");
        TEST_ASSERT(expression.as.expr_stmt->as.funcall.args.items[0].kind == Expr_Ident , "Arg 1 should be an indentifier");
        TEST_ASSERT(expression.as.expr_stmt->as.funcall.args.items[1].kind == Expr_Number , "Arg 2 should be a number");
        assert(sv_eq_string("x", expression.as.expr_stmt->as.funcall.args.items[0].as.ident)
                && "Arg 1 ident name should be \"x\"");
        TEST_ASSERT(expression.as.expr_stmt->as.funcall.args.items[1].as.number == 32 , "Arg 2 number should equal 32");
        printf("[TEST] Funcall expression with args\n");
    }

    {
        setup_test("fun main: () -> void = {if 12 == 32 { println(\"bijour!\") }}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt if_statement  = prog->as.block.items[0].as.funcdef.block->items[0];
        
        TEST_ASSERT(if_statement.kind == StmtKind_If , "Expected an if");
        TEST_ASSERT(if_statement.as.if_stmt.condition != NULL , "Expected condition to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ok_stmt != NULL , "Expected block to not be null");

        Expr  *binop = if_statement.as.if_stmt.condition;
        TEST_ASSERT(binop->kind == Expr_Binary , "Expected a binary expression");
        TEST_ASSERT(binop->as.binary_op.lhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(binop->as.binary_op.rhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(binop->as.binary_op.lhs->as.number == 12 , "Expected number to be 12");
        TEST_ASSERT(binop->as.binary_op.rhs->as.number == 32 , "Expected number to be 32");

        Stmt *block = if_statement.as.if_stmt.ok_stmt;
        TEST_ASSERT(block->kind == StmtKind_Block , "Expected a block");
        TEST_ASSERT(block->as.block.count == 1 , "Expected 1 statement in block");
        Stmt stmt = if_statement.as.if_stmt.ok_stmt->as.block.items[0];
        TEST_ASSERT(stmt.kind == StmtKind_Expression , "Expected statement to be an expression");
        TEST_ASSERT(stmt.as.expr_stmt->kind == Expr_Funcall , "Expected expr to be a funcall");
        TEST_ASSERT(sv_eq_string("println", stmt.as.expr_stmt->as.funcall.callee->as.ident) , "Expected funcall name to be 'println'");
        printf("[TEST] Test simple if\n");
    }

    {
        setup_test("fun main: () -> void = {if (41 == 32) { println(\"bijour!\") }}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt if_statement  = prog->as.block.items[0].as.funcdef.block->items[0];
        
        TEST_ASSERT(if_statement.kind == StmtKind_If , "Expected an if");
        TEST_ASSERT(if_statement.as.if_stmt.condition != NULL , "Expected condition to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ok_stmt != NULL , "Expected block to not be null");

        Expr  *binop = if_statement.as.if_stmt.condition;
        TEST_ASSERT(binop->kind == Expr_Binary , "Expected a binary expression");
        TEST_ASSERT(binop->as.binary_op.lhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(binop->as.binary_op.rhs->kind == Expr_Number , "Expected a number");
        TEST_ASSERT(binop->as.binary_op.lhs->as.number == 41 , "Expected number to be 41");
        TEST_ASSERT(binop->as.binary_op.rhs->as.number == 32 , "Expected number to be 32");

        Stmt *block = if_statement.as.if_stmt.ok_stmt;
        TEST_ASSERT(block->kind == StmtKind_Block , "Expected a block");
        TEST_ASSERT(block->as.block.count == 1 , "Expected 1 statement in block");
        Stmt stmt = block->as.block.items[0];
        TEST_ASSERT(stmt.kind == StmtKind_Expression , "Expected statement to be an expression");
        TEST_ASSERT(stmt.as.expr_stmt->kind == Expr_Funcall , "Expected expr to be a funcall");
        TEST_ASSERT(sv_eq_string("println", stmt.as.expr_stmt->as.funcall.callee->as.ident) , "Expected funcall name to be 'println'");
        printf("[TEST] Test simple if with '()'\n");
    }

    { // 
        setup_test("fun main: () -> void = {if (41 == 32 { println(\"bijour!\") }}");
        TEST_ASSERT(prog == NULL , "prog should be null");
        TEST_ASSERT(parser.err.code , "parser should show an error");
        printf("[TEST] Test simple if with missing one '()'\n");
    }

    {
        setup_test("fun main: () -> void = {if 12 == 32 { println(\"yes!\") } else { println(\"no!\") }}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt if_statement  = prog->as.block.items[0].as.funcdef.block->items[0];

        TEST_ASSERT(if_statement.kind == StmtKind_If , "Expected an if");
        TEST_ASSERT(if_statement.as.if_stmt.condition != NULL , "Expected condition to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ok_stmt != NULL , "Expected if-ok stmt to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ko_stmt != NULL , "Expected else stmt to not be null");

        Stmt *else_stmt = if_statement.as.if_stmt.ko_stmt;
        TEST_ASSERT(else_stmt->kind == StmtKind_Block , "Expected a block");
        TEST_ASSERT(else_stmt->as.block.count == 1 , "Expected 1 statement in block");
        Stmt stmt = else_stmt->as.block.items[0];
        TEST_ASSERT(stmt.kind == StmtKind_Expression , "Expected statement to be an expression");
        TEST_ASSERT(stmt.as.expr_stmt->kind == Expr_Funcall , "Expected expr to be a funcall");
        TEST_ASSERT(sv_eq_string("println", stmt.as.expr_stmt->as.funcall.callee->as.ident) , "Expected funcall name to be 'println'");
        // wild test (not checking kind nor if allocated)
        TEST_ASSERT(sv_eq_string("no!", stmt.as.expr_stmt->as.funcall.args.items[0].as.str) , "Expected funcall name to be 'println'");
        printf("[TEST] Test simple if-else\n");
    }

    {
        setup_test("fun main: () -> void = {if 12 == 32 { println(\"yes!\") } else if 32 == 1 { println(\"bij!\") }}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt if_statement  = prog->as.block.items[0].as.funcdef.block->items[0];

        TEST_ASSERT(if_statement.kind == StmtKind_If , "Expected an if");
        TEST_ASSERT(if_statement.as.if_stmt.condition != NULL , "Expected condition to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ok_stmt != NULL , "Expected if-ok stmt to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ko_stmt != NULL , "Expected else stmt to not be null");

        Stmt *else_stmt = if_statement.as.if_stmt.ko_stmt;
        TEST_ASSERT(else_stmt->kind == StmtKind_If , "Expected an if-block");
        TEST_ASSERT(else_stmt->as.if_stmt.ok_stmt->kind == StmtKind_Block , "Expected a block in if");
        Stmt stmt = else_stmt->as.if_stmt.ok_stmt->as.block.items[0];
        TEST_ASSERT(stmt.kind == StmtKind_Expression , "Expected statement to be an expression");
        TEST_ASSERT(stmt.as.expr_stmt->kind == Expr_Funcall , "Expected expr to be a funcall");
        TEST_ASSERT(sv_eq_string("println", stmt.as.expr_stmt->as.funcall.callee->as.ident) , "Expected funcall name to be 'println'");
        // wild test (not checking kind nor if allocated)
        TEST_ASSERT(sv_eq_string("bij!", stmt.as.expr_stmt->as.funcall.args.items[0].as.str) , "Expected funcall name to be 'println'");
        printf("[TEST] Test if-else_if\n");
    }

    {
        setup_test("fun main: () -> void = {if 12 == 32 { println(\"yes!\") } else if 32 == 1 { println(\"bij!\") } else { println(\"aled\") }}");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 1 , "should have 1 statement");
        Stmt if_statement  = prog->as.block.items[0].as.funcdef.block->items[0];

        TEST_ASSERT(if_statement.kind == StmtKind_If , "Expected an if");
        TEST_ASSERT(if_statement.as.if_stmt.condition != NULL , "Expected condition to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ok_stmt != NULL , "Expected if-ok stmt to not be null");
        TEST_ASSERT(if_statement.as.if_stmt.ko_stmt != NULL , "Expected else stmt to not be null");

        Stmt *elseif_stmt = if_statement.as.if_stmt.ko_stmt;
        TEST_ASSERT(elseif_stmt->kind == StmtKind_If , "Expected an if-block");
        TEST_ASSERT(elseif_stmt->as.if_stmt.ok_stmt->kind == StmtKind_Block , "Expected a block in if");
        Stmt stmt = elseif_stmt->as.if_stmt.ok_stmt->as.block.items[0];
        TEST_ASSERT(stmt.kind == StmtKind_Expression , "Expected statement to be an expression");
        TEST_ASSERT(stmt.as.expr_stmt->kind == Expr_Funcall , "Expected expr to be a funcall");
        TEST_ASSERT(sv_eq_string("println", stmt.as.expr_stmt->as.funcall.callee->as.ident) , "Expected funcall name to be 'println'");
        // wild test (not checking kind nor if allocated)
        TEST_ASSERT(sv_eq_string("bij!", stmt.as.expr_stmt->as.funcall.args.items[0].as.str) , "Expected funcall name to be 'println'");

        Stmt *else_stmt = if_statement.as.if_stmt.ko_stmt->as.if_stmt.ko_stmt;
        TEST_ASSERT(else_stmt->kind == StmtKind_Block , "Expected a block");
        TEST_ASSERT(else_stmt->as.block.items[0].kind == StmtKind_Expression , "Expected a expr");
        TEST_ASSERT(else_stmt->as.block.items[0].as.expr_stmt->kind == Expr_Funcall , "Expected a funcall");
        TEST_ASSERT(sv_eq_string("println", else_stmt->as.block.items[0].as.expr_stmt->as.funcall.callee->as.ident) , "Expected fun name to be println");
        printf("[TEST] Test if-else_if-else\n");
    }

    {
        setup_test("fun main: () -> void = { let a = 32 while a >= 0 println(\"bijour!\") }");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 2 , "should have 2 statement");

        Stmt assign = prog->as.block.items[0].as.funcdef.block->items[0];
        TEST_ASSERT(assign.kind == StmtKind_Assignement , "Expected first statement to be an assignement");

        Stmt while_statement = prog->as.block.items[0].as.funcdef.block->items[1];
        TEST_ASSERT(while_statement.kind == StmtKind_While , "Expected a while");
        TEST_ASSERT(while_statement.as.while_stmt.condition != NULL , "Expected condition to not be null");
        TEST_ASSERT(while_statement.as.while_stmt.ok_stmt != NULL , "Expected while-ok stmt to not be null");

        TEST_ASSERT(while_statement.as.while_stmt.ok_stmt->kind == StmtKind_Expression , "Expected while stmt to be a expression");
        TEST_ASSERT(while_statement.as.while_stmt.ok_stmt->as.expr_stmt->kind == Expr_Funcall , "Expected expression to be a funcall");

        printf("[TEST] Test while (no paren, no block)\n");
    }

    {
        setup_test("fun main: () -> void = { let a = 32 while a >= 0 { a = a + 1 println(\"bijour!\") } }");
        TEST_ASSERT(prog != NULL , "prog should not be null");
        TEST_ASSERT(prog->as.block.items[0].as.funcdef.block->count == 2 , "should have 2 statement");

        Stmt assign = prog->as.block.items[0].as.funcdef.block->items[0];
        TEST_ASSERT(assign.kind == StmtKind_Assignement , "Expected first statement to be an assignement");

        Stmt while_statement = prog->as.block.items[0].as.funcdef.block->items[1];
        TEST_ASSERT(while_statement.kind == StmtKind_While , "Expected a while");
        TEST_ASSERT(while_statement.as.while_stmt.condition != NULL , "Expected condition to not be null");
        TEST_ASSERT(while_statement.as.while_stmt.ok_stmt != NULL , "Expected while-ok stmt to not be null");

        TEST_ASSERT(while_statement.as.while_stmt.ok_stmt->kind == StmtKind_Block , "Expected while ok stmt to be a block");
        TEST_ASSERT(while_statement.as.while_stmt.ok_stmt->as.block.count == 2 , "Expected while ok stmt to have 2 statements");

        printf("[TEST] Test while (no paren, block)\n");
    }

    // SMALLER TEST, not unit
    {
        setup_test("fun main: () -> void = { fn()()() }");
        TEST_ASSERT(prog != NULL, "prog should not be null");
        printf("[TEST] Nested funcall (like 'fn()()()')\n");
    }

    areno_free(&lex_areno  );
    parser_free(&parser);

    printf("-- END %s\n", __FILE__);
    return 0;
}

