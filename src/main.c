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

    Parser parser = (Parser) {
        parser.tokens = tokens,
    };

    Node prog = parser_parse(&parser, &parse_areno);
    dump_node(&prog, 0);

    areno_free(&lex_areno  );
    areno_free(&parse_areno);

    return 0;
}
