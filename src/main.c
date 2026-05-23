#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"

#define STRING_VIEW_IMPLEMENTATION
#include "string_view.h"
#define ARENO_IMPLEMENTATION
#include "areno.h"
#include "lexer.h"

#define BUF_SIZE 1024 * 1024


int main(void)
{
    char *path = "mini.ms";
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Could not open file at path: %s\n", path);
        return 1;
    }

    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    fread(buf, 1, BUF_SIZE, file);
    if (ferror(file)) {
        printf("Error while reading file at path: %s\n", path);
        return 1;
    }

    Areno lex_areno   = {0};

    Lexer lexer = (Lexer) {
        .sv = (String_View) {
            .items = buf,
            .len   = strlen(buf),
        },
        .cursor = 0,
    };
    Token* tokens = lexer_lex(&lexer, &lex_areno);

    Parser parser = (Parser) {
        .sv     = lexer.sv,
        .tokens = tokens,
        .cursor = 0,
    };

    Node *prog = parser_parse(&parser);
    areno_free(&lex_areno);
    if (prog == NULL) {
        printf("Error while parsing:\n%s", parser.err.formatted);
        parser_free(&parser);
        return 1;
    }

    dump_node(prog, 0);

    parser_free(&parser);

    return 0;
}
