#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Buffer.h"
#include "Terminal.h"

typedef struct {
    char **Lines;
    int LinesCount;
    int CursorX, CursorY;
} EditorState;

static EditorState UndoStack[UNDO_STACK_SIZE];
static int UndoTop = -1;

static EditorState RedoStack[UNDO_STACK_SIZE];
static int RedoTop = -1;

static int SearchActive = 0;
static char SearchQuery[256] = {0};
static int SearchLen = 0;

static char Clipboard[1024];
static char **TextBuffer;

static int Lines = 0;
static int AllocatedLines = 16;

static int CursorX = 0;
static int CursorY = 0;

static int ScrollX = 0;
static int ScrollY = 0;

static char CurrentFile[256] = "Untitled.txt";
static char *FileContents;
int FileModified = 0;

void EnterCommandMode() {
    int ScreenRows = GetTerminalRows();
    int ScreenWidth = GetTerminalWidth();

    char Command[COMMAND_BUFFER_SIZE] = {0};
    int CommandLength = 0;
    
    DWORD Written;

    SetCursorPosition(0, ScreenRows - 1);
    FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', ScreenWidth, (COORD){0, ScreenRows - 1}, &Written);

    printf(":");
    fflush(stdout);

    while (1) {
        int Character = ReadKey();

        if (Character == '\r') {
            Command[CommandLength] = '\0';

            break;
        } else if (Character == 8) {
            if (CommandLength > 0) {
                CommandLength--;

                SetCursorPosition(1 + CommandLength, ScreenRows - 1);
                putchar(' ');
                SetCursorPosition(1 + CommandLength, ScreenRows - 1);
                
                fflush(stdout);
            }
        } else if (Character >= 32 && Character <= 126) {
            if (CommandLength < COMMAND_BUFFER_SIZE - 1) {
                Command[CommandLength++] = (char) Character;

                putchar(Character);
                fflush(stdout);
            }
        }
    }

    if (strncmp(Command, "create ", 7) == 0 || strncmp(Command, "Create ", 7) == 0) {
        MEI_CreateFile(Command + 7);
        MEI_OpenFile(CurrentFile);
    } else if (strncmp(Command, "open ", 5) == 0 || strncmp(Command, "Open ", 5) == 0) {
        MEI_OpenFile(Command + 5);
    } else if (strncmp(Command, "remove ", 8) == 0 || strncmp(Command, "Remove ", 8) == 0) {
        MEI_RemoveFile();
    } else if (strcmp(Command, "save") == 0 || strcmp(Command, "Save") == 0) {
        MEI_SaveFile();
    } else if (strcmp(Command, "quit") == 0 || strcmp(Command, "Quit") == 0) {
        exit(0);
    }

    FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', ScreenWidth, (COORD){0, ScreenRows - 1}, &Written);
}

void PushUndo() {
    if (UndoTop >= UNDO_STACK_SIZE - 1) {
        for (int i = 0; i < UndoStack[0].LinesCount; i++)
            free(UndoStack[0].Lines[i]);

        free(UndoStack[0].Lines);
        memmove(&UndoStack[0], &UndoStack[1], sizeof(EditorState) * (UNDO_STACK_SIZE - 1));

        UndoTop = UNDO_STACK_SIZE - 2;
    }

    UndoTop++;

    UndoStack[UndoTop].LinesCount = Lines;
    UndoStack[UndoTop].CursorX = CursorX;
    UndoStack[UndoTop].CursorY = CursorY;
    
    UndoStack[UndoTop].Lines = malloc(Lines * sizeof(char*));

    for (int i = 0; i < Lines; i++)
        UndoStack[UndoTop].Lines[i] = strdup(TextBuffer[i]);

    RedoTop = -1;
}

