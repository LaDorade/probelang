#ifndef LEXER_H_
#define LEXER_H_

#include <stdbool.h>
#include <stddef.h>
#include "areno.h"
#include "string_view.h"

typedef struct {
    size_t      cursor;
    String_View sv;
    size_t      col, row;
    bool        eof;
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
    Lex_Open_square,   // [
    Lex_Close_square,  // ]

    // Double char lexeme
    Lex_Colon_Colon,   // ::
    Lex_Lower_Equal,   // <=
    Lex_Greater_Equal, // >=
    Lex_Equal_Equal,   // ==
    Lex_Not_Equal,     // !=

    // KEYWORDS
    Lex_struct,
    Lex_union,
    Lex_enum,
    Lex_type,

    Lex_let,
    Lex_const,

    Lex_return,
    Lex_reject,
    Lex_local,

    Lex_if,
    Lex_else,
    Lex_match,
    Lex_switch,

    Lex_while,
    Lex_for,

    Lex_try,
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
    size_t row;
    size_t col;
    union {
        int         number;
        String_View string;
        String_View ident;
    };
} Token;

Token*      lexer_lex(Lexer* lexer, Areno* areno);
char        lex_peek(const Lexer *lex);
void        lex_advance(Lexer *lex);
bool        lex_match(Lexer *lex, char c);
const char* lex_print(Lexeme lexeme);
char*       token_print(const Token *tok, Areno *areno);

#endif //LEXER_H_

