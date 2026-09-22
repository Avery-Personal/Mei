#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "Buffer.h"
#include "Terminal.h"
#include "Path.h"
#include "Syntax/Syntax.h"

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

static char FullFilePath[512] = "";

static char CurrentFile[256] = "Untitled.txt";
static char *FileContents;
int FileModified = 0;

static int CommandErrorActive = 0;

static char *MEIStrdup(const char *Source) {
    if (Source == NULL)
        return NULL;

    size_t Length = strlen(Source);
    char *Copy = malloc(Length + 1);

    if (Copy == NULL)
        return NULL;

    memcpy(Copy, Source, Length + 1);

    return Copy;
}

static int SyntaxStyleToColor(SyntaxStyle Style) {
    switch (Style) {
        case SYNTAX_STYLE_KEYWORD:
            return COLOR_SYNTAX_KEYWORD;

        case SYNTAX_STYLE_STRING:
            return COLOR_SYNTAX_STRING;

        case SYNTAX_STYLE_CHARACTER:
            return COLOR_SYNTAX_CHARACTER;

        case SYNTAX_STYLE_NUMBER:
            return COLOR_SYNTAX_NUMBER;

        case SYNTAX_STYLE_COMMENT:
            return COLOR_SYNTAX_COMMENT;

        case SYNTAX_STYLE_OPERATOR:
            return COLOR_SYNTAX_OPERATOR;

        case SYNTAX_STYLE_TYPE:
            return COLOR_SYNTAX_TYPE;

        case SYNTAX_STYLE_FUNCTION:
            return COLOR_SYNTAX_FUNCTION;

        case SYNTAX_STYLE_CONSTANT:
            return COLOR_SYNTAX_CONSTANT;

        case SYNTAX_STYLE_PREPROCESSOR:
            return COLOR_SYNTAX_PREPROCESSOR;

        case SYNTAX_STYLE_PUNCTUATION:
            return COLOR_SYNTAX_PUNCTUATION;

        case SYNTAX_STYLE_IDENTIFIER:
        case SYNTAX_STYLE_NORMAL:
        default:
            return COLOR_NORMAL;
    }
}

static SyntaxStyle GetSyntaxStyleAt(const SyntaxTokenList *Tokens, size_t Position) {
    for (size_t i = 0; i < Tokens -> Count; i++) {
        const SyntaxToken *Token = &Tokens -> Tokens[i];

        if (Position >= Token -> Start && Position < Token -> Start + Token -> Length) {
            return Token -> Style;
        }
    }

    return SYNTAX_STYLE_NORMAL;
}

static void RenderSyntaxLine(const char *Line, int Length, int StartColumn, int VisibleLength, const SyntaxTokenList *Tokens) {
    int EndColumn = StartColumn + VisibleLength;

    SyntaxStyle CurrentStyle = SYNTAX_STYLE_NORMAL;

    for (int Position = StartColumn; Position < EndColumn && Position < Length; Position++) {
        if (SearchActive && SearchLen > 0 && Position + SearchLen <= Length && strncmp(&Line[Position], SearchQuery, SearchLen) == 0) {
            if (CurrentStyle != SYNTAX_STYLE_NORMAL) {
                MEI_SetTextColor(COLOR_NORMAL);
                
                CurrentStyle = SYNTAX_STYLE_NORMAL;
            }

            MEI_SetTextColor(COLOR_SEARCH_MATCH);

            int MatchLength = SearchLen;

            if (Position + MatchLength > EndColumn)
                MatchLength = EndColumn - Position;

            fwrite(&Line[Position], 1, MatchLength, stdout);

            Position += MatchLength - 1;

            MEI_SetTextColor(COLOR_NORMAL);

            CurrentStyle = SYNTAX_STYLE_NORMAL;

            continue;
        }

        SyntaxStyle Style = GetSyntaxStyleAt(Tokens, Position);
        if (Style != CurrentStyle) {
            MEI_SetTextColor( SyntaxStyleToColor(Style));

            CurrentStyle = Style;
        }

        fwrite(&Line[Position], 1, 1, stdout);
    }

    MEI_SetTextColor(COLOR_NORMAL);
}

