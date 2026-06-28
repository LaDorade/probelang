#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"
#include "lexer.h"

#define ARENO_IMPLEMENTATION
#include "areno.h"
#define STRING_VIEW_IMPLEMENTATION
#include "string_view.h"

#define BUF_SIZE 1024 * 1024

// # Source - https://stackoverflow.com/a/71305350
// # Posted by Gabriel Staples, modified by community. See post 'Timeline' for change history
// # Retrieved 2026-06-05, License - CC BY-SA 4.0
//
// # ANSI Color Code Examples to help make sense of the regex expressions below
// # Git config color code descriptions; see here:
// # https://stackoverflow.com/questions/26941144/how-do-you-customize-the-color-of-the-diff-header-in-git-diff/61993060#61993060
// # ---------------    ----------------------------------------------------------------
// #                    Git config color code desription
// # ANSI Color Code    Order: text_color(x1) background_color(x1) attributes(0 or more)
// # ----------------   ----------------------------------------------------------------
// # \033[m             # code to turn off or "end" the previous color code
// # \033[1m            # "white"
// # \033[31m           # "red"
// # \033[32m           # "green"
// # \033[33m           # "yellow"
// # \033[34m           # "blue"
// # \033[36m           # "cyan"
// # \033[1;33m         # "yellow bold"
// # \033[1;36m         # "cyan bold"
// # \033[3;30;42m      # "black green italic" = black text with green background, italic text
// # \033[9;30;41m      # "black red strike" = black text with red background, strikethrough line through the text
//

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

    Areno global_areno = {0};

    String_View text = {buf, strlen(buf)};
    Lexer lexer = (Lexer) {
        .sv    = text,
        .areno = &global_areno,
    };
    Token* tokens = lexer_lex(&lexer);

    if (lexer.err.count > 0) {
        for (size_t i = 0; i < lexer.err.count; i++) {
            Lex_Error err = lexer.err.items[i];

            size_t row = err.guilty.row;
            size_t col = err.guilty.col;
            printf("\e[1m" "%s:%zu:%zu: " "\033[31m" "error:" "\033[m" " %s" "\e[m",
                    path, row, col,
                    err.formatted
                  );
            String_View line = sv_get_line(lexer.sv, row);
            printf("%.*s\n", (int)line.len, line.items);
            printf("%*s""\033[32m" "^" "\033[m\n", (int)col - 1, "");
        }
        fflush(stdout);
        return 1;
    }

    Parser parser = (Parser) {
        .tokens = tokens,
        .areno  = &global_areno,
    };


    Stmt *prog = parser_parse(&parser);
    if (prog == NULL) {
        size_t row  = parser.err.guilty.row;
        size_t col  = parser.err.guilty.col;
        size_t size = parser.err.guilty.size; 
        printf("\e[1m" "%s:%zu:%zu: " "\033[31m" "error:" "\033[m" " %s" "\e[m",
                path, row, col,
                parser.err.formatted
                );
        String_View line = sv_get_line(lexer.sv, row);
        printf("%.*s\n", (int)line.len, line.items);

        printf("%*s""\033[32m" "^" "\033[m", (int)col - 1, "");
        for (size_t i = 1; i < size; i++) printf("\033[32m" "~" "\033[m");
        printf("\n");

        fflush(stdout);
        return 1;
    }

    dump_stmt(prog, 0);

    areno_free(&global_areno);
    return 0;
}
