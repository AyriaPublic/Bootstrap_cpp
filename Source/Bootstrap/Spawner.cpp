/*
    Initial author: Convery (tcn@ayria.se)
    Started: 30-07-2017
    License: MIT
    Notes:
        Simply creates a new process and injects into it.
*/

#include "../Stdinclude.h"

namespace Bootstrap
{
    // Saved copy of the process input for nix.
    int Savedargc; char **Savedargv;

    // Helpers for when we need to switch the bootstapper version.
    using Processthread = std::pair<void * /* Processhandle */, void * /* Threadhandle */>;
    constexpr const char *Currentarchitecture = sizeof(void *) == sizeof(uint64_t) ? "64" : "32";
    constexpr const char *Complimentarchitecture = sizeof(void *) == sizeof(uint64_t) ? "32" : "64";

    #if defined (_WIN32)
    #include <windows.h>

    // Create a new process, either the target or another bootstrap.
    void *Spawnprocess(const char *Targetpath, Processtype Targettype)
    {

    }

    #else
    #include <signal.h>
    #include <unistd.h>
    #include <spawn.h>

    // Create a new process, either the target or another bootstrap.
    void *Spawnprocess(const char *Targetpath, Processtype Targettype)
    {
        pid_t ProcessID;

        // Check if we should spawn the other bootstrapper.
        if((Targettype == Processtype::ELF64_NATIVE && sizeof(void *) != sizeof(uint64_t)) ||
            (Targettype == Processtype::ELF32_NATIVE && sizeof(void *) == sizeof(uint64_t)))
        {
            // Replace the current arch with its compliment.
            std::string Targetname = Savedargv[0];
            Targetname.replace(Targetname.find_last_of(Currentarchitecture), 2, Complimentarchitecture);

            // Recreate the commandline.
            char **Arguments = new char *[Savedargc]();
            for(int i = 1; i < Savedargc; ++i)
            {
                Arguments[i - 1] = Savedargv[i];
            }

            // Replace the current process with the other bootstrapper.
            execvp(Targetname.c_str(), Arguments);

            // This code should never be executed.
            Infoprint("Could not spawn the other bootstrapper for some reason.");
            Infoprint("Please notify the developers or debug this if you can.");
            Infoprint("Exiting and giving up on trying to start the game.");
            exit(1);
        }

        // Only Apple and Blackberry allows starting processes suspended.
        #if defined (__APPLE__) || defined (__QNXNTO__)
        {
            /*
                TODO(Convery):
                Find someone that actually uses a Mac to test.
                I have no idea if this code even compiles.
            */

            // Recreate the commandline.
            char **Arguments = new char *[Savedargc - 1]();
            for(int i = 2; i < Savedargc; ++i)
            {
                Arguments[i - 2] = Savedargv[i];
            }

            // Process spawn attributes.
            posix_spawnattr_t Attributes;
            posix_spawnattr_init(&Attributes);
            posix_spawnattr_setflags(&Attributes, POSIX_SPAWN_START_SUSPENDED);
            // #if defined (__QNXNTO__)  posix_spawnattr_setflags(&Attributes, POSIX_SPAWN_HOLD);

            // Create a new process.
            int Status = posix_spawn(&ProcessID, Targetpath, NULL, &Attributes, Arguments, NULL);

            // Destroy the spawn attributes.
            posix_spawnattr_destroy(&Attributes);

            // Check for errors.
            if(Status != 0)
            {
                Infoprint("Could not create the game process using posix_spawn.");
                Infoprint("Will try to use the linux hackery.");
            }
        }
        #endif

        // Linux needs some hackery to suspend the process on startup.
        {
            // Check if the other spawner ran.
            if(ProcessID) goto LABEL_INJECT;

            // Create a copy of the binary.
            char Buffer[8192]; size_t Count = 0;
            std::FILE *Srchandle = std::fopen(Targetpath, "rb");
            std::FILE *Dsthandle = std::fopen(va("%s_ayria", Targetpath).c_str(), "wb");
            while (Count = std::fread(Buffer, 1, 8192, Srchandle)) std::fwrite(Buffer, 1, Count, Dsthandle);
            std::fclose(Srchandle); std::fclose(Dsthandle);

            // Modify the copied binary to add an import.
            /*
                TODO(Convery):
                Open the copied ELF file and modify the dynamic segment
                to add an import to our SO that suspends the host
                process when it's loaded.
            */

            // Recreate the commandline.
            char **Arguments = new char *[Savedargc - 1]();
            for(int i = 2; i < Savedargc; ++i)
            {
                Arguments[i - 2] = Savedargv[i];
            }

            // Process spawn attributes.
            posix_spawnattr_t Attributes;
            posix_spawnattr_init(&Attributes);

            // Create a new process.
            int Status = posix_spawn(&ProcessID, va("%s_ayria", Targetpath).c_str(), NULL, &Attributes, Arguments, NULL);

            // Destroy the spawn attributes.
            posix_spawnattr_destroy(&Attributes);

            // Check for errors.
            if(Status != 0)
            {
                Infoprint("Could not create the game process using posix_spawn.");
                Infoprint("Game over, man, game over.");
                Infoprint("Exiting.");
                exit(1);
            }
        }

        // Inject the modules into the process.
        {
            LABEL_INJECT:;

            /*
                TODO(Convery):
                Use ptrace to inject the bootstrap module.
                We could just replace the 'hack' SO with
                the bootstrap module but let's try to
                mimic windows for now.

                https://github.com/gaffe23/linux-inject
            */
        }

        // Resume the process.
        kill(ProcessID, SIGCONT);
    }
    #endif
}
