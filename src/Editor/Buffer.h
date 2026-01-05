#ifndef BUFFER_H
#define BUFFER_H

#define LINE_NUMBER_GUTTER 6
#define COMMAND_BUFFER_SIZE 256

void EnterCommandMode();

void MEI_CreateFile(const char *Filename);
void MEI_OpenFile(const char *Filename);
void MEI_RemoveFile();
void MEI_SaveFile();

static void UpdateHorizontalScroll();
static void UpdateVerticalScroll();

void InitializeBuffer();
void InitializeEmptyBuffer();
void CheckBuffer();

void InsertCharacter(char Character);
void DeleteCharacter();

void InsertNewLine();

void PrintBuffer();
void DrawStatusBar(const char *Filename);

void SaveFile();
void ModifyFile();
const char *GetFileName();

void MoveCursorLeft();
void MoveCursorRight();
void MoveCursorUp();
void MoveCursorDown();

int GetCursorX();
int GetCursorY();

int GetScrollX();
int GetScrollY();

#endif
