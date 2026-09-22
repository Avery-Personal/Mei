#include <stdio.h>

#include "Terminal.h"

#ifdef _WIN32
    static DWORD OriginalMode;

    void EnableRawMode() {
        HANDLE InputHandle = GetStdHandle(STD_INPUT_HANDLE);

        GetConsoleMode(InputHandle, &OriginalMode);

        DWORD Mode = OriginalMode;
        Mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
        Mode |= ENABLE_PROCESSED_INPUT;

        SetConsoleMode(InputHandle, Mode);
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

    void SetTextColor(TextColor Color) {
        switch (Color) {
            case COLOR_CURSOR_LINE:
                printf("\x1b[1;37m");

                break;

            case COLOR_SEARCH_MATCH:
                printf("\x1b[44;37m");

                break;

            default:
                printf("\x1b[0m");

                break;
        }

        fflush(stdout);
    }

    int GetTerminalRows() {
        struct winsize WindowSize;

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &WindowSize);

        return WindowSize.ws_row;
    }

    int GetTerminalWidth() {
        struct winsize WindowSize;

        ioctl(STDOUT_FILENO, TIOCGWINSZ, &WindowSize);

        return WindowSize.ws_col;
    }

    int ReadKey() {
        char Character;

        while (read(STDIN_FILENO, &Character, 1) != 1);

        if (Character == 27) {
            char Sequence[2];

            if (read(STDIN_FILENO, &Sequence[0], 1) != 1)
                return 27;

            if (read(STDIN_FILENO, &Sequence[1], 1) != 1)
                return 27;

            if (Sequence[0] == '[') {
                switch (Sequence[1]) {
                    case 'D': return -1;
                    case 'C': return -2;
                    case 'A': return -3;
                    case 'B': return -4;
                }
            }

            return 27;
        }

        return Character;
    }
#endif