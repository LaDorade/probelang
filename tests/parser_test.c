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
    }

    {
        setup_test("main    :: () {yop(\"bijour\", 32);let b = ----!-23;}"
                    "bijour :: () {yop(\"bijour\", 32);let b = 23;" "}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->kind == NodeKind_Block && "Root node should be of kind NodeKind_Block");
        assert(prog->statements.count == 2 && "Should have 2 statements");
        printf("[TEST] Tested root node\n");
    }

    {   // should support 0 statements
        setup_test("main :: () {}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].kind == NodeKind_Funcdef &&
                "first statement should be a function");
        // verbeux
        assert(prog->statements.items[0].funcdef.block->kind == NodeKind_Block && "should have a block");
        assert(prog->statements.items[0].funcdef.block->statements.count == 0 && "should have no statements");
        printf("[TEST] Tested empty function\n");
    }

    {   // assignement
        setup_test("main :: () {let a = 42;}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements  = prog->statements.items[0].funcdef.block->statements.items;
        Node  assignement = statements[0];

        assert(assignement.kind == NodeKind_Assignation && "Statement should be an assignement");
        assert(sv_eq_string("a", assignement.assignement.ident) && "Assignement ident name should be a");
        Node *value = assignement.assignement.value;
        assert(value->kind == NodeKind_Expression && "Assignement value should be an expression");
        assert(value->expression->kind == Expr_Number && "Assignement value kind should be a number");
        assert(value->expression->number == 42 && "Assignement value value should be 42");
        printf("[TEST] Tested standard assignement\n");
    }

    {   // bad assignement
        setup_test("main :: () {let 32 = 42;}");
        assert(prog == NULL && "Prog should be null");
        assert(parser.err.code == Parse_Err_UnexpectedToken && "Unenexpected error should be expected");
        assert(parser.err.formatted != NULL && "Error message should not be null");
        assert(parser.err.row == 1 && parser.err.col == 17 && "Error location shoud be correct");
        free(parser.err.formatted); // TODO: cleanup function
        printf("[TEST] Tested error assignement\n");
    }

    {   // block assignement TODO: Test inside of block
        setup_test("main :: () {let a = { 32 };}");
        assert(prog != NULL && "prog should not be null");
        assert(prog->statements.items[0].funcdef.block->statements.count == 1 && "should have 1 statement");
        Node *statements  = prog->statements.items[0].funcdef.block->statements.items;
        Node  assignement = statements[0];
        assert(assignement.kind == NodeKind_Assignation && "Statement should be an assignement");
        assert(sv_eq_string("a", assignement.assignement.ident) && "Assignement ident name should be a");
        Node *value = assignement.assignement.value;
        assert(value->kind == NodeKind_Block && "Assignement value should be a block");
    }

    areno_free(&lex_areno  );
    areno_free(&parse_areno);

    printf("-- END %s\n", __FILE_NAME__);
    return 0;
}
