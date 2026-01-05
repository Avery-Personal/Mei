#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Buffer.h"
#include "Terminal.h"

static char **TextBuffer;

static int Lines = 0;
static int AllocatedLines = 16;

static int CursorX = 0;
static int CursorY = 0;

static int ScrollX = 0;
static int ScrollY = 0;

static void UpdateVerticalScroll() {
    int ScreenRows = GetTerminalRows();

    if (CursorY < ScrollY)
        ScrollY = CursorY;

    if (CursorY >= ScrollY + ScreenRows)
        ScrollY = CursorY - ScreenRows + 1;
}

static void UpdateHorizontalScroll() {
    int ScreenWidth = GetTerminalWidth();
    int Margin = 2;

    if (CursorX < ScrollX)
        ScrollX = CursorX;

    if (CursorX >= ScrollX + ScreenWidth - Margin)
        ScrollX = CursorX - ScreenWidth + Margin;
}

void InitializeBuffer() {
    TextBuffer = malloc(AllocatedLines * sizeof(char*));
    TextBuffer[Lines++] = strdup("");
}

void CheckBuffer() {
    if (Lines >= AllocatedLines) {
        AllocatedLines *= 2;

        TextBuffer = realloc(TextBuffer, AllocatedLines * sizeof(char*));
    }
}

void InsertCharacter(char Character) {
    char *Line = TextBuffer[CursorY];
    int len = strlen(Line);

    char *NewLine = malloc(len + 2);

    memcpy(NewLine, Line, CursorX);

    NewLine[CursorX] = Character;

    strcpy(NewLine + CursorX + 1, Line + CursorX);
    free(TextBuffer[CursorY]);

    TextBuffer[CursorY] = NewLine;
    CursorX++;

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void DeleteCharacter() {
    char *Line = TextBuffer[CursorY];
    int len = strlen(Line);

    if (CursorX > 0) {
        char *NewLine = malloc(len);

        memcpy(NewLine, Line, CursorX - 1);
        strcpy(NewLine + (CursorX - 1), Line + CursorX);
        free(TextBuffer[CursorY]);

        TextBuffer[CursorY] = NewLine;
        CursorX--;
    } else if (CursorY > 0) {
        int PreviousLen = strlen(TextBuffer[CursorY - 1]);
        char *NewLine = malloc(PreviousLen + len + 1);

        strcpy(NewLine, TextBuffer[CursorY - 1]);
        strcat(NewLine, Line);

        free(TextBuffer[CursorY - 1]);
        free(TextBuffer[CursorY]);

        TextBuffer[CursorY - 1] = NewLine;

        for (int i = CursorY; i < Lines - 1; i++)
            TextBuffer[i] = TextBuffer[i + 1];

        Lines--;

        CursorY--;
        CursorX = PreviousLen;
    }

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void InsertNewLine() {
    char *Line = TextBuffer[CursorY];
    int len = strlen(Line);

    char *NewLine = strdup(Line + CursorX);

    Line[CursorX] = '\0';

    CheckBuffer();

    for (int i = Lines; i > CursorY + 1; i--)
        TextBuffer[i] = TextBuffer[i - 1];

    TextBuffer[CursorY + 1] = NewLine;
    Lines++;

    CursorY++;
    CursorX = 0;

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void PrintBuffer() {
    int ScreenRows = GetTerminalRows() - 1;
    int ScreenWidth = GetTerminalWidth();
    
    DWORD Written;
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    for (int Rows = 0; Rows < ScreenRows; Rows++) {
        int LineIndex = Rows + ScrollY;
        if (LineIndex >= Lines)
            break;

        SetCursorPosition(0, Rows);
        FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', ScreenWidth, (COORD){0, Rows}, &Written);

        if (LineIndex == CursorY)
            SetConsoleTextAttribute(OutputHandle, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);

        if (LineIndex < Lines) {
            char Number[LINE_NUMBER_GUTTER + 1];

            snprintf(Number, sizeof(Number), "%4d |", LineIndex + 1);
            fwrite(Number, 1, strlen(Number), stdout);
        } else {
            fwrite("    0 |", 1, LINE_NUMBER_GUTTER, stdout);
        }
        
        SetConsoleTextAttribute(OutputHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        char *Line = TextBuffer[LineIndex];
        int len = strlen(Line);

        if (ScrollX < len) {
            int Visible = len - ScrollX;
            int MaxText = ScreenWidth - LINE_NUMBER_GUTTER;

            if (Visible > MaxText)
                Visible = MaxText;

            fwrite(Line + ScrollX, 1, Visible, stdout);
        }
    }
}

void DrawStatusBar(const char *Filename) {
    int ScreenRows = GetTerminalRows();
    int ScreenWidth = GetTerminalWidth();
    
    int Modified = 0;
    char Status[256];
    
    DWORD Written;
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(OutputHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    FillConsoleOutputCharacter(OutputHandle, ' ', ScreenWidth, (COORD){0, ScreenRows - 1}, &Written);
    SetCursorPosition(0, ScreenRows - 1);

    snprintf(Status, sizeof(Status), "File: %s  |  Ln %d, Col %d  |  %s", Filename, GetCursorY() + 1, GetCursorX() + 1, Modified ? "Modified" : "Saved");
    fwrite(Status, 1, strlen(Status), stdout);
}

void MoveCursorLeft() {
    if (CursorX > 0)
        CursorX--;
    else if (CursorX == 0 && CursorY > 0) {
        MoveCursorUp();

        CursorX = strlen(TextBuffer[CursorY]);
    }

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void MoveCursorRight() {
    int len = strlen(TextBuffer[CursorY]);

    if (CursorX < len)
        CursorX++;
    else if (CursorY < Lines - 1) {
        CursorY++;
        CursorX = 0;
    }
    
    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void MoveCursorUp() {
    if (CursorY > 0) {
        CursorY--;

        int len = strlen(TextBuffer[CursorY]);
        if (CursorX > len)
            CursorX = len;
    }

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void MoveCursorDown() {
    if (CursorY < Lines - 1) {
        CursorY++;

        int len = strlen(TextBuffer[CursorY]);
        if (CursorX > len)
            CursorX = len;
    }

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

int GetCursorX() {
    return CursorX;
}

int GetCursorY() {
    return CursorY;
}

int GetScrollX() {
    return ScrollX;
}

int GetScrollY() {
    return ScrollY;
}
