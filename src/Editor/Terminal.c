#include <stdio.h>
#include <string.h>

#include "Terminal.h"

#ifdef _WIN32
    static DWORD OriginalMode;

    void EnableRawMode() {
        HANDLE InputHandle = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

        GetConsoleMode(InputHandle, &OriginalMode);

        DWORD Mode = OriginalMode;

        Mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        Mode |= ENABLE_PROCESSED_INPUT;

        SetConsoleMode(InputHandle, Mode);

        DWORD OutputMode;

        if (GetConsoleMode(OutputHandle, &OutputMode)) {
            OutputMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            
            SetConsoleMode(OutputHandle, OutputMode);
        }

        SetConsoleTitle("Mei - Text Editor");
    }

    void DisableRawMode() {
        HANDLE InputHandle = GetStdHandle(STD_INPUT_HANDLE);

        SetConsoleMode(InputHandle, OriginalMode);
    }

    void ClearScreen() {
        HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO CSBI;

        DWORD Count;
        DWORD CellCount;
        COORD HomeCoords = {0, 0};

        GetConsoleScreenBufferInfo(OutputHandle, &CSBI);

        CellCount = CSBI.dwSize.X * CSBI.dwSize.Y;

        FillConsoleOutputCharacter(OutputHandle, ' ', CellCount, HomeCoords, &Count);
        FillConsoleOutputAttribute(OutputHandle, CSBI.wAttributes, CellCount, HomeCoords, &Count);
        SetConsoleCursorPosition(OutputHandle, HomeCoords);
    }

    void ClearLine(int Y) {
        HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO CSBI;

        if (!GetConsoleScreenBufferInfo(OutputHandle, &CSBI))
            return;

        COORD LineStart = { 0, (SHORT) Y };
        DWORD Written;
        DWORD Width = (DWORD)(CSBI.srWindow.Right - CSBI.srWindow.Left + 1);

        FillConsoleOutputCharacter(OutputHandle, ' ', Width, LineStart, &Written);
        FillConsoleOutputAttribute(OutputHandle, CSBI.wAttributes, Width, LineStart, &Written);
        SetConsoleCursorPosition(OutputHandle, LineStart);
    }

    void SetCursorPosition(int X, int Y) {
        HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD Position = { (SHORT) X, (SHORT) Y };

        SetConsoleCursorPosition(OutputHandle, Position);
    }

    int GetTerminalRows() {
        CONSOLE_SCREEN_BUFFER_INFO CSBI;

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CSBI);

        return CSBI.srWindow.Bottom - CSBI.srWindow.Top + 1;
    }

    int GetTerminalWidth() {
        CONSOLE_SCREEN_BUFFER_INFO CSBI;

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CSBI);

        return CSBI.srWindow.Right - CSBI.srWindow.Left + 1;
    }

    int ReadKey() {
        HANDLE InputHandle = GetStdHandle(STD_INPUT_HANDLE);

        INPUT_RECORD Record;
        DWORD Count;

        while (1) {
            ReadConsoleInput(InputHandle, &Record, 1, &Count);

            if (Record.EventType == KEY_EVENT && Record.Event.KeyEvent.bKeyDown) {
                char Character = Record.Event.KeyEvent.uChar.AsciiChar;

                if (Character)
                    return Character;
                
                switch (Record.Event.KeyEvent.wVirtualKeyCode) {
                    case VK_LEFT: return -1;
                    case VK_RIGHT: return -2;
                    case VK_UP: return -3;
                    case VK_DOWN: return -4;
                }
            }
        }
    }
