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

// Start the lexer, reset cursor, col & row
Token* lexer_lex(Lexer *lexer)
{
    size_t current_tok = 0;
    Token* tokens = (Token*) areno_alloc(lexer->areno, sizeof(Token) * BUF_SIZE);

    // Reset lexer to the start
    lexer->cursor = 0;
    lexer->col    = 0;
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

        switch (c) {
            Lexeme kind = Lex_Invalid;
            // tripe char Lexemes with double or simple char alt
            case '.':
                kind = lexer_match(lexer, '.')
                    ? lexer_match(lexer, '.')
                        ? Lex_Dot_Dot_Dot
                        : Lex_Dot_Dot
                    : Lex_Dot;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;

            // double char Lexemes with single char alt
            case '/': {
                if (lexer_match(lexer, '/')) { // comment
                    while (!lexer->eof && !lexer_match(lexer, '\n')) {
                        lexer_advance(lexer);
                    }
                } else { // divide
                    kind = Lex_Divide;
                    tokens[current_tok++] = token_create(lexer, kind);
                }
            } continue;
            case ':':
                kind = lexer_match(lexer, ':') ? Lex_Colon_Colon : Lex_Colon;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '!':
                kind = lexer_match(lexer, '=') ? Lex_Not_Equal : Lex_Bang;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '=':
                kind = lexer_match(lexer, '=') ? Lex_Equal_Equal : Lex_Equal;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '<':
                kind = lexer_match(lexer, '=') ? Lex_Lower_Equal : Lex_Lower;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '>':
                kind = lexer_match(lexer, '=') ? Lex_Greater_Equal : Lex_Greater;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '-':
                kind = lexer_match(lexer, '>') ? Lex_Arrow_Right : Lex_Minus;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;

            // single char Lexeme
            case ',':
                kind = Lex_Comma;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '?':
                kind = Lex_Question;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case ';':
                kind = Lex_Semicolon;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '(':
                kind = Lex_Open_Paren;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case ')':
                kind = Lex_Close_Paren;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '{':
                kind = Lex_Open_Curly;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '}':
                kind = Lex_Close_Curly;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '+':
                kind = Lex_Plus;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '*':
                kind = Lex_Mul;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '%':
                kind = Lex_Modulo;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '[':
                kind = Lex_Modulo;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case ']':
                kind = Lex_Modulo;
                tokens[current_tok++] = token_create(lexer, kind);
                continue;
            case '"':
                tokens[current_tok++] = lex_string(lexer);
                continue;
            default:
                break;
        }

        if (isdigit(c)) {
            tokens[current_tok++] = lex_digit(lexer);
        } else if (isalpha(c)) { // ident OR keyword
            tokens[current_tok++] = lex_ident(lexer);
        } else if (isspace(c)) {
            continue;
        } else {
            // TODO: lexer error handling
            printf("Unknown token: %c at %ld:%ld\n", c, lexer->row, lexer->col);
        }
    }
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
        printf("Conversion from '%s' to double precision float failed\n", numBuf);
        exit(1);
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

    while (!lexer->eof && isalnum((c = lexer_peek(lexer))))
    {
        lexer_advance(lexer);
        wordBuf[len++] = c;
    }

    Token tok = token_create(lexer, Lex_Invalid);
    if      (strcmp(wordBuf, "struct") == 0)
        tok.kind = Lex_struct;
    else if (strcmp(wordBuf, "union")  == 0)
        tok.kind = Lex_union;
    else if (strcmp(wordBuf, "enum")   == 0)
        tok.kind = Lex_enum;
    else if (strcmp(wordBuf, "type")   == 0)
        tok.kind = Lex_type;
    else if (strcmp(wordBuf, "module") == 0)
        tok.kind = Lex_module;
    else if (strcmp(wordBuf, "use")    == 0)
        tok.kind = Lex_use;
    else if (strcmp(wordBuf, "using")  == 0)
        tok.kind = Lex_using;
    else if (strcmp(wordBuf, "fun")    == 0)
        tok.kind = Lex_fun;
    else if (strcmp(wordBuf, "let")    == 0)
        tok.kind = Lex_let;
    else if (strcmp(wordBuf, "const")  == 0)
        tok.kind = Lex_const;
    else if (strcmp(wordBuf, "defer")  == 0)
        tok.kind = Lex_defer;
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
    else if (strcmp(wordBuf, "do")     == 0)
        tok.kind = Lex_do;
    else if (strcmp(wordBuf, "while")  == 0)
        tok.kind = Lex_while;
    else if (strcmp(wordBuf, "for")    == 0)
        tok.kind = Lex_for;
    else if (strcmp(wordBuf, "break")  == 0)
        tok.kind = Lex_break;
    else if (strcmp(wordBuf, "continue")  == 0)
        tok.kind = Lex_continue;
    else if (strcmp(wordBuf, "or")  == 0)
        tok.kind = Lex_or;
    else if (strcmp(wordBuf, "and")  == 0)
        tok.kind = Lex_and;
    else if (strcmp(wordBuf, "try")    == 0)
        tok.kind = Lex_try;
    else if (strcmp(wordBuf, "catch")  == 0)
        tok.kind = Lex_catch;
    else {
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
    return (Token) {
        .kind = lexeme,
        .col  = lex->col,
        .row  = lex->row,
        .size = lex->cursor - lex->start,
    };
}

