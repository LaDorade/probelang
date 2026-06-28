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

Areno  global_areno = {0};
Lexer  lexer        = {0};
Token *tokens       = NULL;

void setup_test(char *buf)
{
    lexer = (Lexer) {0};
    lexer.areno = &global_areno;

    tokens = NULL;

    lexer.sv = (String_View) {
        .items = buf,
        .len   = strlen(buf),
    };
    tokens = lexer_lex(&lexer);

    test.nb_test++;
}

void TEST_ASSERT(int expected, const char* msg) {
    if (!expected) {
        fprintf(stderr, "> %s\n", msg);


        fprintf(stderr, "Error while Testing snippet:\n");
        fprintf(stderr, "%.*s\n", (int)lexer.sv.len, lexer.sv.items);

        for (size_t i = 0; i < lexer.err.count; i++) {
            Lex_Error err = lexer.err.items[i];

            size_t row = err.guilty.row;
            size_t col = err.guilty.col;
            printf("\e[1m" "%zu:%zu: " "\033[31m" "error:" "\033[m" " %s" "\e[m",
                    row, col,
                    err.formatted
                  );
            String_View line = sv_get_line(lexer.sv, row);
            printf("%.*s\n", (int)line.len, line.items);
        }

        fflush(stderr);
        abort();
    }
}

int main()
{
    printf("-- %s\n", __FILE__);
    {
        setup_test("");
        TEST_ASSERT(tokens != NULL, "Should support empty file");
        printf("[TEST] nothing\n");
    }

    printf("-- END %s\n", __FILE__);

    areno_free(&global_areno);
    return 0;
}

