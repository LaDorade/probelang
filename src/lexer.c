#include "lexer.h"

#include "areno.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


#define BUF_SIZE 1024

char lex_peek(const Lexer *lex)
{
    if (lex->eof) return 0;
    return lex->sv.items[lex->cursor];
}

void lex_advance(Lexer *lex)
{
    if (lex_peek(lex) == '\n') {
        lex->row += 1;
        lex->col  = 0;
    } else {
        lex->col += 1;
    }

    if (lex->cursor >= lex->sv.len) {
        lex->eof = true;
        return;
    }
    lex->cursor += 1;
}

bool lex_match(Lexer *lex, char c)
{
    if (lex_peek(lex) == c)
    {
        lex_advance(lex);
        return true;
    }
    return false;
}

// Start the lexer, reset cursor, col & row
Token* lexer_lex(Lexer *lexer, Areno* areno)
{
    size_t current_tok = 0;
    Token* tokens = (Token*) areno_alloc(areno, sizeof(Token) * BUF_SIZE);

    // Reset lexer to the start
    lexer->cursor = 0;
    lexer->col    = 0;
    lexer->row    = 1;

    while(!lexer->eof)
    {
        char c = lex_peek(lexer);
        lex_advance(lexer);

        Token tok = (Token) {
            .kind = Lex_Invalid,
            .col  = lexer->col,
            .row  = lexer->row,
        };

        if (lexer->eof) { // end
            tok.kind = Lex_EOF;
            tokens[current_tok++] = tok;
            break;
        }

        switch (c) {
            // double char Lexemes with single char alt
            case ':':
                tok.kind = lex_match(lexer, ':') ? Lex_Colon_Colon : Lex_Colon;
                tokens[current_tok++] = tok;
                continue;
            case '!':
                tok.kind = lex_match(lexer, '=') ? Lex_Not_Equal : Lex_Bang;
                tokens[current_tok++] = tok;
                continue;
            case '=':
                tok.kind = lex_match(lexer, '=') ? Lex_Equal_Equal : Lex_Equal;
                tokens[current_tok++] = tok;
                continue;
            case '<':
                tok.kind = lex_match(lexer, '=') ? Lex_Lower_Equal : Lex_Lower;
                tokens[current_tok++] = tok;
                continue;
            case '>':
                tok.kind = lex_match(lexer, '=') ? Lex_Greater_Equal : Lex_Greater;
                tokens[current_tok++] = tok;
                continue;

            // single char Lexeme
            case ',':
                tok.kind = Lex_Comma;
                tokens[current_tok++] = tok;
                continue;
            case '?':
                tok.kind = Lex_Question;
                tokens[current_tok++] = tok;
                continue;
            case '.':
                tok.kind = Lex_Dot;
                tokens[current_tok++] = tok;
                continue;
            case ';':
                tok.kind = Lex_Semicolon;
                tokens[current_tok++] = tok;
                continue;
            case '(':
                tok.kind = Lex_Open_bracket;
                tokens[current_tok++] = tok;
                continue;
            case ')':
                tok.kind = Lex_Close_bracket;
                tokens[current_tok++] = tok;
                continue;
            case '{':
                tok.kind = Lex_Open_brace;
                tokens[current_tok++] = tok;
                continue;
            case '}':
                tok.kind = Lex_Close_brace;
                tokens[current_tok++] = tok;
                continue;
            case '+':
                tok.kind = Lex_Plus;
                tokens[current_tok++] = tok;
                continue;
            case '-':
                tok.kind = Lex_Minus;
                tokens[current_tok++] = tok;
                continue;
            case '*':
                tok.kind = Lex_Mul;
                tokens[current_tok++] = tok;
                continue;
            case '/':
                tok.kind = Lex_Divide;
                tokens[current_tok++] = tok;
                continue;
            case '%':
                tok.kind = Lex_Modulo;
                tokens[current_tok++] = tok;
                continue;
            case '[':
                tok.kind = Lex_Modulo;
                tokens[current_tok++] = tok;
                continue;
            case ']':
                tok.kind = Lex_Modulo;
                tokens[current_tok++] = tok;
                continue;
            case '"': {
                size_t len = 0;
                char wordBuf[BUF_SIZE];
                memset(wordBuf, 0, sizeof(wordBuf));

                // TODO: handle escape char and new lines in strings
                while ((c = lex_peek(lexer)) != '"') {
                    wordBuf[len++] = c;
                    lex_advance(lexer);
                }
                lex_advance(lexer);

                char* items = areno_alloc(areno, len);
                strcpy(items, wordBuf);
                tok.kind   = Lex_String;
                tok.string = (String_View) {
                    .items = items,
                    .len   = len,
                };
                tokens[current_tok++] = tok;
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

            tok.kind = Lex_Number;
            tok.number = (int)strtol(numBuf, NULL, 10);
            if (errno != 0) {
                printf("Conversion from '%s' to number failed\n", numBuf);
                exit(1);
            }
            tokens[current_tok++] = tok;
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

            if      (strcmp(wordBuf, "struct") == 0)
                tok.kind = Lex_struct;
            else if (strcmp(wordBuf, "union")  == 0)
                tok.kind = Lex_union;
            else if (strcmp(wordBuf, "enum")   == 0)
                tok.kind = Lex_enum;
            else if (strcmp(wordBuf, "type")   == 0)
                tok.kind = Lex_type;
            else if (strcmp(wordBuf, "let")    == 0)
                tok.kind = Lex_let;
            else if (strcmp(wordBuf, "const")  == 0)
                tok.kind = Lex_const;
            else if (strcmp(wordBuf, "return") == 0)
                tok.kind = Lex_return;
            else if (strcmp(wordBuf, "reject") == 0)
                tok.kind = Lex_reject;
            else if (strcmp(wordBuf, "local")  == 0)
                tok.kind = Lex_local;
            else if (strcmp(wordBuf, "if")     == 0)
                tok.kind = Lex_if;
            else if (strcmp(wordBuf, "else")   == 0)
                tok.kind = Lex_else;
            else if (strcmp(wordBuf, "switch") == 0)
                tok.kind = Lex_switch;
            else if (strcmp(wordBuf, "match")  == 0)
                tok.kind = Lex_match;
            else if (strcmp(wordBuf, "while")  == 0)
                tok.kind = Lex_while;
            else if (strcmp(wordBuf, "for")    == 0)
                tok.kind = Lex_for;
            else if (strcmp(wordBuf, "try")    == 0)
                tok.kind = Lex_try;
            else if (strcmp(wordBuf, "catch")  == 0)
                tok.kind = Lex_catch;
            else {
                char* items = areno_alloc(areno, len);
                strcpy(items, wordBuf);

                tok.kind  = Lex_Ident;
                tok.ident = (String_View) {
                    .items = items,
                    .len   = len,
                };
            }
            tokens[current_tok++] = tok;
        continue;

        } else if (isspace(c))
        {
            continue;

        } else
        {
            printf("Unknown token: %c at %ld:%ld\n", c, lexer->row, lexer->col);
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
        case Lex_Open_square:   return "[";
        case Lex_Close_square:  return "]";

        // Double char lexeme
        case Lex_Colon_Colon:   return "::";
        case Lex_Lower_Equal:   return "<=";
        case Lex_Greater_Equal: return ">=";
        case Lex_Equal_Equal:   return "==";
        case Lex_Not_Equal:     return "!=";

        // KEYWORDS
        case Lex_struct:    return "struct";
        case Lex_union:     return "union";
        case Lex_enum:      return "enum";
        case Lex_type:      return "type";
        case Lex_interface: return "interface";

        case Lex_extern: return "extern";
        case Lex_static: return "static";
        case Lex_let:    return "let";
        case Lex_const:  return "const";

        case Lex_defer:  return "defer";
        case Lex_return: return "return";
        case Lex_reject: return "reject";
        case Lex_local:  return "local";

        case Lex_if:     return "if";
        case Lex_else:   return "else";
        case Lex_match:  return "match";
        case Lex_switch: return "switch";

        case Lex_while:  return "while";
        case Lex_for:    return "for";

        case Lex_try:    return "try";
        case Lex_catch:  return "catch";

        case Lex_Ident:  return "IDENT";
        case Lex_Number: return "NUMBER";
        case Lex_String: return "STRING";

        case Lex_EOF:     return "EOF";
        case Lex_Invalid: return "INVALID LEXEME";
    }
    assert(0 && "UNREACHABLE");
    return "UNREACHABLE";
}

char *str_printf(Areno *areno, const char *fmt, ...) {
    va_list args;
    va_list args2;

    va_start(args, fmt);
    va_copy(args2, args);
    va_start(args2, fmt);

    int size  = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (size < 0) {
        va_end(args2);
        return NULL;
    }

    char *str = areno_alloc(areno, size);
    vsnprintf(str, size + 1, fmt, args2);

    va_end(args2);
    return str;
}

// Return a NULL terminated string representing the token
char *token_print(const Token *tok, Areno *areno)
{
    const char *lexeme = lex_print(tok->kind);
    char *str = NULL;
    const char *string_format  = "%s: '%.*s'";
    const char *number_format  = "%s: '%d'";
    const char *default_format = "'%s'";

    if (tok->kind == Lex_Ident) {
        str = str_printf(areno, string_format, lexeme, (int)tok->ident.len, tok->ident.items);
    } else if (tok->kind == Lex_String) {
        str = str_printf(areno, string_format, lexeme, (int)tok->string.len, tok->string.items);
    } else if (tok->kind == Lex_Number){
        str = str_printf(areno, number_format, lexeme, tok->number);
    } else {
        str = str_printf(areno, default_format, lexeme);
    }

    return str;
}
