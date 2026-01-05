#ifndef TERMINAL_H
#define TERMINAL_H

#include <windows.h>

void EnableRawMode();
void DisableRawMode();

void ClearScreen();
void SetCursorPosition(int X, int Y);

int GetTerminalRows();
int GetTerminalWidth();

int ReadKey();

#endif
