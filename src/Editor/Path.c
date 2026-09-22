#include <string.h>

#include "Path.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

    #include <windows.h>

    int GetFullPath(const char *Path, char *Buffer, size_t BufferSize) {
        DWORD Result = GetFullPathNameA(Path, (DWORD)BufferSize, Buffer, NULL);
        if (Result == 0)
            return 0;

        if (Result >= BufferSize)
            return 0;

        return 1;
    }
#else
    #include <stdlib.h>
    #include <limits.h>

    int GetFullPath(const char *Path, char *Buffer, size_t BufferSize) {
        char *Resolved = realpath(Path, NULL);
        if (!Resolved)
            return 0;

        size_t Length = strlen(Resolved);

        if (Length >= BufferSize) {
            free(Resolved);

            return 0;
        }

        memcpy(Buffer, Resolved, Length + 1);

        free(Resolved);

        return 1;
    }
#endif
