/*
    Initial author: Convery (tcn@hedgehogscience.com)
    Started: 08-01-2018
    License: MIT
    Notes:
        Provides the entrypoint for Windows and Nix.
*/

#include "Stdinclude.hpp"

// Standard entrypoint for ASCII / UTF8 systems.
int main(int argc, char **argv)
{
    Bootstrap::Game_t Target;

    // Verify that we have received enough info to parse.
    if(argc < 3)
    {
        // Log to the file and to stdout.
        Infoprint("Failed to start, missing parameters.");
        printf("Usage: Bootstrap[Architecture] [Gamedirectory] [Gamename] <Startuparguments>\n");
        printf("Example: Bootstrap32 \"C:/Steam/Tetris3\" \"win32/ship/tetris_32.exe\" -no_console -developer=1 -otherargument\n");
        return 1;
    }

    // Clear the previous sessions logfile.
    Clearlog();

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

    // Notify the user.
    std::string Targetinfo = "Bootstrap target:\n";
    Targetinfo += va("Directory: \"%s\"\n", Target.Targetdirectory.c_str());
    Targetinfo += va("Binary: \"%s\"\n", Target.Targetbinary.c_str());
    Targetinfo += va("Arguments: \"%s\"\n", Target.Startupargs.c_str());
    switch (Target.Targettype)
    {
        case Bootstrap::Processtype::ELF32_NATIVE: Targetinfo += va("Processtype: \"%s\"", "ELF32"); break;
        case Bootstrap::Processtype::ELF64_NATIVE: Targetinfo += va("Processtype: \"%s\"", "ELF64"); break;
        case Bootstrap::Processtype::PE32_NATIVE: Targetinfo += va("Processtype: \"%s\"", "PE32"); break;
        case Bootstrap::Processtype::PE64_NATIVE: Targetinfo += va("Processtype: \"%s\"", "PE64"); break;
        case Bootstrap::Processtype::PE_MANAGED: Targetinfo += va("Processtype: \"%s\"", "Managed"); break;
    };
    Infoprint(Targetinfo.c_str());

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
