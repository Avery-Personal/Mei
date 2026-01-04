#include <stdio.h>

#include "Terminal.h"

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

char ReadKey() {
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
