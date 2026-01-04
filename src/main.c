#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **TextBuffer;

int Lines = 0;
int AllocatedLines = 16;

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

void StartTerminal() {
    #if defined(_WIN32) || defined(_WIN64)
        system("start cmd");
    #elif defined(__APPLE__) && defined(__MACH__)
        system("open -a Terminal.app")
    #elif defined(__unix__) || defined(__unix)
        system("gnome-terminal")
    #else
        fprintf(stderr, "Unknown operating system\n");
    #endif
}

int main() {
    InitializeBuffer();
    StartTerminal();

    char Input[256];

    while (1) {
        CheckBuffer();

        printf("Enter text: ");
        fgets(Input, sizeof(Input), stdin);

        Input[strcspn(Input, "\n")] = 0;
        TextBuffer[Lines - 1] = strdup(Input);

        if (strcmp(TextBuffer[Lines-1], ":Exit") == 0) {
            printf("Exiting\n");

            break;
        }

        Lines++;
        TextBuffer[Lines - 1] = strdup("");
    }

    for (int i=0; i < Lines; i++)
        free(TextBuffer[i]);

    free(TextBuffer);

    return 0;
}