static int CommandEquals(const char *A, const char *B) {
    while (*A != '\0' && *B != '\0') {
        if (tolower((unsigned char) *A) != tolower((unsigned char) *B)) {
            return 0;
        }

        A++;
        B++;
    }

    return *A == '\0' && *B == '\0';
}

void EnterCommandMode(void) {
    int ScreenRows = GetTerminalRows();

    if (ScreenRows <= 0)
        ScreenRows = 1;

    char Command[COMMAND_BUFFER_SIZE] = {0};
    int CommandLength = 0;

    ClearLine(ScreenRows - 1);

    printf(":");
    fflush(stdout);

    while (1) {
        int Character = ReadKey();

        if (Character == ':' && CommandLength == 0) {
            ClearLine(ScreenRows - 1);
            
            DrawStatusBar(CurrentFile);
            
            return;
        }

        if (Character == 27) {
            ClearLine(ScreenRows - 1);

            DrawStatusBar(CurrentFile);

            return;
        }

        if (Character == '\r' || Character == '\n') {
            Command[CommandLength] = '\0';

            break;
        }

        if (Character == 8) {
            if (CommandLength > 0) {
                CommandLength--;

                SetCursorPosition(1 + CommandLength, ScreenRows - 1);

                putchar(' ');

                SetCursorPosition(1 + CommandLength, ScreenRows - 1);

                fflush(stdout);
            }

            continue;
        }

        if (Character >= 32 && Character <= 126) {
            if (CommandLength < COMMAND_BUFFER_SIZE - 1) {
                Command[CommandLength++] = (char) Character;

                putchar(Character);
                fflush(stdout);
            }
        }
    }

    if (CommandLength == 0) {
        ClearLine(ScreenRows - 1);

        DrawStatusBar(CurrentFile);

        return;
    }

    char *CommandName = Command;
    char *Argument = Command;

    while (*Argument != '\0' && !isspace((unsigned char)*Argument)) {
        Argument++;
    }

    if (*Argument != '\0') {
        *Argument = '\0';
        Argument++;

        while (*Argument != '\0' && isspace((unsigned char)*Argument)) {
            Argument++;
        }
    }

    if (CommandEquals(CommandName, "create")) {
        if (*Argument == '\0') {
            ShowCommandError("Command Line Error - create requires a filename");

            return;
        }

        if (!MEI_CreateFile(Argument)) {
            ShowCommandError("Command Line Error - Couldn't create file");

            return;
        }

    } else if (CommandEquals(CommandName, "open")) {
        if (*Argument == '\0') {
            ShowCommandError("Command Line Error - open requires a filename");

            return;
        }

        if (!MEI_OpenFile(Argument)) {
            ShowCommandError("Command Line Error - Couldn't open file");

            return;
        }

    } else if (CommandEquals(CommandName, "remove")) {
        if (!MEI_RemoveFile()) {
            ShowCommandError("Command Line Error - Couldn't remove file");

            return;
        }

    } else if (CommandEquals(CommandName, "save")) {
        if (*Argument != '\0') {
            ShowCommandError("Command Line Error - save takes no arguments");

            return;
        }

        if (!MEI_SaveFile()) {
            ShowCommandError("Command Line Error - Couldn't save file");

            return;
        }

    } else if (CommandEquals(CommandName, "quit")) {
        if (*Argument != '\0') {
            ShowCommandError("Command Line Error - Quit takes no arguments");

            return;
        }

        DisableRawMode();

        exit(0);
    } else {
        ShowCommandError("Command Line Error - Invalid Command");

        return;
    }

    ClearLine(ScreenRows - 1);

    DrawStatusBar(CurrentFile);
}

