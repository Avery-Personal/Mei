#include <string.h>

#include "Syntax.h"
#include "Languages/CSyntax.h"
#include "Languages/LuaSyntax.h"

static const SyntaxLanguage CLanguage = {
    .ID = SYNTAX_LANGUAGE_C,
    .Name = "C",
    .MatchesFilename = CMatchesFilename,
    .LexerLine = CLexerLine
};

static const SyntaxLanguage LuaLanguage = {
    .ID = SYNTAX_LANGUAGE_Lua,
    .Name = "Lua",
    .MatchesFilename = LuaMatchesFilename,
    .LexerLine = LuaLexerLine
};

static const SyntaxLanguage *Languages[] = {
    &CLanguage,
    &LuaLanguage
};

static const size_t LanguageCount = sizeof(Languages) / sizeof(Languages[0]);

const SyntaxLanguage *SyntaxGetLanguage(const char *Filename) {
    if (Filename == NULL)
        return NULL;

    for (size_t i = 0; i < LanguageCount; i++) {
        if (Languages[i] -> MatchesFilename(Filename))
            return Languages[i];
    }

    return NULL;
}

SyntaxStyle SyntaxGetStyle(SyntaxTokenType Type) {
    switch (Type) {
        case SYNTAX_TOKEN_KEYWORD: return SYNTAX_STYLE_KEYWORD;
        case SYNTAX_TOKEN_NUMBER: return SYNTAX_STYLE_NUMBER;
        case SYNTAX_TOKEN_STRING: return SYNTAX_STYLE_STRING;
        case SYNTAX_TOKEN_CHARACTER: return SYNTAX_STYLE_CHARACTER;
        case SYNTAX_TOKEN_COMMENT: return SYNTAX_STYLE_COMMENT;
        case SYNTAX_TOKEN_OPERATOR: return SYNTAX_STYLE_OPERATOR;
        case SYNTAX_TOKEN_PUNCTUATION: return SYNTAX_STYLE_PUNCTUATION;
        case SYNTAX_TOKEN_TYPE: return SYNTAX_STYLE_TYPE;
        case SYNTAX_TOKEN_FUNCTION: return SYNTAX_STYLE_FUNCTION;
        case SYNTAX_TOKEN_CONSTANT: return SYNTAX_STYLE_CONSTANT;
        case SYNTAX_TOKEN_PREPROCESSOR: return SYNTAX_STYLE_PREPROCESSOR;

        case SYNTAX_TOKEN_IDENTIFIER:
        case SYNTAX_TOKEN_UNKNOWN:
        default:
            return SYNTAX_STYLE_NORMAL;
    }
}

void SyntaxLexerLine(const SyntaxLanguage *Language, const char *Line, size_t Length, SyntaxState *State, SyntaxTokenList *Tokens) {
    if (Tokens == NULL)
        return;

    Tokens -> Count = 0;

    if (Language == NULL || Language -> LexerLine == NULL)
        return;

    Language -> LexerLine(Line, Length, State, Tokens);
}
