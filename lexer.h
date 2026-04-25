#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>
#include "areno.h"
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
        int         number;
        String_View string;
        String_View ident;
    };
} Token;

Token*      lexer_lex(Lexer* lexer, Areno* areno);
char        lex_peek(const Lexer *lex);
void        lex_advance(Lexer *lex);
int         lex_match(Lexer *lex, char c);
const char* lex_print(Lexeme lexeme);

#endif //LEXER_H_

