/*
    Initial author: Convery (tcn@ayria.se)
    Started: 30-07-2017
    License: MIT
    Notes:
        Simply creates a new process and injects into it.
*/

#include "../Stdinclude.h"

#if defined (_WIN32)
    #include <Windows.h>
#else
    #error *nix builds are no longer supported, use LD_PRELOAD / DYLD_INSERT_LIBRARIES to inject the bootstrapper.
#endif

namespace Bootstrap
{
    // Helpers for when we need to switch the bootstapper version.
    constexpr const char *Currentarchitecture = sizeof(void *) == sizeof(uint64_t) ? "64" : "32";
    constexpr const char *Complimentarchitecture = sizeof(void *) == sizeof(uint64_t) ? "32" : "64";

    // The names of supported bootstrap modules.
    std::unordered_map<Processtype, std::string> Modulenames =
    {
        { Processtype::ELF32_NATIVE, "Nativebootstrap32.so" },
        { Processtype::ELF64_NATIVE, "Nativebootstrap64.so" },
        { Processtype::PE32_NATIVE, "Nativebootstrap32.dll" },
        { Processtype::PE64_NATIVE, "Nativebootstrap64.dll" }
    };

    // Create a new process, either the target or another bootstrap.
    bool Spawnprocess(Game_t &Target)
    {
        void *Processhandle; void *Threadhandle;

        // Check if we should spawn the other bootstrapper.
        if ((Target.Targettype == Processtype::PE64_NATIVE && sizeof(void *) != sizeof(uint64_t)) ||
            (Target.Targettype == Processtype::PE32_NATIVE && sizeof(void *) == sizeof(uint64_t)))
        {
            // Notify the user.
            Infoprint(va("Bootstrap%s does not support %s-bit games. Launching Bootstrap%s.", Currentarchitecture, Complimentarchitecture, Complimentarchitecture));

            // Replace the current architecture with its compliment.
            Target.Bootstrapperversion.replace(Target.Bootstrapperversion.find_last_of(Currentarchitecture) - 1,
                2, Complimentarchitecture);

            // Create a commandline.
            std::string Commandline = Target.Bootstrapperversion + " \"" + Target.Targetdirectory + "\" \"" + Target.Targetbinary + "\" " + Target.Startupargs;

            // Spawn the bootstrapper.
            STARTUPINFO Unused1{}; PROCESS_INFORMATION Unused2{};
            return TRUE == CreateProcessA(Target.Bootstrapperversion.c_str(), (char *)Commandline.c_str(), NULL, NULL, NULL, NULL, NULL, NULL, &Unused1, &Unused2);
        }

        // Spawn the game we are interested in.
        {
            // Create a command-line.
            std::string Commandline = "\"" + Target.Targetdirectory + "\" \"" + Target.Targetbinary + "\" " + Target.Startupargs;

            // Spawn the game.
            STARTUPINFO Startupinfo{}; PROCESS_INFORMATION Processinfo{};
            if (FALSE == CreateProcessA((Target.Targetdirectory + "/" + Target.Targetbinary).c_str(), (char *)Commandline.c_str(), NULL, NULL, NULL, CREATE_SUSPENDED | DETACHED_PROCESS, NULL, Target.Targetdirectory.c_str(), &Startupinfo, &Processinfo))
                return false;

            // Notify the user.
            Infoprint(va("Started: \"%s\"", (Target.Targetdirectory + "/" + Target.Targetbinary).c_str()));

            // Extract the handles for the new process.
            Processhandle = Processinfo.hProcess;
            Threadhandle = Processinfo.hThread;
        }

        // Inject the bootstrapmodule.
        {
            char CWDBuffer[256]{};
            GetCurrentDirectoryA(256, CWDBuffer);

            std::string Fullpath = CWDBuffer;
            Fullpath += "/" + Modulenames[Target.Targettype];

            auto Libraryaddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
            auto Remotestring = VirtualAllocEx(Processhandle, NULL, Fullpath.size() + 1, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            // Notify the user.
            Infoprint(va("Injecting \"%s\" into \"%s\"", Modulenames[Target.Targettype].c_str(), Target.Targetbinary.c_str()));

            WriteProcessMemory(Processhandle, Remotestring, Fullpath.c_str(), Fullpath.size() + 1, NULL);
            auto Result = CreateRemoteThread(Processhandle, NULL, NULL, LPTHREAD_START_ROUTINE(Libraryaddress), Remotestring, NULL, NULL);
            WaitForSingleObject(Result, 3000);

            if (Result == NULL)
            {
                TerminateProcess(Processhandle, 0xDEADC0DE);
                return false;
            }
         }

        // Resume the process and return.
        ResumeThread(Threadhandle);
        return true;
    }
}
