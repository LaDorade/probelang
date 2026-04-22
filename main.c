#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>

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

    Lexer lexer = (Lexer) {
        .items  = buf,
        .cursor = 0,
        .count  = strlen(buf),
    };

    Areno areno = {0};
    size_t current_tok = 0;
    Token tokens[BUF_SIZE];
    memset(tokens, 0, sizeof(tokens));
    while(!lexer.eof)
    {
        char c = lex_peek(&lexer);
        lex_advance(&lexer);
        if (lexer.eof) { // end
            tokens[current_tok++] = (Token) { .kind = Lex_EOF, };
            break;
        }

        switch (c) {
            // double char Lexemes with single char alt
            case ':':
                if (lex_match(&lexer, ':'))
                    tokens[current_tok++] = (Token) { .kind = Lex_Colon_Colon, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Colon, };
                continue;
            case '!':
                if (lex_match(&lexer, '='))
                    tokens[current_tok++] = (Token) { .kind = Lex_Not_Equal, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Bang, };
                continue;
            case '=':
                if (lex_match(&lexer, '='))
                    tokens[current_tok++] = (Token) { .kind = Lex_Equal_Equal, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Equal, };
                continue;
            case '<':
                if (lex_match(&lexer, '='))
                    tokens[current_tok++] = (Token) { .kind = Lex_Lower_Equal, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Lower, };
                continue;
            case '>':
                if (lex_match(&lexer, '='))
                    tokens[current_tok++] = (Token) { .kind = Lex_Greater_Equal, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Greater, };
                continue;

            // single char Lexeme
            case ',':
                tokens[current_tok++] = (Token) {.kind = Lex_Comma};
                continue;
            case '?':
                tokens[current_tok++] = (Token) {.kind = Lex_Question};
                continue;
            case '.':
                tokens[current_tok++] = (Token) {.kind = Lex_Dot};
                continue;
            case ';':
                tokens[current_tok++] = (Token) {.kind = Lex_Semicolon};
                continue;
            case '(':
                tokens[current_tok++] = (Token) {.kind = Lex_Open_bracket};
                continue;
            case ')':
                tokens[current_tok++] = (Token) {.kind = Lex_Close_bracket};
                continue;
            case '{':
                tokens[current_tok++] = (Token) {.kind = Lex_Open_brace};
                continue;
            case '}':
                tokens[current_tok++] = (Token) {.kind = Lex_Close_brace};
                continue;
            case '+':
                tokens[current_tok++] = (Token) {.kind = Lex_Plus};
                continue;
            case '-':
                tokens[current_tok++] = (Token) {.kind = Lex_Minus};
                continue;
            case '*':
                tokens[current_tok++] = (Token) {.kind = Lex_Mul};
                continue;
            case '/':
                tokens[current_tok++] = (Token) {.kind = Lex_Divide};
                continue;
            case '%':
                tokens[current_tok++] = (Token) {.kind = Lex_Modulo};
                continue;
            case '"': {
                size_t len = 0;
                char wordBuf[BUF_SIZE];
                memset(wordBuf, 0, sizeof(wordBuf));

                while ((c = lex_peek(&lexer)) != '"') {
                    wordBuf[len++] = c;
                    lex_advance(&lexer);
                }
                lex_advance(&lexer);

                char* items = areno_alloc(&areno, len);
                strcpy(items, wordBuf);
                tokens[current_tok++] = (Token) {
                    .kind = Lex_String,
                    .string = (String_View) {
                        .items = items,
                        .len = len,
                    }
                };
                continue;
            }
            default:
                break;
        }

        if (isdigit(c))
        {
            size_t len = 0;
            char numBuf[BUF_SIZE];
            memset(numBuf, 0, sizeof(numBuf));
            numBuf[len++] = c;

            while (isdigit((c = lex_peek(&lexer))))
            {
                lex_advance(&lexer);
                numBuf[len++] = c;
            }

            tokens[current_tok++] = (Token) {
                .kind = Lex_Number,
                .number = (int)strtol(numBuf, NULL, 10),
            };
            if (errno != 0) {
                printf("Conversion from '%s' to number failed\n", numBuf);
                exit(1);
            }
            continue;

        } else if (isalpha(c))
        { // ident OR keyword
            size_t len = 0;
            char wordBuf[BUF_SIZE];
            memset(wordBuf, 0, sizeof(wordBuf));
            wordBuf[len++] = c;

            while (isalnum((c = lex_peek(&lexer))))
            {
                lex_advance(&lexer);
                wordBuf[len++] = c;
            }

            if (strcmp(wordBuf, "return") == 0)
                tokens[current_tok++] = (Token) {.kind = Lex_return};
            else if (strcmp(wordBuf, "reject") == 0)
                tokens[current_tok++] = (Token) {.kind = Lex_reject};
            else if (strcmp(wordBuf, "let")    == 0)
                tokens[current_tok++] = (Token) {.kind = Lex_let};
            else if (strcmp(wordBuf, "catch")  == 0)
                tokens[current_tok++] = (Token) {.kind = Lex_catch};
            else if (strcmp(wordBuf, "local")  == 0)
                tokens[current_tok++] = (Token) {.kind = Lex_local};
            else {
                char* items = areno_alloc(&areno, len);
                strcpy(items, wordBuf);
                tokens[current_tok++] = (Token) {
                    .kind = Lex_Ident,
                    .string = (String_View) {
                        .items = items,
                        .len = len,
                    }
                };
            }
            continue;

        } else if (isspace(c))
        {
            continue;

        } else
        {
            printf("Unknown token: %c\n", c);
        }
    }

    for (int i = 0; i < BUF_SIZE; i++) {
        Token tok = tokens[i];
        if (tok.kind == Lex_Ident)
            printf("Iden: %*s\n", (int)tok.string.len, tok.string.items);
        else if (tok.kind == Lex_Number)
            printf("Number: %d\n", tok.number);
        else if (tok.kind == Lex_String)
            printf("String: %*s\n", (int)tok.string.len, tok.string.items);
        else if (tok.kind == Lex_EOF) break;
    }

    return 0;
}