void Undo() {
    if (UndoTop < 0)
        return;

    RedoTop++;

    RedoStack[RedoTop].LinesCount = Lines;
    RedoStack[RedoTop].CursorX = CursorX;
    RedoStack[RedoTop].CursorY = CursorY;

    RedoStack[RedoTop].Lines = malloc(Lines * sizeof(char*));

    for (int i = 0; i < Lines; i++)
        RedoStack[RedoTop].Lines[i] = strdup(TextBuffer[i]);

    EditorState *Previous = &UndoStack[UndoTop];

    for (int i = 0; i < Lines; i++)
        free(TextBuffer[i]);

    free(TextBuffer);

    Lines = Previous->LinesCount;
    TextBuffer = malloc(Lines * sizeof(char*));

    for (int i = 0; i < Lines; i++)
        TextBuffer[i] = strdup(Previous->Lines[i]);

    CursorX = Previous -> CursorX;
    CursorY = Previous -> CursorY;

    UndoTop--;
}

void Redo() {
    if (RedoTop < 0)
        return;

    if (UndoTop < UNDO_STACK_SIZE - 1) {
        UndoTop++;

        UndoStack[UndoTop].LinesCount = Lines;
        UndoStack[UndoTop].CursorX = CursorX;
        UndoStack[UndoTop].CursorY = CursorY;

        UndoStack[UndoTop].Lines = malloc(Lines * sizeof(char*));

        for (int i = 0; i < Lines; i++)
            UndoStack[UndoTop].Lines[i] = strdup(TextBuffer[i]);
    }

    EditorState *RedoState = &RedoStack[RedoTop];

    for (int i = 0; i < Lines; i++)
        free(TextBuffer[i]);

    free(TextBuffer);

    Lines = RedoState -> LinesCount;
    TextBuffer = malloc(Lines * sizeof(char*));

    for (int i = 0; i < Lines; i++)
        TextBuffer[i] = strdup(RedoState -> Lines[i]);

    CursorX = RedoState -> CursorX;
    CursorY = RedoState -> CursorY;

    RedoTop--;
}

void CopySelection() {
    strncpy(Clipboard, TextBuffer[CursorY], sizeof(Clipboard));
}

void CutSelection() {
    CopySelection();
    DeleteCharacter();
}

void PasteClipboard() {
    for (int i = 0; Clipboard[i]; i++)
        InsertCharacter(Clipboard[i]);
}

void HandleSearchInput(int Character) {
    int ScreenRows = GetTerminalRows();
    int ScreenWidth = GetTerminalWidth();

    DWORD Written;

    if (Character == 27 || Character == '\r') {
        SearchActive = 0;

        return;
    }

    if (Character == 8 && SearchLen > 0) {
        SearchLen--;
        SearchQuery[SearchLen] = '\0';
    } else if (Character >= 32 && Character <= 126) {
        if (SearchLen < sizeof(SearchQuery) - 1) {
            SearchQuery[SearchLen++] = (char)Character;
            SearchQuery[SearchLen] = '\0';
        }
    }

    for (int Y = 0; Y < Lines; Y++) {
        char *Position = strstr(TextBuffer[Y], SearchQuery);

        if (Position) {
            CursorY = Y;
            CursorX = Position - TextBuffer[Y];

            UpdateHorizontalScroll();
            UpdateVerticalScroll();

            break;
        }
    }

    SetCursorPosition(0, ScreenRows - 1);
    FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', ScreenWidth, (COORD){0, ScreenRows - 1}, &Written);

    printf("/%s", SearchQuery);

    fflush(stdout);
}

void MEI_CreateFile(const char *Filename) {
    FILE *File = fopen(Filename, "w");
    if (!File) {
        fprintf(stderr, "Couldn't create file.");
        strncpy(CurrentFile, Filename, sizeof(CurrentFile));

        return;
    }

    fclose(File);

    strncpy(CurrentFile, Filename, sizeof(CurrentFile));
    InitializeEmptyBuffer();
}
  
void MEI_OpenFile(const char *Filename) {
    FILE *File = fopen(Filename, "r");
    if (!File) {
        InitializeEmptyBuffer();
        strncpy(CurrentFile, Filename, sizeof(CurrentFile));
        
        return;
    }

    if (TextBuffer) {
        for (int i = 0; i < Lines; i++)
            free(TextBuffer[i]);

        free(TextBuffer);
    }
    
    AllocatedLines = 16;
    Lines = 0;
    TextBuffer = malloc(AllocatedLines * sizeof(char*));

    char Line[1024];

    while (fgets(Line, sizeof(Line), File)) {
        Line[strcspn(Line, "\r\n")] = 0;

        CheckBuffer();
        
        TextBuffer[Lines++] = strdup(Line);
    }

    fclose(File);

    CursorX = CursorY = ScrollX = ScrollY = 0;
    FileModified = 0;

    strncpy(CurrentFile, Filename, sizeof(CurrentFile));

    if (Lines == 0)
        TextBuffer[Lines++] = strdup("");
}

