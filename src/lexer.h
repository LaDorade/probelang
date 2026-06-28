#ifndef LEXER_H_
#define LEXER_H_

#include <stdbool.h>
#include <stddef.h>

#include "areno.h"
#include "string_view.h"

#define SINGLE_CHARS \
    X(',', ",", Lex_Comma)        \
    X(';', ";", Lex_Semicolon)    \
    X('(', "(", Lex_Open_Paren)   \
    X(')', ")", Lex_Close_Paren)  \
    X('{', "{", Lex_Open_Curly)   \
    X('}', "}", Lex_Close_Curly)  \
    X('[', "[", Lex_Open_Square)  \
    X(']', "]", Lex_Close_Square) \
    X('+', "+", Lex_Plus)         \
    X('*', "*", Lex_Mul)          \
    X('%', "%", Lex_Modulo)       \
    X('?', "?", Lex_Question)     \


#define KEYWORDS \
    X(struct,Lex_struct) \
    X(union,Lex_union) \
    X(enum,Lex_enum) \
    X(type,Lex_type) \
    X(interface,Lex_interface) \
\
    X(module,Lex_module) \
    X(use,Lex_use) \
    X(using,Lex_using) \
\
    X(fun,Lex_fun) \
\
    X(extern,Lex_extern) \
    X(static,Lex_static) \
    X(let,Lex_let) \
    X(const,Lex_const) \
\
    X(defer,Lex_defer) \
    X(return,Lex_return) \
    X(reject,Lex_reject) \
    X(local,Lex_local) \
\
    X(if,Lex_if) \
    X(else,Lex_else) \
    X(match,Lex_match) \
    X(switch,Lex_switch) \
    X(break,Lex_break) \
    X(or,Lex_or) \
    X(and,Lex_and) \
\
    X(do,Lex_do) \
    X(while,Lex_while) \
    X(for,Lex_for) \
    X(continue,Lex_continue) \
\
    X(try,Lex_try) \
    X(catch,Lex_catch)

static const char* keywords[] = {
#define X(name, _) #name,
    KEYWORDS
#undef X
};

static const int keyword_nb = sizeof(keywords) / sizeof(keywords[0]);

typedef enum {
    Lex_Invalid = 0,

    // Single char lexeme
#define X(_, __, lexeme) lexeme,
    SINGLE_CHARS
#undef X

    // Double char lexeme
    Lex_Colon,         // :
    Lex_Colon_Colon,   // ::
    Lex_Divide,        // /
    Lex_Bang,          // !
    Lex_Equal,         // =
    Lex_Lower,         // <
    Lex_Greater,       // >
    Lex_Lower_Equal,   // <=
    Lex_Greater_Equal, // >=
    Lex_Equal_Equal,   // ==
    Lex_Not_Equal,     // !=
    Lex_Minus,         // -
    Lex_Arrow_Right,   // ->

    // Triple char lexeme
    Lex_Dot_Dot_Dot,   // ...
    Lex_Dot_Dot,       // ..
    Lex_Dot,           // .

    // KEYWORDS
#define X(_, lexeme) lexeme,
    KEYWORDS
#undef X

    // Multi
    Lex_Ident,
    Lex_Number,
    Lex_String_Lit,

    Lex_EOF,
    Lex_LastLex = Lex_EOF
} Lexeme;

static const Lexeme keyword_lexemes[] = {
#define X(_, lexeme) lexeme,
    KEYWORDS
#undef X
};

typedef struct {
    union {
        char        chr;
        double      number;
        String_View string;
        String_View ident;
    } as;

    size_t size; // nb of char of the Token
    size_t row; // 1 indexed
    size_t col; // 1 indexed
    Lexeme kind;
} Token;

typedef struct {
    Token  guilty;
    char  *formatted;
} Lex_Error;

typedef struct {
    Lex_Error *items;
    size_t     count;
    size_t     capacity;
} Lex_Errors;

typedef struct {
    Lex_Errors  err;
    String_View sv;
    Areno      *areno;
    size_t      cursor;
    // save the indice of start of current Token
    size_t start;
    size_t col, row;
    bool   eof;
} Lexer;


Token* lexer_lex (Lexer* lexer);
Token  lex_string(Lexer* lexer);
Token  lex_digit (Lexer* lexer);
Token  lex_ident (Lexer* lexer);


////////////////// UTILITIES //////////////////////
static inline char  lexer_peek   (const Lexer *lex);
static inline void  lexer_advance(Lexer *lex);
static inline char  lexer_prev   (const Lexer *lex);
static inline bool  lexer_match  (Lexer *lex, char c);
static inline Token token_create (const Lexer *lexer, Lexeme lexeme);

//////////////////// PRINT ////////////////////////
const char* lexer_print(Lexeme lexeme);
char*       token_print (const Token *tok, Areno *areno);

#endif //LEXER_H_

