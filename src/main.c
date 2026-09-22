#include <stdio.h>
#include <stdlib.h>
#include "Editor/Terminal.h"
#include "Editor/Buffer.h"

int main() {
    InitializeBuffer();
    EnableRawMode();

    ClearScreen();

    while (1) {
        if (!IsCommandErrorActive()) {
            ClearScreen();
            PrintBuffer();
            DrawStatusBar(GetFileName());

            SetCursorPosition(LINE_NUMBER_GUTTER + GetCursorX() - GetScrollX(), GetCursorY() - GetScrollY());
        }

        int Character = ReadKey();

        if (IsCommandErrorActive()) {
            ClearCommandError();
        }

        if (IsSearchActive()) {
            HandleSearchInput(Character);

            continue;
        }

        if (Character == 27)
            break;
        else if (Character == ':')
            EnterCommandMode();
        else if (Character == '!')
            EnterGlobalCommandMode();
        else if (Character == 6) {
            SetActiveSearch(1);

            ResetSearchLen();
            ResetSearchQuery();
        } else if (Character == 26)
            Undo();
        else if (Character == 25)
            Redo();
        else if (Character == 19)
            MEI_SaveFile();
        else if (Character == 15)
            MEI_OpenFile("Test.txt");
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
