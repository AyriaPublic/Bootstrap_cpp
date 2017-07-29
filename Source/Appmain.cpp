/*
    Initial author: Convery (tcn@ayria.se)
    Started: 29-07-2017
    License: MIT
    Notes:
        Module entrypoint.
*/

#include "Stdinclude.h"

// Standard entrypoint for ASCII / UTF8 systems.
int main(int argc, char **argv)
{
    // Verify that we have received an app to start.
    if(argc < 2)
    {
        // Log to the file and to stdout.
        Infoprint("Failed to start, missing parameters.");
        printf("Usage:\n%s Game [Startup arguments]\n", argv[0]);
        return 1;
    }

    return 0;
}
