/*
    Initial author: Convery (tcn@ayria.se)
    Started: 29-07-2017
    License: MIT
    Notes:
        Module entrypoint.
*/

#include "Stdinclude.h"

// Delete the last sessions log on startup.
namespace { struct Deletelog { Deletelog() { Clearlog(); } }; static Deletelog Deleted{}; }

// Standard entrypoint for ASCII / UTF8 systems.
int main(int argc, char **argv)
{
    Bootstrap::Game_t Target;

    // Verify that we have received enough info to parse.
    if(argc < 3)
    {
        // Log to the file and to stdout.
        Infoprint("Failed to start, missing parameters.");
        printf("Usage: Bootstrap[Architecture].exe [Gamedirectory] [Gamename] <Startuparguments>\n");
        printf("Example: Bootstrap32.exe \"C:/Steam/Tetris3\" \"win32/ship/tetris_32.exe\" -no_console -developer=1 -otherargument\n");
        return 1;
    }

    // Parse the input about our target.
    Target.Bootstrapperversion = argv[0];
    Target.Targetdirectory = argv[1];
    Target.Targetbinary = argv[2];
    for (int i = 3; i < argc; ++i)
    {
        Target.Startupargs += argv[i];
        Target.Startupargs += " ";
    }

    // Get the type of bootstrapper we'll need.
    Target.Targettype = Bootstrap::Analyzetarget((Target.Targetdirectory + "/" + Target.Targetbinary).c_str());
    if (Target.Targettype == Bootstrap::Processtype::INVALID)
    {
        // Log to the file and to stdout.
        Infoprint("Failed to start, invalid target binary.");
        printf("The target \"%s\" is not a supported applicationtype.\n", (Target.Targetdirectory + "/" + Target.Targetbinary).c_str());
        return 1;
    }

    // Spawn the gameprocess.
    if (!Bootstrap::Spawnprocess(Target))
    {
        // Log to the file and to stdout.
        Infoprint("Failed to start, could not start/inject into the game.");
        printf("Failed to start. Please verify your input, game, and that you have permission to modify the game.\n");
        return 1;
    }

    return 0;
}
