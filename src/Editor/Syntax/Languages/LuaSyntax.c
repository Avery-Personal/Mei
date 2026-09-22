#include <ctype.h>
#include <string.h>

#include "LuaSyntax.h"
#include "../Syntax.h"

static int IsIdentifierStart(char Character) {
    return isalpha((unsigned char) Character) || Character == '_';
}

static int IsIdentifierCharacter(char Character) {
    return isalnum((unsigned char) Character) || Character == '_';
}

static int IsDigitCharacter(char Character) {
    return isdigit((unsigned char) Character);
}

static int IsHexDigitCharacter(char Character) {
    return isxdigit((unsigned char) Character);
}

static int IsLuaKeyword(const char *Start, size_t Length) {
    static const char *Keywords[] = {
        "and",
        "break",
        "do",
        "else",
        "elseif",
        "end",
        "for",
        "function",
        "goto",
        "if",
        "in",
        "local",
        "not",
        "or",
        "repeat",
        "return",
        "then",
        "until",
        "while",

        NULL

    };

    for (size_t i = 0; Keywords[i] != NULL; i++) {
        size_t KeywordLength = strlen(Keywords[i]);

        if (KeywordLength == Length && strncmp(Start, Keywords[i], Length) == 0) {

            return 1;
        }
    }

    return 0;
}

static int IsLuaType(const char *Start, size_t Length) {
    static const char *Types[] = {
        "boolean",
        "number",
        "string",
        "table",
        "thread",
        "userdata",
        "function",
        "nil",

        NULL

    };

    for (size_t i = 0; Types[i] != NULL; i++) {
        size_t TypeLength = strlen(Types[i]);

        if (TypeLength == Length && strncmp(Start, Types[i], Length) == 0) {
            return 1;
        }
    }

    return 0;
}

static int IsLuaConstant(const char *Start, size_t Length) {
    static const char *Constants[] = {
        "true",
        "false",
        "nil",

        NULL

    };

    for (size_t i = 0; Constants[i] != NULL; i++) {
        size_t ConstantLength = strlen(Constants[i]);

        if (ConstantLength == Length && strncmp(Start, Constants[i], Length) == 0) {
            return 1;
        }
    }

    return 0;
}

static void AddToken(SyntaxTokenList **Tokens, SyntaxTokenType Type, size_t Start, size_t Length) {
    if (Tokens == NULL || *Tokens == NULL)
        return;

    if (Length == 0)
        return;

    if ((*Tokens) -> Count >= SYNTAX_MAX_TOKENS_PER_LINE)
        return;

    SyntaxToken *Token = &(*Tokens) -> Tokens[(*Tokens) -> Count++];

    Token -> Type = Type;
    Token -> Style = SyntaxGetStyle(Type);
    Token -> Start = Start;
    Token -> Length = Length;

}

static int IsTwoCharacterOperator(const char *Line, size_t Position, size_t Length) {
    if (Position + 1 >= Length)
        return 0;

    char A = Line[Position];
    char B = Line[Position + 1];

    return (A == '=' && B == '=') || (A == '~' && B == '=') || (A == '<' && B == '=') || (A == '>' && B == '=') || (A == '/' && B == '/') || (A == '.' && B == '.') || (A == '<' && B == '<') || (A == '>' && B == '>');
}

static int IsPunctuation(char Character) {
    return Character == '(' || Character == ')' || Character == '{' || Character == '}' || Character == '[' || Character == ']' || Character == ',' || Character == ';' || Character == ':' || Character == '.';
}

int LuaMatchesFilename(const char *Filename) {
    if (Filename == NULL)
        return 0;

    const char *Extension = strrchr(Filename, '.');

    if (Extension == NULL)
        return 0;

    return strcmp(Extension, ".lua") == 0;

}

