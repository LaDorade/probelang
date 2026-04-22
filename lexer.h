#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>
#include "string_view.h"

typedef struct {
    char  *items;
    size_t cursor;
    size_t count;
    int    eof;
} Lexer;

typedef enum {
    Lex_Invalid = 0,

    // Single char lexeme
    Lex_Colon,         // :
    Lex_Comma,         // ,
    Lex_Semicolon,     // ;
    Lex_Dot,           // .
    Lex_Open_bracket,  // (
    Lex_Close_bracket, // )
    Lex_Open_brace,    // {
    Lex_Close_brace,   // }
    Lex_Plus,          // +
    Lex_Minus,         // -
    Lex_Mul,           // *
    Lex_Divide,        // /
    Lex_Modulo,        // %
    Lex_Bang,          // !
    Lex_Question,      // !
    Lex_Equal,         // =
    Lex_Lower,         // <
    Lex_Greater,       // >

    // Double char lexeme
    Lex_Colon_Colon,   // ::
    Lex_Lower_Equal,   // <=
    Lex_Greater_Equal, // >=
    Lex_Equal_Equal,   // ==
    Lex_Not_Equal,     // !=

    // KEYWORDS
    Lex_local,
    Lex_let,
    Lex_return,
    Lex_reject,
    Lex_catch,
    Lex_LastKeyword = Lex_catch,

    // Multi
    Lex_Ident,
    Lex_Number,
    Lex_String,

    Lex_EOF,
    Lex_LastLex = Lex_EOF
} Lexeme;

typedef struct {
    Lexeme kind;
    union {
        String_View string;
        int number;
    };
} Token;

char lex_peek(const Lexer *lex);
void lex_advance(Lexer *lex);
int  lex_match(Lexer *lex, char c);

#endif //LEXER_H_

#ifdef LEXER_IMPLEMENTATION

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

#endif //LEXER_IMPLEMENTATION
