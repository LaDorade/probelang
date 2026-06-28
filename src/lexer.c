#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "areno.h"

#define BUF_SIZE 1024

// Start the lexer, reset cursor, col & row
Token* lexer_lex(Lexer *lexer)
{
    size_t current_tok = 0;
    Token* tokens = (Token*) areno_alloc(lexer->areno, sizeof(Token) * BUF_SIZE);

    // Reset lexer to the start
    lexer->cursor = 0;
    lexer->col    = 1;
    lexer->row    = 1;

    while(!lexer->eof)
    {
        lexer->start = lexer->cursor;
        char c = lexer_peek(lexer);
        lexer_advance(lexer);

        if (lexer->eof) { // end
            tokens[current_tok++] = token_create(lexer, Lex_EOF);
            break;
        }

        Lexeme kind = Lex_Invalid;
        switch (c) {
            case '.':
                kind = lexer_match(lexer, '.')
                    ? lexer_match(lexer, '.')
                        ? Lex_Dot_Dot_Dot
                        : Lex_Dot_Dot
                    : Lex_Dot;
                break;

            case '/': {
                if (lexer_match(lexer, '/')) { // comment
                    while (!lexer->eof && !lexer_match(lexer, '\n')) {
                        lexer_advance(lexer);
                    }
                    continue;
                } else { // divide
                    kind = Lex_Divide;
                }
            }
            case '"': // string lit
                tokens[current_tok++] = lex_string(lexer);
                continue;

            case ':':
                kind = lexer_match(lexer, ':') ? Lex_Colon_Colon : Lex_Colon;
                break;
            case '!':
                kind = lexer_match(lexer, '=') ? Lex_Not_Equal : Lex_Bang;
                break;
            case '=':
                kind = lexer_match(lexer, '=') ? Lex_Equal_Equal : Lex_Equal;
                break;
            case '<':
                kind = lexer_match(lexer, '=') ? Lex_Lower_Equal : Lex_Lower;
                break;
            case '>':
                kind = lexer_match(lexer, '=') ? Lex_Greater_Equal : Lex_Greater;
                break;
            case '-':
                kind = lexer_match(lexer, '>') ? Lex_Arrow_Right : Lex_Minus;
                break;

                // single char Lexeme
            #define X(c, _, lexeme) case c: kind = lexeme; break;
                SINGLE_CHARS
            #undef X

        }
        if (kind != Lex_Invalid) {
            tokens[current_tok++] = token_create(lexer, kind);
            continue;
        }

        if (isdigit(c)) {
            tokens[current_tok++] = lex_digit(lexer);
        } else if (isalpha(c) || c == '_') { // ident OR keyword
            tokens[current_tok++] = lex_ident(lexer);
        } else if (isspace(c)) {
            continue;
        } else {
            Token guilty = token_create(lexer, Lex_Invalid);
            guilty.as.chr = c;
            Lex_Error err = (Lex_Error) {
                .guilty = guilty,
                .formatted = areno_printf(lexer->areno, "Unrecognized char '%c'\n", c),
            };
            areno_arr_push(lexer->areno, &lexer->err, err);
        }
    }

    // maybe remove ?
    // we sure know that we errored, but lexer
    // is in a valid state
    // But, we dont want to continue (sure?)
    if (lexer->err.count) return NULL; 

    return tokens;
}

Token lex_string(Lexer *lexer)
{
    size_t len = 0;
    char wordBuf[BUF_SIZE];
    memset(wordBuf, 0, sizeof(wordBuf));

    char c = '\0';
    // TODO: handle escaped char
    while (!lexer->eof && (c = lexer_peek(lexer)) != '"') {
        wordBuf[len++] = c;
        lexer_advance(lexer);
    }
    lexer_advance(lexer); // closing "

    char* items = areno_alloc(lexer->areno, len);
    strcpy(items, wordBuf);
    Token tok = token_create(lexer, Lex_String_Lit);
    tok.as.string = (String_View) {
        .items = items,
        .len   = len,
    };
    return tok;
}

Token lex_digit(Lexer *lexer)
{
    char c = lexer_prev(lexer);
    size_t len = 0;
    char numBuf[BUF_SIZE];
    memset(numBuf, 0, sizeof(numBuf));
    numBuf[len++] = c;

    while (!lexer->eof && isdigit((c = lexer_peek(lexer))))
    {
        lexer_advance(lexer);
        numBuf[len++] = c;
    }

    Token tok = token_create(lexer, Lex_Number);
    tok.as.number = strtod(numBuf, NULL);
    if (errno != 0) {
        tok.as.number = 0;
        Lex_Error err = (Lex_Error) {
            .guilty = tok,
            .formatted = areno_printf(lexer->areno, "Conversion from '%s' to double precision float failed\n", numBuf),
        };
        areno_arr_push(lexer->areno, &lexer->err, err);
    }
    return tok;
}

