#include "lexer.h"

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
