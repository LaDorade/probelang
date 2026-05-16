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
Node  *prog        = NULL;
Areno  lex_areno   = {0};
Areno  parse_areno = {0};

void setup_test(char *buf)
{
    lexer  = (Lexer)  {0};
    parser = (Parser) {0};
    tokens = NULL;
    prog   = NULL;

    lexer.sv = (String_View) {
        .items = buf,
        .len   = strlen(buf),
    };
    tokens = lexer_lex(&lexer, &lex_areno);
    parser.sv     = lexer.sv;
    parser.tokens = tokens;
    prog = parser_parse(&parser, &parse_areno);

    test.nb_test++;
}

int main()
{
    printf("-- %s\n", __FILE_NAME__);
    {   // should support nothing
        setup_test("");
        assert(prog != NULL && "prog should not be null");
        printf("[TEST] nothing\n");
    }

    {
        setup_test("main    :: () {yop(\"bijour\", 32); let b =  (1 + -23) * 2;}"
                    "bijour :: () {top(\"aled  \", 32); let b =  23;}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->kind == NodeKind_Block && "Root node should be of kind NodeKind_Block");
        assert(prog->statements.count == 2 && "Should have 2 statements");
        printf("[TEST] Root node ok\n");
    }

    {   // should support 0 statements
        setup_test("main :: () {}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].kind == NodeKind_Funcdef &&
                "first statement should be a function");
        // verbeux
        assert(prog->statements.items[0].funcdef.block->kind == NodeKind_Block && "should have a block");
        assert(prog->statements.items[0].funcdef.block->statements.count == 0 && "should have no statements");
        printf("[TEST] Empty function\n");
    }

    {   // assignement
        setup_test("main :: () {let a = 42;}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements  = prog->statements.items[0].funcdef.block->statements.items;
        Node  assignement = statements[0];

        assert(assignement.kind == NodeKind_Assignement && "Statement should be an assignement");
        assert(sv_eq_string("a", assignement.assignement.ident) && "Assignement ident name should be a");
        Node *value = assignement.assignement.value;
        assert(value->kind == NodeKind_Expression && "Assignement value should be an expression");
        assert(value->expression->kind == Expr_Number && "Assignement value kind should be a number");
        assert(value->expression->number == 42 && "Assignement value value should be 42");
        printf("[TEST] Standard assignement\n");
    }

    {   // bad assignement
        setup_test("main :: () {let 32 = 42;}");
        assert(prog == NULL && "Prog should be null");
        assert(parser.err.code == Parse_Err_UnexpectedToken && "Unenexpected error should be expected");
        assert(parser.err.formatted != NULL && "Error message should not be null");
        assert(parser.err.row == 1 && parser.err.col == 17 && "Error location shoud be correct");
        free(parser.err.formatted); // TODO: cleanup function
        printf("[TEST] Error assignement\n");
    }

    {   // block assignement TODO: Test inside of block
        setup_test("main :: () {let a = { 32 };}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements  = prog->statements.items[0].funcdef.block->statements.items;
        Node  assignement = statements[0];
        assert(assignement.kind == NodeKind_Assignement && "Statement should be an assignement");
        assert(sv_eq_string("a", assignement.assignement.ident) && "Assignement ident name should be a");
        Node *value = assignement.assignement.value;
        assert(value->kind == NodeKind_Block && "Assignement value should be a block");
        printf("[TEST] Block assignement\n");
    }

    {
        setup_test("main :: () {1 + 2}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements  = prog->statements.items[0].funcdef.block->statements.items;
        Node  expression = statements[0];
        assert(expression.kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->kind == Expr_Binary && "Expected a binary expression");
        assert(expression.expression->binary_op.lhs->kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->binary_op.rhs->kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->binary_op.lhs->expression->kind == Expr_Number && "Expected a number");
        assert(expression.expression->binary_op.rhs->expression->kind == Expr_Number && "Expected a number");
        assert(expression.expression->binary_op.lhs->expression->number == 1 && "Expected number to be 1");
        assert(expression.expression->binary_op.rhs->expression->number == 2 && "Expected number to be 2");
        printf("[TEST] Basic Expression\n");
    }

    {
        setup_test("main :: () {1 + 2;}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements  = prog->statements.items[0].funcdef.block->statements.items;
        Node  expression = statements[0];
        assert(expression.kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->kind == Expr_Binary && "Expected a binary expression");
        assert(expression.expression->binary_op.lhs->kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->binary_op.rhs->kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->binary_op.lhs->expression->kind == Expr_Number && "Expected a number");
        assert(expression.expression->binary_op.rhs->expression->kind == Expr_Number && "Expected a number");
        assert(expression.expression->binary_op.lhs->expression->number == 1 && "Expected number to be 1");
        assert(expression.expression->binary_op.rhs->expression->number == 2 && "Expected number to be 2");
        printf("[TEST] Basic Expression (with ;)\n");
    }

    {
        setup_test("main :: () {\"bijour\";}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements = prog->statements.items[0].funcdef.block->statements.items;
        Node  expression = statements[0];
        assert(expression.kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->kind == Expr_String && "Expected a string expression");
        assert(sv_eq_string("bijour", expression.expression->str) && "Expected string to be \"bijour\"");
        printf("[TEST] String expression (with ;)\n");
    }

    {
        setup_test("main :: () {\"bijour\" 1 + 2}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 2 && "should have 2 statement");
        Node *statements  = prog->statements.items[0].funcdef.block->statements.items;
        Node  string = statements[0];
        assert(string.kind == NodeKind_Expression && "Expected an expression");
        assert(string.expression->kind == Expr_String && "Expected a string expression");
        assert(sv_eq_string("bijour", string.expression->str) && "Expected string to be \"bijour\"");
        Node  binop = statements[1];
        assert(binop.kind == NodeKind_Expression && "Expected an expression");
        assert(binop.expression->kind == Expr_Binary && "Expected a binary expression");
        assert(binop.expression->binary_op.lhs->kind == NodeKind_Expression && "Expected an expression");
        assert(binop.expression->binary_op.rhs->kind == NodeKind_Expression && "Expected an expression");
        assert(binop.expression->binary_op.lhs->expression->kind == Expr_Number && "Expected a number");
        assert(binop.expression->binary_op.rhs->expression->kind == Expr_Number && "Expected a number");
        assert(binop.expression->binary_op.lhs->expression->number == 1 && "Expected number to be 1");
        assert(binop.expression->binary_op.rhs->expression->number == 2 && "Expected number to be 2");
        printf("[TEST] Two expression without ';' between\n");
    }

    {
        setup_test("main :: () { fn() }");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements = prog->statements.items[0].funcdef.block->statements.items;
        Node  expression = statements[0];
        assert(expression.kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->kind == Expr_Funcall && "Expected a funcall expression");
        assert(sv_eq_string("fn", expression.expression->funcall.name) && "Expected a function to be \"fn\"");
        assert(expression.expression->funcall.args.count == 0 && "Should have no argument passed");
        printf("[TEST] Funcall expression\n");

        setup_test("main :: () { fn(x, 32) }");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        statements = prog->statements.items[0].funcdef.block->statements.items;
        expression = statements[0];
        assert(expression.kind == NodeKind_Expression && "Expected an expression");
        assert(expression.expression->kind == Expr_Funcall && "Expected a funcall expression");
        assert(sv_eq_string("fn", expression.expression->funcall.name) && "Expected a function to be \"fn\"");
        assert(expression.expression->funcall.args.count == 2 && "Should have 2 arguments passed");
        assert(expression.expression->funcall.args.items[0].kind == NodeKind_Expression && "Arg 1 should be an expression");
        assert(expression.expression->funcall.args.items[1].kind == NodeKind_Expression && "Arg 2 should be an expression");
        assert(expression.expression->funcall.args.items[0].expression->kind == Expr_Ident && "Arg 1 should be an indentifier");
        assert(expression.expression->funcall.args.items[1].expression->kind == Expr_Number && "Arg 2 should be a number");
        assert(sv_eq_string("x", expression.expression->funcall.args.items[0].expression->ident)
                && "Arg 1 ident name should be \"x\"");
        assert(expression.expression->funcall.args.items[1].expression->number == 32 && "Arg 2 number should equal 32");
        printf("[TEST] Funcall expression with args\n");
    }

    {
        setup_test("main :: () {if 12 == 32 { println(\"bijour!\"); }}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node if_statement  = prog->statements.items[0].funcdef.block->statements.items[0];
        
        assert(if_statement.kind == NodeKind_If && "Expected an if");
        assert(if_statement.if_node.condition != NULL && "Expected condition to not be null");
        assert(if_statement.if_node.ok_node != NULL && "Expected block to not be null");

        Node  *binop = if_statement.if_node.condition;
        assert(binop->kind == NodeKind_Expression && "Expected an expression");
        assert(binop->expression->kind == Expr_Binary && "Expected a binary expression");
        assert(binop->expression->binary_op.lhs->kind == NodeKind_Expression && "Expected an expression");
        assert(binop->expression->binary_op.rhs->kind == NodeKind_Expression && "Expected an expression");
        assert(binop->expression->binary_op.lhs->expression->kind == Expr_Number && "Expected a number");
        assert(binop->expression->binary_op.rhs->expression->kind == Expr_Number && "Expected a number");
        assert(binop->expression->binary_op.lhs->expression->number == 12 && "Expected number to be 12");
        assert(binop->expression->binary_op.rhs->expression->number == 32 && "Expected number to be 32");

        Node *block = if_statement.if_node.ok_node;
        assert(block->kind == NodeKind_Block && "Expected a block");
        assert(block->statements.count == 1 && "Expected 1 statement in block");
        Node stmt = if_statement.if_node.ok_node->statements.items[0];
        assert(stmt.kind == NodeKind_Expression && "Expected statement to be an expression");
        assert(stmt.expression->kind == Expr_Funcall && "Expected expr to be a funcall");
        assert(sv_eq_string("println", stmt.expression->funcall.name) && "Expected funcall name to be 'println'");
        printf("[TEST] Test simple if\n");
    }

    {
        setup_test("main :: () {if (41 == 32) { println(\"bijour!\"); }}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node if_statement  = prog->statements.items[0].funcdef.block->statements.items[0];
        
        assert(if_statement.kind == NodeKind_If && "Expected an if");
        assert(if_statement.if_node.condition != NULL && "Expected condition to not be null");
        assert(if_statement.if_node.ok_node != NULL && "Expected block to not be null");

        Node  *binop = if_statement.if_node.condition;
        assert(binop->kind == NodeKind_Expression && "Expected an expression");
        assert(binop->expression->kind == Expr_Binary && "Expected a binary expression");
        assert(binop->expression->binary_op.lhs->kind == NodeKind_Expression && "Expected an expression");
        assert(binop->expression->binary_op.rhs->kind == NodeKind_Expression && "Expected an expression");
        assert(binop->expression->binary_op.lhs->expression->kind == Expr_Number && "Expected a number");
        assert(binop->expression->binary_op.rhs->expression->kind == Expr_Number && "Expected a number");
        assert(binop->expression->binary_op.lhs->expression->number == 41 && "Expected number to be 41");
        assert(binop->expression->binary_op.rhs->expression->number == 32 && "Expected number to be 32");

        Node *block = if_statement.if_node.ok_node;
        assert(block->kind == NodeKind_Block && "Expected a block");
        assert(block->statements.count == 1 && "Expected 1 statement in block");
        Node stmt = block->statements.items[0];
        assert(stmt.kind == NodeKind_Expression && "Expected statement to be an expression");
        assert(stmt.expression->kind == Expr_Funcall && "Expected expr to be a funcall");
        assert(sv_eq_string("println", stmt.expression->funcall.name) && "Expected funcall name to be 'println'");
        printf("[TEST] Test simple if with '()'\n");
    }

    { // 
        setup_test("main :: () {if (41 == 32 { println(\"bijour!\"); }}");
        assert(prog == NULL && "prog should be null");
        assert(parser.err.code && "parser should show an error");
        assert(parser.err.col == 26 && "error should be at col 26");
        printf("[TEST] Test simple if with missing one '()'\n");
    }

    {
        setup_test("main :: () {if 12 == 32 { println(\"yes!\"); } else { println(\"no!\"); }}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node if_statement  = prog->statements.items[0].funcdef.block->statements.items[0];

        assert(if_statement.kind == NodeKind_If && "Expected an if");
        assert(if_statement.if_node.condition != NULL && "Expected condition to not be null");
        assert(if_statement.if_node.ok_node != NULL && "Expected if-ok node to not be null");
        assert(if_statement.if_node.ko_node != NULL && "Expected else node to not be null");

        Node *else_node = if_statement.if_node.ko_node;
        assert(else_node->kind == NodeKind_Block && "Expected a block");
        assert(else_node->statements.count == 1 && "Expected 1 statement in block");
        Node stmt = else_node->statements.items[0];
        assert(stmt.kind == NodeKind_Expression && "Expected statement to be an expression");
        assert(stmt.expression->kind == Expr_Funcall && "Expected expr to be a funcall");
        assert(sv_eq_string("println", stmt.expression->funcall.name) && "Expected funcall name to be 'println'");
        // wild test (not checking kind nor if allocated)
        assert(sv_eq_string("no!", stmt.expression->funcall.args.items[0].expression->str) && "Expected funcall name to be 'println'");
        printf("[TEST] Test simple if-else\n");
    }

    {
        setup_test("main :: () {if 12 == 32 { println(\"yes!\"); } else if 32 == 1 { println(\"bij!\"); }}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node if_statement  = prog->statements.items[0].funcdef.block->statements.items[0];

        assert(if_statement.kind == NodeKind_If && "Expected an if");
        assert(if_statement.if_node.condition != NULL && "Expected condition to not be null");
        assert(if_statement.if_node.ok_node != NULL && "Expected if-ok node to not be null");
        assert(if_statement.if_node.ko_node != NULL && "Expected else node to not be null");

        Node *else_node = if_statement.if_node.ko_node;
        assert(else_node->kind == NodeKind_If && "Expected an if-block");
        assert(else_node->if_node.ok_node->kind == NodeKind_Block && "Expected a block in if");
        Node stmt = else_node->if_node.ok_node->statements.items[0];
        assert(stmt.kind == NodeKind_Expression && "Expected statement to be an expression");
        assert(stmt.expression->kind == Expr_Funcall && "Expected expr to be a funcall");
        assert(sv_eq_string("println", stmt.expression->funcall.name) && "Expected funcall name to be 'println'");
        // wild test (not checking kind nor if allocated)
        assert(sv_eq_string("bij!", stmt.expression->funcall.args.items[0].expression->str) && "Expected funcall name to be 'println'");
        printf("[TEST] Test if-else_if\n");
    }

    {
        setup_test("main :: () {if 12 == 32 { println(\"yes!\"); } else if 32 == 1 { println(\"bij!\"); } else { println(\"aled\"); }}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node if_statement  = prog->statements.items[0].funcdef.block->statements.items[0];

        assert(if_statement.kind == NodeKind_If && "Expected an if");
        assert(if_statement.if_node.condition != NULL && "Expected condition to not be null");
        assert(if_statement.if_node.ok_node != NULL && "Expected if-ok node to not be null");
        assert(if_statement.if_node.ko_node != NULL && "Expected else node to not be null");

        Node *elseif_node = if_statement.if_node.ko_node;
        assert(elseif_node->kind == NodeKind_If && "Expected an if-block");
        assert(elseif_node->if_node.ok_node->kind == NodeKind_Block && "Expected a block in if");
        Node stmt = elseif_node->if_node.ok_node->statements.items[0];
        assert(stmt.kind == NodeKind_Expression && "Expected statement to be an expression");
        assert(stmt.expression->kind == Expr_Funcall && "Expected expr to be a funcall");
        assert(sv_eq_string("println", stmt.expression->funcall.name) && "Expected funcall name to be 'println'");
        // wild test (not checking kind nor if allocated)
        assert(sv_eq_string("bij!", stmt.expression->funcall.args.items[0].expression->str) && "Expected funcall name to be 'println'");

        Node *else_node = if_statement.if_node.ko_node->if_node.ko_node;
        assert(else_node->kind == NodeKind_Block && "Expected a block");
        assert(else_node->statements.items[0].kind == NodeKind_Expression && "Expected a expr");
        assert(else_node->statements.items[0].expression->kind == Expr_Funcall && "Expected a funcall");
        assert(sv_eq_string("println", else_node->statements.items[0].expression->funcall.name) && "Expected fun name to be println");
        printf("[TEST] Test if-else_if-else\n");
    }

    areno_free(&lex_areno  );
    areno_free(&parse_areno);

    printf("-- END %s\n", __FILE_NAME__);
    return 0;
}