//////////////////// PRINT ////////////////////////

const char* lexer_print(Lexeme lexeme)
{
    switch (lexeme) {
        // Single char lexeme
        case Lex_Colon:        return ":";
        case Lex_Comma:        return ",";
        case Lex_Semicolon:    return ";";
        case Lex_Dot:          return ".";
        case Lex_Open_Paren:   return "(";
        case Lex_Close_Paren:  return ")";
        case Lex_Open_Curly:   return "{";
        case Lex_Close_Curly:  return "}";
        case Lex_Open_Square:  return "[";
        case Lex_Close_Square: return "]";
        case Lex_Plus:         return "+";
        case Lex_Minus:        return "-";
        case Lex_Mul:          return "*";
        case Lex_Divide:       return "/";
        case Lex_Modulo:       return "%";
        case Lex_Bang:         return "!";
        case Lex_Question:     return "?";
        case Lex_Equal:        return "=";
        case Lex_Lower:        return "<";
        case Lex_Greater:      return ">";

        // Double char lexeme
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
        case Lex_struct:    return "struct";
        case Lex_union:     return "union";
        case Lex_enum:      return "enum";
        case Lex_type:      return "type";
        case Lex_interface: return "interface";

        case Lex_module:    return "module";
        case Lex_use:       return "use";
        case Lex_using:     return "using";

        case Lex_extern: return "extern";
        case Lex_static: return "static";
        case Lex_let:    return "let";
        case Lex_const:  return "const";

        case Lex_fun:  return "fun";

        case Lex_defer:  return "defer";
        case Lex_return: return "return";
        case Lex_reject: return "reject";
        case Lex_local:  return "local";

        case Lex_if:     return "if";
        case Lex_else:   return "else";
        case Lex_match:  return "match";
        case Lex_switch: return "switch";
        case Lex_break:  return "break";
        case Lex_or:     return "or";
        case Lex_and:    return "and";

        case Lex_do:     return "do";
        case Lex_while:  return "while";
        case Lex_for:    return "for";
        case Lex_continue: return "continue";

        case Lex_try:    return "try";
        case Lex_catch:  return "catch";

        case Lex_Ident:  return "IDENT";
        case Lex_Number: return "NUMBER";
        case Lex_String_Lit: return "STRING";

        case Lex_EOF:     return "EOF";
        case Lex_Invalid: return "INVALID LEXEME";
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
