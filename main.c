#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"

#define STRING_VIEW_IMPLEMENTATION
#include "string_view.h"
#define LEXER_IMPLEMENTATION
#include "lexer.h"
#define ARENO_IMPLEMENTATION
#include "areno.h"

#define BUF_SIZE 1024


int main(void)
{
    char *path = "probe.ms";
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Could not open file at path: %s\n", path);
        return 1;
    }

    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    unsigned long n = fread(buf, 1, BUF_SIZE, file);
    if (ferror(file)) {
        printf("Error while reading file at path: %s\n", path);
        return 1;
    }

    Areno lex_areno   = {0};
    Areno parse_areno = {0};

    Lexer lexer = (Lexer) {
        .items  = buf,
        .cursor = 0,
        .count  = strlen(buf),
    };
    Token* tokens = lexer_lex(&lexer, &lex_areno);

    Token tok = tokens[0];
    size_t count = 0;
    while (tok.kind != Lex_EOF) {
        printf("Token: %s at %ld:%ld\n", token_print(&tok, &lex_areno), tok.row, tok.col);
        tok = tokens[++count];
    }


    // Parser parser = (Parser) {
    //     parser.tokens = tokens,
    // };
    // Expr *expr = parse_expression(&parser, &parse_areno);
    // areno_free(&lex_areno);
    //
    // dump_expression(expr, 0);
    //
    // areno_free(&parse_areno);
    return 0;
}
