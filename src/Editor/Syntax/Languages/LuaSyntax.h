#ifndef SYNTAX_LUA_H
#define SYNTAX_LUA_H

    #include <stddef.h>

    #include "../SyntaxTypes.h"

    int LuaMatchesFilename(const char *Filename);
    void LuaLexerLine(const char *Line, size_t Length, SyntaxState *State, SyntaxTokenList *Tokens);

#endif