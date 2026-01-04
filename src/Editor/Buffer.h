#ifndef BUFFER_H
#define BUFFER_H

void InitializeBuffer();
void CheckBuffer();

void InsertCharacter(char Character);
void DeleteCharacter();

void PrintBuffer();

void MoveCursorX(int Amount);
void MoveCursorY(int Amount);

void MoveCursorLeft();
void MoveCursorRight();
void MoveCursorUp();
void MoveCursorDown();

int GetCursorX();
int GetCursorY();

#endif
