#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **TextBuffer;

static int Lines = 0;
static int AllocatedLines = 16;

static int CursorX = 0;
static int CursorY = 0;

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
}

void DeleteCharacter() {
    char *Line = TextBuffer[CursorY];
    int len = strlen(Line);

    if (CursorX == 0 || len == 0)
        return;

    char *NewLine = malloc(len);

    memcpy(NewLine, Line, CursorX - 1);
    strcpy(NewLine + (CursorX - 1), Line + CursorX);
    free(TextBuffer[CursorY]);

    TextBuffer[CursorY] = NewLine;
    CursorX--;
}

void PrintBuffer() {
    for (int i = 0; i < Lines; i++) {
        printf("%s\n", TextBuffer[i]);
    }
}

void MoveCursorLeft() {
    if (CursorX > 0)
        CursorX--;
}

void MoveCursorRight() {
    int len = strlen(TextBuffer[CursorY]);

    if (CursorX < len)
        CursorX++;
}

void MoveCursorUp() {
    if (CursorY > 0) {
        CursorY--;

        int len = strlen(TextBuffer[CursorY]);
        if (CursorX > len)
            CursorX = len;
    }
}

void MoveCursorDown() {
    if (CursorY < Lines - 1) {
        CursorY++;

        int len = strlen(TextBuffer[CursorY]);
        if (CursorX > strlen(TextBuffer[CursorY]))
            CursorX = strlen(TextBuffer[CursorY]);
    }
}

int GetCursorX() {
    return CursorX;
}

int GetCursorY() {
    return CursorY;
}
