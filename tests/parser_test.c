#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

#include "../src/parser.h"
#include "../src/lexer.h"

#define STRING_VIEW_IMPLEMENTATION
#include "string_view.h"
#define ARENO_IMPLEMENTATION
#include "areno.h"

#define BUF_SIZE 1024 * 1024


int main(void)
{
    char *buf = "main :: () {"
"    yop(\"bijour\", 32);"
"    let b = ----!-23;"
"}"
""
"bijour :: () {"
"    yop(\"bijour\", 32);"
"    let b = 23;"
"}";

    Areno lex_areno   = {0};
    Areno parse_areno = {0};

    Lexer lexer = (Lexer) {
        .sv = (String_View) {
            .items = buf,
            .len   = strlen(buf),
        },
        .cursor = 0,
    };
    Token* tokens = lexer_lex(&lexer, &lex_areno);

    Parser parser = (Parser) {
        .lexer  = lexer,
        .tokens = tokens,
        .cursor = 0,
    };

    Node prog = parser_parse(&parser, &parse_areno);
    // dump_node(&prog, 0);

    areno_free(&lex_areno  );
    areno_free(&parse_areno);

    return 0;
}