void EnterGlobalCommandMode(void) {
    int ScreenRows = GetTerminalRows();

    char Command[COMMAND_BUFFER_SIZE] = {0};
    int CommandLength = 0;

    ClearLine(ScreenRows - 1);

    printf("!");
    fflush(stdout);

    while (1) {
        int Character = ReadKey();

        if (Character == '!' && CommandLength == 0) {
            ClearLine(ScreenRows - 1);
            
            DrawStatusBar(CurrentFile);
            
            return;
        }

        if (Character == '\r') {
            Command[CommandLength] = '\0';

            break;
        }

        if (Character == 27) {
            ClearLine(ScreenRows - 1);

            DrawStatusBar(CurrentFile);

            return;
        }

        if (Character == 8) {
            if (CommandLength > 0) {
                CommandLength--;

                SetCursorPosition(1 + CommandLength, ScreenRows - 1);

                putchar(' ');

                SetCursorPosition(1 + CommandLength, ScreenRows - 1);

                fflush(stdout);
            }

            continue;
        }

        if (Character >= 32 && Character <= 126) {
            if (CommandLength < COMMAND_BUFFER_SIZE - 1) {
                Command[CommandLength++] = (char) Character;

                putchar(Character);
                fflush(stdout);
            }
        }
    }

    if (CommandLength == 0) {
        ClearLine(ScreenRows - 1);

        DrawStatusBar(CurrentFile);

        return;
    }
    
    DisableRawMode();
    
    printf("\n");
    fflush(stdout);

    int Result = system(Command);

    EnableRawMode();

    printf("\n");
    printf("[Process exited with status %d. Press any key to return to Mei]",  Result);

    fflush(stdout);

    ReadKey();

    ClearScreen();
    ClearLine(ScreenRows - 1);

    DrawStatusBar(CurrentFile);
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

    Lines = Previous -> LinesCount;
    TextBuffer = malloc(Lines * sizeof(char*));

    for (int i = 0; i < Lines; i++)
        TextBuffer[i] = strdup(Previous -> Lines[i]);

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

    ClearLine(ScreenRows - 1);

    printf("/%s", SearchQuery);

    fflush(stdout);
}

static int SetCurrentFile(const char *Filename) {
    if (Filename == NULL || Filename[0] == '\0')
        return 0;

    size_t Length = strlen(Filename);

    if (Length >= sizeof(CurrentFile))
        return 0;

    memcpy(CurrentFile, Filename, Length + 1);

    return 1;
}

int MEI_CreateFile(const char *Filename) {
    if (Filename == NULL || Filename[0] == '\0')
        return 0;

    FILE *File = fopen(Filename, "w");
    if (!File)
        return 0;

    fclose(File);

    if (!SetCurrentFile(Filename))
        return 0;

    InitializeEmptyBuffer();

    return 1;
}
  
int MEI_OpenFile(const char *Filename) {
    if (Filename == NULL || Filename[0] == '\0')
        return 0;

    if (strlen(Filename) >= sizeof(CurrentFile))
        return 0;

    FILE *File = fopen(Filename, "r");
    if (!File)
        return 0;

    int NewAllocatedLines = 16;
    int NewLines = 0;

    char **NewBuffer = malloc(NewAllocatedLines * sizeof(char *));

    if (!NewBuffer) {
        fclose(File);
        
        return 0;
    }

    char Line[1024];

    while (fgets(Line, sizeof(Line), File)) {
        Line[strcspn(Line, "\r\n")] = '\0';

        if (NewLines >= NewAllocatedLines) {
            int NewCapacity = NewAllocatedLines * 2;

            char **ResizedBuffer = realloc(NewBuffer, NewCapacity * sizeof(char *));

            if (!ResizedBuffer) {
                for (int i = 0; i < NewLines; i++)
                    free(NewBuffer[i]);

                free(NewBuffer);
                
                fclose(File);

                return 0;
            }

            NewBuffer = ResizedBuffer;
            NewAllocatedLines = NewCapacity;
        }

        NewBuffer[NewLines] = MEIStrdup(Line);
        if (!NewBuffer[NewLines]) {
            for (int i = 0; i < NewLines; i++)
                free(NewBuffer[i]);

            free(NewBuffer);
            fclose(File);

            return 0;
        }

        NewLines++;
    }

    fclose(File);

    if (NewLines == 0) {
        NewBuffer[NewLines] = MEIStrdup("");

        if (!NewBuffer[NewLines]) {
            free(NewBuffer);
            
            return 0;
        }

        NewLines++;
    }

    if (TextBuffer) {
        for (int i = 0; i < Lines; i++)
            free(TextBuffer[i]);

        free(TextBuffer);
    }

    TextBuffer = NewBuffer;
    Lines = NewLines;
    AllocatedLines = NewAllocatedLines;

    CursorX = 0;
    CursorY = 0;
    ScrollX = 0;
    ScrollY = 0;

    FileModified = 0;

    SetCurrentFile(Filename);

    return 1;
}

