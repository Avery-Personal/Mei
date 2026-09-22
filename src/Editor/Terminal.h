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
    COLOR_SEARCH_MATCH
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