Token lex_ident(Lexer *lexer)
{
    char c = lexer_prev(lexer);
    size_t len = 0;
    char wordBuf[BUF_SIZE];
    memset(wordBuf, 0, sizeof(wordBuf));
    wordBuf[len++] = c;

    while (!lexer->eof && (isalnum((c = lexer_peek(lexer))) || c == '_'))
    {
        lexer_advance(lexer);
        wordBuf[len++] = c;
    }

    Token tok = token_create(lexer, Lex_Invalid);
    for (size_t i = 0; i < keyword_nb; i++) {
        if (strcmp(wordBuf, keywords[i]) == 0) {
            tok.kind = keyword_lexemes[i];
            break;
        }
    }

    if (tok.kind == Lex_Invalid) { // not a keyword
        char* items = areno_alloc(lexer->areno, len);
        strcpy(items, wordBuf);

        tok.kind = Lex_Ident;
        tok.as.ident = (String_View) {
            .items = items,
            .len   = len,
        };
    }

    return tok;
}


////////////////// UTILITIES //////////////////////

static inline char lexer_peek(const Lexer *lex)
{
    if (lex->eof) return 0;
    return lex->sv.items[lex->cursor];
}

static inline void lexer_advance(Lexer *lex)
{
    if (lexer_peek(lex) == '\n') {
        lex->row += 1;
        lex->col  = 0; // \n count as the "first" (0 indexed) char
    } else {
        lex->col += 1;
    }

    if (lex->cursor >= lex->sv.len) {
        lex->eof = true;
        return;
    }
    lex->cursor += 1;
}

// return last char, first one of the string view if cursor is 0
static inline char lexer_prev(const Lexer *lex)
{
    if (lex->cursor <= 0) return lex->sv.items[0];
    return lex->sv.items[lex->cursor - 1];
}

static inline bool lexer_match(Lexer *lex, char c)
{
    if (lexer_peek(lex) == c)
    {
        lexer_advance(lex);
        return true;
    }
    return false;
}

static inline Token token_create(const Lexer* lex, Lexeme lexeme)
{
    // is always minimum at 1 except for EOF or INVALID
    size_t size = lex->cursor - lex->start;
    return (Token) {
        .kind = lexeme,
        .col  = lex->col - size + 1,
        .row  = lex->row,
        .size = size,
    };
}

//////////////////// PRINT ////////////////////////

const char* lexer_print(Lexeme lexeme)
{
    switch (lexeme) {
        // Single char lexeme
#define X(_, str, lexeme) case lexeme: return str;
    SINGLE_CHARS
#undef X

        // Double char lexeme
        case Lex_Colon:         return ":";
        case Lex_Dot:           return ".";
        case Lex_Divide:        return "/";
        case Lex_Bang:          return "!";
        case Lex_Equal:         return "=";
        case Lex_Lower:         return "<";
        case Lex_Greater:       return ">";
        case Lex_Minus:         return "-";
        case Lex_Dot_Dot:       return "..";
        case Lex_Colon_Colon:   return "::";
        case Lex_Lower_Equal:   return "<=";
        case Lex_Greater_Equal: return ">=";
        case Lex_Equal_Equal:   return "==";
        case Lex_Not_Equal:     return "!=";
        case Lex_Arrow_Right:   return "->";

        // Double char lexeme
        case Lex_Dot_Dot_Dot:   return "...";

        // KEYWORDS
#define X(name, lexeme) case lexeme: return #name;
    KEYWORDS
#undef X

        case Lex_Ident:      return "IDENT";
        case Lex_Number:     return "NUMBER";
        case Lex_String_Lit: return "STRING";

        case Lex_EOF:        return "EOF";
        case Lex_Invalid:    return "INVALID LEXEME";
    }
    assert(0 && "UNREACHABLE");
    return "UNREACHABLE";
}

// Return a NULL terminated string representing the token
char *token_print(const Token *tok, Areno *areno)
{
    const char *lexeme = lexer_print(tok->kind);
    char *str = NULL;
    const char *string_format  = "%s: (%.*s)";
    const char *number_format  = "%s: (%d)";
    const char *default_format = "%s";

    if (tok->kind == Lex_Ident) {
        str = areno_printf(areno, string_format, lexeme, (int)tok->as.ident.len, tok->as.ident.items);
    } else if (tok->kind == Lex_String_Lit) {
        str = areno_printf(areno, string_format, lexeme, (int)tok->as.string.len, tok->as.string.items);
    } else if (tok->kind == Lex_Number){
        str = areno_printf(areno, number_format, lexeme, tok->as.number);
    } else {
        str = areno_printf(areno, default_format, lexeme);
    }

    return str;
}