#else
    static struct termios OriginalMode;

    void EnableRawMode() {
        tcgetattr(STDIN_FILENO, &OriginalMode);

        struct termios Raw = OriginalMode;

        Raw.c_lflag &= ~(ECHO | ICANON | ISIG);
        Raw.c_iflag &= ~(IXON | ICRNL);
        Raw.c_cc[VMIN] = 1;
        Raw.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &Raw);

        printf("\x1b]0;Mei - Text Editor\x07");

        fflush(stdout);
    }

    void DisableRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &OriginalMode);
    }

    void ClearScreen() {
        printf("\x1b[2J\x1b[H");

        fflush(stdout);
    }

    void ClearLine(int Y) {
        SetCursorPosition(0, Y);

        printf("\x1b[2K");

        fflush(stdout);
    }

    void SetCursorPosition(int X, int Y) {
        printf("\x1b[%d;%dH", Y + 1, X + 1);

        fflush(stdout);
    }

    int GetTerminalRows(void) {
        struct winsize WindowSize = {0};

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &WindowSize) != 0) {
            return 24;
        }

        if (WindowSize.ws_row == 0)
            return 24;

        return WindowSize.ws_row;
    }

    int GetTerminalWidth(void) {
        struct winsize WindowSize = {0};

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &WindowSize) != 0) {
            return 80;
        }

        if (WindowSize.ws_col == 0)
            return 80;

        return WindowSize.ws_col;
    }

    int ReadKey() {
        char Character;

        while (read(STDIN_FILENO, &Character, 1) != 1);

        if (Character == 127)
            return 8;

        if (Character == 27) {
            char Sequence[8];

            int len = 0;

            while (len < 7) {
                if (read(STDIN_FILENO, &Sequence[len], 1) != 1)
                    break;

                if (Sequence[len] >= 'A' && Sequence[len] <= 'Z') {
                    len++;
                    
                    break;
                }

                if (Sequence[len] >= 'a' && Sequence[len] <= 'z') {
                    len++;
                    
                    break;
                }

                if (Sequence[len] == '~') {
                    len++;
                    
                    break;
                }

                len++;
            }

            Sequence[len] = '\0';

            if (len >= 1 && Sequence[0] == '[') {
                if (len == 2) {
                    switch (Sequence[1]) {
                        case 'D': return -1;
                        case 'C': return -2;
                        case 'A': return -3;
                        case 'B': return -4;
                    }
                }

                if (strcmp(&Sequence[1], "1;6Z") == 0) {
                    return -5; 
                }
            }

            return 27;
        }

        return Character;
    }
#endif

void MEI_SetTextColor(TextColor Color) {
    const char *Escape = "\x1b[0m";

    switch (Color) {
        case COLOR_CURSOR_LINE:
            Escape = "\x1b[1;37m";
            
            break;

        case COLOR_SEARCH_MATCH:
            Escape = "\x1b[44;97m";
            
            break;

        case COLOR_SYNTAX_KEYWORD:
            Escape = "\x1b[1;35m";
            
            break;

        case COLOR_SYNTAX_STRING:
            Escape = "\x1b[32m";
            
            break;

        case COLOR_SYNTAX_CHARACTER:
            Escape = "\x1b[32m";
            
            break;

        case COLOR_SYNTAX_NUMBER:
            Escape = "\x1b[33m";
            
            break;

        case COLOR_SYNTAX_COMMENT:
            Escape = "\x1b[90m";
            
            break;

        case COLOR_SYNTAX_OPERATOR:
            Escape = "\x1b[36m";
            
            break;

        case COLOR_SYNTAX_TYPE:
            Escape = "\x1b[1;34m";
            
            break;

        case COLOR_SYNTAX_FUNCTION:
            Escape = "\x1b[1;36m";
            
            break;

        case COLOR_SYNTAX_CONSTANT:
            Escape = "\x1b[33m";
            
            break;

        case COLOR_SYNTAX_PREPROCESSOR:
            Escape = "\x1b[35m";
            
            break;

        case COLOR_SYNTAX_PUNCTUATION:
            Escape = "\x1b[37m";
            
            break;

        case COLOR_NORMAL:
        default:
            Escape = "\x1b[0m";
            
            break;
    }

    printf("%s", Escape);
    fflush(stdout);
}