void MEI_RemoveFile() {
    FILE *File = fopen(CurrentFile, "r");
    if (!File) {
        fprintf(stderr, "Couldn't find file.");
        strncpy(CurrentFile, CurrentFile, sizeof(CurrentFile));

        return;
    }

    fclose(File);
    
    int Removed = remove(CurrentFile);
    if (Removed != 0)
        fprintf(stderr, "Unable to remove file.");
}

void MEI_SaveFile() {
    FILE *File = fopen(CurrentFile, "w");
    if (!File)
        return;

    for (int i = 0; i < Lines; i++)
        fprintf(File, "%s\n", TextBuffer[i]);

    fclose(File);

    FileModified = 0;
}

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

void InitializeEmptyBuffer() {
    if (TextBuffer) {
        for (int i = 0; i < Lines; i++)
            free(TextBuffer[i]);

        free(TextBuffer);
    }

    AllocatedLines = 16;
    Lines = 0;

    TextBuffer = malloc(AllocatedLines * sizeof(char*));
    TextBuffer[Lines++] = strdup("");

    CursorX = CursorY = ScrollX = ScrollY = 0;
    FileModified = 0;
}

void CheckBuffer() {
    if (Lines >= AllocatedLines) {
        AllocatedLines *= 2;

        TextBuffer = realloc(TextBuffer, AllocatedLines * sizeof(char*));
    }
}

void InsertCharacter(char Character) {
    PushUndo();

    char *Line = TextBuffer[CursorY];
    int len = strlen(Line);

    char *NewLine = malloc(len + 2);

    memcpy(NewLine, Line, CursorX);

    NewLine[CursorX] = Character;

    strcpy(NewLine + CursorX + 1, Line + CursorX);
    free(TextBuffer[CursorY]);

    TextBuffer[CursorY] = NewLine;
    CursorX++;
    
    FileModified = 1;

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void DeleteCharacter() {
    if (Lines == 0)
        return;

    PushUndo();
    
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
    
    FileModified = 1;

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void InsertNewLine() {
    PushUndo();
    
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

    FileModified = 1;

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
            
            int i = ScrollX;

            if (Visible > MaxText)
                Visible = MaxText;

            while (i < len && i < ScrollX + Visible) {
                if (SearchActive && SearchLen > 0 && strncmp(&Line[i], SearchQuery, SearchLen) == 0) {
                    SetConsoleTextAttribute(OutputHandle, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

                    fwrite(&Line[i], 1, SearchLen, stdout);

                    SetConsoleTextAttribute(OutputHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

                    i += SearchLen;
                } else {
                    fwrite(&Line[i], 1, 1, stdout);

                    i++;
                }
            }
        }
    }
}

void DrawStatusBar(const char *Filename) {
    int ScreenRows = GetTerminalRows();
    int ScreenWidth = GetTerminalWidth();
    
    char Status[256];
    
    DWORD Written;
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(OutputHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    FillConsoleOutputCharacter(OutputHandle, ' ', ScreenWidth, (COORD){0, ScreenRows - 1}, &Written);
    SetCursorPosition(0, ScreenRows - 1);

    snprintf(Status, sizeof(Status), "File: %s  |  Ln %d, Col %d  |  %s", CurrentFile, GetCursorY() + 1, GetCursorX() + 1, FileModified ? "Modified" : "Saved");
    fwrite(Status, 1, strlen(Status), stdout);
}

void SaveFile() {
    FileModified = 0;
}

void ModifyFile() {
    FileModified = 1;
}

const char *GetFileName() {
    return CurrentFile;
}

void SetActiveSearch(int Active) {
    SearchActive = Active;
}

int IsSearchActive() {
    return SearchActive;
}

void ResetSearchQuery() {
    SearchQuery[0] = '\0';
}

void ResetSearchLen() {
    SearchLen = 0;
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