int MEI_RemoveFile(void) {
    if (CurrentFile[0] == '\0')
        return 0;

    return remove(CurrentFile) == 0;
}

int MEI_SaveFile(void) {
    FILE *File = fopen(CurrentFile, "w");
    if (!File)
        return 0;

    for (int i = 0; i < Lines; i++) {
        if (fprintf(File, "%s\n", TextBuffer[i]) < 0) {
            fclose(File);
            
            return 0;
        }
    }

    if (fclose(File) != 0)
        return 0;

    FileModified = 0;

    return 1;
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

void InsertTab() {
    PushUndo();

    char *Line = TextBuffer[CursorY];
    int len = strlen(Line);

    char *NewLine = malloc(len + 3);

    memcpy(NewLine, Line, CursorX);

    NewLine[CursorX] = ' ';
    NewLine[CursorX + 1] = ' ';

    strcpy(NewLine + CursorX + 2, Line + CursorX);

    free(TextBuffer[CursorY]);
    
    TextBuffer[CursorY] = NewLine;

    CursorX += 2;

    FileModified = 1;

    UpdateHorizontalScroll();
    UpdateVerticalScroll();
}

void PrintBuffer() {
    int ScreenRows = GetTerminalRows() - 1;
    int ScreenWidth = GetTerminalWidth();

    const SyntaxLanguage *Language = SyntaxGetLanguage(CurrentFile);

    SyntaxState State = {.InBlockComment = 0};

    for (int LineIndex = 0; LineIndex < Lines; LineIndex++) {
        char *Line = TextBuffer[LineIndex];

        SyntaxTokenList Tokens = {0};

        SyntaxLexerLine(Language, Line, strlen(Line), &State, &Tokens);

        if (LineIndex < ScrollY)
            continue;

        int ScreenRow = LineIndex - ScrollY;

        if (ScreenRow >= ScreenRows)
            break;

        ClearLine(ScreenRow);

        if (LineIndex == CursorY)
            MEI_SetTextColor(COLOR_CURSOR_LINE);

        char Number[LINE_NUMBER_GUTTER + 1];

        snprintf(Number, sizeof(Number), "%4d |", LineIndex + 1);

        fwrite(Number, 1, strlen(Number), stdout);

        MEI_SetTextColor(COLOR_NORMAL);

        int Length = (int) strlen(Line);
        int MaxText = ScreenWidth - LINE_NUMBER_GUTTER;

        if (ScrollX < Length) {
            int Visible = Length - ScrollX;

            if (Visible > MaxText)
                Visible = MaxText;

            RenderSyntaxLine(Line, Length, ScrollX, Visible, &Tokens);
        }

        MEI_SetTextColor(COLOR_NORMAL);
    }
}

void DrawStatusBar(const char *Filename) {
    int ScreenRows = GetTerminalRows();

    char FilePath[1024];
    char Status[256];

    MEI_SetTextColor(COLOR_NORMAL);
    ClearLine(ScreenRows - 1);

    if (!GetFullPath(Filename, FilePath, sizeof(FilePath))) {
        snprintf(FilePath, sizeof(FilePath), "%s", Filename);
    }

    snprintf(Status, sizeof(Status), "File: %s  |  Ln %d, Col %d  |  %s  |  %s", CurrentFile, GetCursorY() + 1, GetCursorX() + 1, FileModified ? "Modified" : "Saved", FilePath);
    fwrite(Status, 1, strlen(Status), stdout);
}

void ShowCommandError(const char *Message) {
    int ScreenRows = GetTerminalRows();

    ClearLine(ScreenRows - 1);
    MEI_SetTextColor(COLOR_NORMAL);

    printf("%s", Message);
    fflush(stdout);

    CommandErrorActive = 1;
}

void ClearCommandError(void) {
    CommandErrorActive = 0;
}

int IsCommandErrorActive(void) {
    return CommandErrorActive;
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