void LuaLexerLine(const char *Line, size_t Length, SyntaxState *State, SyntaxTokenList *Tokens) {
    if (Line == NULL || Tokens == NULL)
        return;

    if (State == NULL)
        return;

    Tokens -> Count = 0;

    size_t Position = 0;

    while (Position < Length) {
        if (State -> InBlockComment) {
            size_t Start = Position;

            while (Position < Length) {

                if (Position + 1 < Length && Line[Position] == ']' && Line[Position + 1] == ']') {
                    Position += 2;

                    State -> InBlockComment = 0;

                    break;

                }

                Position++;

            }

            AddToken(&Tokens, SYNTAX_TOKEN_COMMENT, Start, Position - Start);

            continue;

        }

        char Character = Line[Position];

        if (isspace((unsigned char) Character)) {

            Position++;

            continue;

        }
        
        if (Position + 1 < Length && Line[Position] == '-' && Line[Position + 1] == '-') {
            if (Position + 3 < Length && Line[Position + 2] == '[' && Line[Position + 3] == '[') {
                size_t Start = Position;

                Position += 4;

                while (Position < Length) {
                    if (Position + 1 < Length &&  Line[Position] == ']' &&  Line[Position + 1] == ']') {
                        Position += 2;

                        State -> InBlockComment = 0;

                        break;

                    }

                    Position++;
                }

                if (Position >= Length && !(Length >= 2 && Line[Length - 2] == ']' && Line[Length - 1] == ']')) {
                    State -> InBlockComment = 1;
                }

                AddToken(&Tokens, SYNTAX_TOKEN_COMMENT, Start, Position - Start);

                continue;
            }

            AddToken(&Tokens, SYNTAX_TOKEN_COMMENT, Position, Length - Position);

            return;
        }

        if (Position + 1 < Length && Line[Position] == '[' && Line[Position + 1] == '[') {
            size_t Start = Position;

            Position += 2;

            while (Position < Length) {
                if (Position + 1 < Length && Line[Position] == ']' && Line[Position + 1] == ']') {

                    Position += 2;

                    break;

                }

                Position++;
            }

            AddToken(&Tokens, SYNTAX_TOKEN_STRING, Start, Position - Start);

            continue;
        }

        if (Character == '"') {
            size_t Start = Position;

            Position++;

            while (Position < Length) {
                if (Line[Position] == '\\') {
                    Position += 2;

                    continue;

                }

                if (Line[Position] == '"') {
                    Position++;

                    break;
                }

                Position++;
            }

            AddToken(&Tokens, SYNTAX_TOKEN_STRING, Start, Position - Start);

            continue;
        }

        if (Character == '\'') {
            size_t Start = Position;

            Position++;

            while (Position < Length) {
                if (Line[Position] == '\\') {
                    Position += 2;

                    continue;
                }

                if (Line[Position] == '\'') {
                    Position++;

                    break;
                }

                Position++;
            }

            AddToken(&Tokens, SYNTAX_TOKEN_STRING, Start, Position - Start);

            continue;
        }

        if (IsDigitCharacter(Character)) {
            size_t Start = Position;

            if (Character == '0' && Position + 1 < Length && (Line[Position + 1] == 'x' || Line[Position + 1] == 'X')) {
                Position += 2;

                while (Position < Length && IsHexDigitCharacter(Line[Position])) {
                    Position++;
                }

                if (Position < Length && Line[Position] == '.') {
                    Position++;

                    while (Position < Length && IsHexDigitCharacter(Line[Position])) {

                        Position++;
                    }
                }

                if (Position < Length && (Line[Position] == 'p' || Line[Position] == 'P')) {
                    Position++;

                    if (Position < Length && (Line[Position] == '+' || Line[Position] == '-')) {
                        Position++;
                    }

                    while (Position < Length && IsDigitCharacter(Line[Position])) {
                        Position++;
                    }
                }
            } else {
                while (Position < Length && IsDigitCharacter(Line[Position])) {

                    Position++;
                }

                if (Position < Length && Line[Position] == '.') {
                    Position++;

                    while (Position < Length && IsDigitCharacter(Line[Position])) {
                        Position++;
                    }
                }

                if (Position < Length && (Line[Position] == 'e' || Line[Position] == 'E')) {
                    Position++;

                    if (Position < Length && (Line[Position] == '+' || Line[Position] == '-')) {
                        Position++;
                    }

                    while (Position < Length && IsDigitCharacter(Line[Position])) {
                        Position++;
                    }
                }
            }

            AddToken(&Tokens, SYNTAX_TOKEN_NUMBER, Start, Position - Start);

            continue;
        }

        if (IsIdentifierStart(Character)) {
            size_t Start = Position;

            while (Position < Length &&IsIdentifierCharacter(Line[Position])) {
                Position++;
            }

            size_t IdentifierLength = Position - Start;

            if (IsLuaKeyword(Line + Start, IdentifierLength)) {
                AddToken(&Tokens, SYNTAX_TOKEN_KEYWORD, Start, IdentifierLength);

                continue;
            }

            if (IsLuaConstant(Line + Start, IdentifierLength)) {
                AddToken(&Tokens, SYNTAX_TOKEN_CONSTANT, Start, IdentifierLength);

                continue;
            }

            if (IsLuaType(Line + Start, IdentifierLength)) {
                AddToken(&Tokens, SYNTAX_TOKEN_TYPE, Start, IdentifierLength);

                continue;
            }

            size_t LookAhead = Position;

            while (LookAhead < Length && isspace((unsigned char) Line[LookAhead])) {
                LookAhead++;
            }

            if (LookAhead < Length && Line[LookAhead] == '(') {
                AddToken(&Tokens, SYNTAX_TOKEN_FUNCTION, Start, IdentifierLength);

                continue;

            }

            AddToken(&Tokens, SYNTAX_TOKEN_IDENTIFIER, Start, IdentifierLength);

            continue;

        }

        if (IsTwoCharacterOperator(Line, Position, Length)) {

            AddToken(&Tokens, SYNTAX_TOKEN_OPERATOR, Position, 2);

            Position += 2;

            continue;

        }

        if (strchr("+-*/%^#=<>~&|", Character) != NULL) {

            AddToken(&Tokens, SYNTAX_TOKEN_OPERATOR, Position, 1);

            Position++;

            continue;

        }

        if (Position + 2 < Length && Line[Position] == '.' && Line[Position + 1] == '.' && Line[Position + 2] == '.') {
            AddToken(&Tokens, SYNTAX_TOKEN_OPERATOR, Position, 3);

            Position += 3;

            continue;

        }

        if (IsPunctuation(Character)) {
            AddToken(&Tokens, SYNTAX_TOKEN_PUNCTUATION, Position, 1);

            Position++;

            continue;

        }

        Position++;

    }
}
