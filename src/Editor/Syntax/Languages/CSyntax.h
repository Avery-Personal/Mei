#ifndef SYNTAX_C_H
#define SYNTAX_C_H

    #include <stddef.h>

    #include "../SyntaxTypes.h"

    int CMatchesFilename(const char *Filename);
    void CLexerLine(const char *Line, size_t Length, SyntaxState *State, SyntaxTokenList *Tokens);

#endif