#include <stdio.h>
#include <stdlib.h>
#include "Editor/Terminal.h"
#include "Editor/Buffer.h"

int main() {
    InitializeBuffer();
    EnableRawMode();

    ClearScreen();

    while (1) {
        ClearScreen();
        PrintBuffer();
        
        SetCursorPosition(GetCursorX(), GetCursorY());

        char Character = ReadKey();

        if (Character == 27)
            break;
        else if (Character == 8)
            DeleteCharacter();
        else if (Character == '\r')
            InsertNewLine();
        else if (Character == -1) // LA
            MoveCursorLeft();
        else if (Character == -2) // RA
            MoveCursorRight();
        else if (Character == -3) // UA
            MoveCursorUp();
        else if (Character == -4) // DA
            MoveCursorDown();
        else
            InsertCharacter(Character);
    }

    DisableRawMode();

    return 0;
}
