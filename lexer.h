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

char        lex_peek(const Lexer *lex);
void        lex_advance(Lexer *lex);
int         lex_match(Lexer *lex, char c);
const char* lex_print(Lexeme lexeme);

#endif //LEXER_H_

#ifndef LEXER_IMPLEMENTATION

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

#endif //LEXER_IMPLEMENTATION
