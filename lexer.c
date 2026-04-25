#include "lexer.h"

#include "areno.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


#define BUF_SIZE 1024

char lex_peek(const Lexer *lex)
{
    if    (lex->eof) return 0;
    return lex->items[lex->cursor];
}

void lex_advance(Lexer *lex)
{
    if (lex->cursor >= lex->count) {
        lex->eof = 1;
        return;
    }
    lex->cursor += 1;
}

int lex_match(Lexer *lex, char c)
{
    if (lex_peek(lex) == c)
    {
        lex_advance(lex);
        return 1;
    }
    return 0;
}

Token* lexer_lex(Lexer *lexer, Areno* areno)
{
    size_t current_tok = 0;
    Token* tokens = (Token*) areno_alloc(areno, sizeof(Token) * BUF_SIZE);

    while(!lexer->eof)
    {
        char c = lex_peek(lexer);
        lex_advance(lexer);
        if (lexer->eof) { // end
            tokens[current_tok++] = (Token) { .kind = Lex_EOF, };
            break;
        }

        switch (c) {
            // double char Lexemes with single char alt
            case ':':
                if (lex_match(lexer, ':'))
                    tokens[current_tok++] = (Token) { .kind = Lex_Colon_Colon, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Colon, };
                continue;
            case '!':
                if (lex_match(lexer, '='))
                    tokens[current_tok++] = (Token) { .kind = Lex_Not_Equal, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Bang, };
                continue;
            case '=':
                if (lex_match(lexer, '='))
                    tokens[current_tok++] = (Token) { .kind = Lex_Equal_Equal, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Equal, };
                continue;
            case '<':
                if (lex_match(lexer, '='))
                    tokens[current_tok++] = (Token) { .kind = Lex_Lower_Equal, };
                else
                    tokens[current_tok++] = (Token) { .kind = Lex_Lower, };
                continue;
            case '>':
                if (lex_match(lexer, '='))
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

                while ((c = lex_peek(lexer)) != '"') {
                    wordBuf[len++] = c;
                    lex_advance(lexer);
                }
                lex_advance(lexer);

                char* items = areno_alloc(areno, len);
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

            while (isdigit((c = lex_peek(lexer))))
            {
                lex_advance(lexer);
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

            while (isalnum((c = lex_peek(lexer))))
            {
                lex_advance(lexer);
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
                char* items = areno_alloc(areno, len);
                strcpy(items, wordBuf);

                tokens[current_tok++] = (Token) {
                    .kind = Lex_Ident,
                    .ident = (String_View) {
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
    return tokens;
}

const char* lex_print(Lexeme lexeme)
{
    switch (lexeme) {
        // Single char lexeme
        case Lex_Colon:         return ":";
        case Lex_Comma:         return ",";
        case Lex_Semicolon:     return ";";
        case Lex_Dot:           return ".";
        case Lex_Open_bracket:  return "(";
        case Lex_Close_bracket: return ")";
        case Lex_Open_brace:    return "{";
        case Lex_Close_brace:   return "}";
        case Lex_Plus:          return "+";
        case Lex_Minus:         return "-";
        case Lex_Mul:           return "*";
        case Lex_Divide:        return "/";
        case Lex_Modulo:        return "%";
        case Lex_Bang:          return "!";
        case Lex_Question:      return "!";
        case Lex_Equal:         return "=";
        case Lex_Lower:         return "<";
        case Lex_Greater:       return ">";

        // Double char lexeme
        case Lex_Colon_Colon:   return "::";
        case Lex_Lower_Equal:   return "<=";
        case Lex_Greater_Equal: return ">=";
        case Lex_Equal_Equal:   return "==";
        case Lex_Not_Equal:     return "!=";

        // KEYWORDS
        case Lex_local:  return "local";
        case Lex_let:    return "let";
        case Lex_return: return "return";
        case Lex_reject: return "reject";
        case Lex_catch:  return "catch";

        case Lex_Ident:  return "IDENT";

        case Lex_Invalid:
        case Lex_EOF:
        case Lex_String:
        case Lex_Number:
            return "[Not Printable]";
    }
}
