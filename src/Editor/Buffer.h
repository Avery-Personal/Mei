#ifndef BUFFER_H
#define BUFFER_H

static void UpdateHorizontalScroll();
static void UpdateVerticalScroll();

void InitializeBuffer();
void CheckBuffer();

void InsertCharacter(char Character);
void DeleteCharacter();

void PrintBuffer();

void InsertNewLine();

void MoveCursorLeft();
void MoveCursorRight();
void MoveCursorUp();
void MoveCursorDown();

int GetCursorX();
int GetCursorY();

int GetScrollX();
int GetScrollY();

#endif
