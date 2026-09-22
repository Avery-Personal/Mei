#ifndef SYNTAX_TYPES_H
#define SYNTAX_TYPES_H

    #include <stddef.h>
    #include <stdint.h>

    #define SYNTAX_MAX_TOKENS_PER_LINE 256

    typedef enum {
        SYNTAX_TOKEN_UNKNOWN = 0,

        SYNTAX_TOKEN_KEYWORD,
        SYNTAX_TOKEN_IDENTIFIER,

        SYNTAX_TOKEN_NUMBER,
        SYNTAX_TOKEN_STRING,
        SYNTAX_TOKEN_CHARACTER,

        SYNTAX_TOKEN_COMMENT,

        SYNTAX_TOKEN_OPERATOR,
        SYNTAX_TOKEN_PUNCTUATION,

        SYNTAX_TOKEN_TYPE,
        SYNTAX_TOKEN_FUNCTION,
        SYNTAX_TOKEN_CONSTANT,

        SYNTAX_TOKEN_PREPROCESSOR
    } SyntaxTokenType;

    typedef enum {
        SYNTAX_STYLE_NORMAL = 0,

        SYNTAX_STYLE_KEYWORD,
        SYNTAX_STYLE_IDENTIFIER,

        SYNTAX_STYLE_NUMBER,
        SYNTAX_STYLE_STRING,
        SYNTAX_STYLE_CHARACTER,

        SYNTAX_STYLE_COMMENT,

        SYNTAX_STYLE_OPERATOR,
        SYNTAX_STYLE_PUNCTUATION,

        SYNTAX_STYLE_TYPE,
        SYNTAX_STYLE_FUNCTION,
        SYNTAX_STYLE_CONSTANT,

        SYNTAX_STYLE_PREPROCESSOR
    } SyntaxStyle;

    typedef struct {
        int InBlockComment;
    } SyntaxState;

    typedef struct {
        SyntaxTokenType Type;
        SyntaxStyle Style;

        size_t Start;
        size_t Length;
    } SyntaxToken;

    typedef struct {
        SyntaxToken Tokens[SYNTAX_MAX_TOKENS_PER_LINE];
        
        size_t Count;
    } SyntaxTokenList;

#endif