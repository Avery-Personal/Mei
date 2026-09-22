#ifndef SYNTAX_H
#define SYNTAX_H

    #include "SyntaxTypes.h"

    typedef enum {
        SYNTAX_LANGUAGE_NONE = 0,
        SYNTAX_LANGUAGE_C,
        SYNTAX_LANGUAGE_CPP,
        SYNTAX_LANGUAGE_CS,
        SYNTAX_LANGUAGE_Lua,
    } SyntaxLanguageID;
    
    typedef struct SyntaxLanguage {
        SyntaxLanguageID ID;
        const char *Name;

        int (*MatchesFilename)(const char *Filename);
        void (*LexerLine)(const char *Line, size_t Length, SyntaxState *State, SyntaxTokenList *Tokens);
    } SyntaxLanguage;

    const SyntaxLanguage *SyntaxGetLanguage(const char *Filename);
    SyntaxStyle SyntaxGetStyle(SyntaxTokenType Type);

    void SyntaxLexerLine(const SyntaxLanguage *Language, const char *Line, size_t Length, SyntaxState *State, SyntaxTokenList *Tokens);

#endif