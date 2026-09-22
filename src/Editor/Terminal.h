#ifndef TERMINAL_H
#define TERMINAL_H

    #ifdef _WIN32
        #include <windows.h>
    #else
        #include <termios.h>
        #include <unistd.h>
        #include <sys/ioctl.h>
    #endif

    typedef enum {
        COLOR_NORMAL,
        COLOR_CURSOR_LINE,
        COLOR_SEARCH_MATCH,

        COLOR_SYNTAX_KEYWORD,
        COLOR_SYNTAX_STRING,
        COLOR_SYNTAX_CHARACTER,
        COLOR_SYNTAX_NUMBER,
        COLOR_SYNTAX_COMMENT,
        COLOR_SYNTAX_OPERATOR,
        COLOR_SYNTAX_TYPE,
        COLOR_SYNTAX_FUNCTION,
        COLOR_SYNTAX_CONSTANT,
        COLOR_SYNTAX_PREPROCESSOR,
        COLOR_SYNTAX_PUNCTUATION
    } TextColor;

    void EnableRawMode();
    void DisableRawMode();

    void ClearScreen();
    void ClearLine(int Y);

    void SetCursorPosition(int X, int Y);
    void SetTextColor(TextColor Color);

    int GetTerminalRows();
    int GetTerminalWidth();

    int ReadKey();

#endif