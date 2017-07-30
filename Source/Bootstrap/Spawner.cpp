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

    // The names of supported bootstrap modules.
    std::unordered_map<Processtype, std::string> Modulenames =
    {
        { Processtype::ELF32_NATIVE, "Nativebootstrap32.so" },
        { Processtype::ELF64_NATIVE, "Nativebootstrap64.so" },
        { Processtype::PE32_NATIVE, "Nativebootstrap32.dll" },
        { Processtype::PE64_NATIVE, "Nativebootstrap64.dll" }
    };

    #if defined (_WIN32)
    #include <windows.h>

    // Create a new process, either the target or another bootstrap.
    void *Spawnprocess(const char *Targetpath, Processtype Targettype)
    {

    }

    #else
    #include <sys/ptrace.h>
    #include <sys/user.h>
    #include <signal.h>
    #include <unistd.h>
    #include <dlfcn.h>
    #include <spawn.h>
    #include <wait.h>

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

            // Create a copy of the binary and hack.
            system(va("sudo cp %s %s_ayria", Targetpath, Targetpath).c_str());

            // Modify the copied binary to add an import.
            // TODO(Convery): Maybe implement this directly.
            system(va("sudo patchelf --add-needed libLinuxhack%s.so %s_ayria", Currentarchitecture, Targetpath).c_str());

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

            // Attach ptrace to the game.
            if(ptrace(PTRACE_ATTACH, ProcessID, NULL, NULL) == -1)
            {
                Infoprint("Could not attach to the game process.");
                Infoprint("Exiting.");
                exit(1);
            }

            // Wait for the process to be ready.
            int Status;
            if(waitpid(ProcessID, &Status, WUNTRACED) != ProcessID)
            {
                Infoprint("Could not attach to the game process.");
                Infoprint("Exiting.");
                exit(1);
            }

            // Environment information.
            {
                char *Fullpath = realpath(Modulenames[Targettype].c_str(), NULL);
                size_t Pathlength = std::strlen(Fullpath) + 1;

                // Get the libc location.
                auto Getlibc = [](pid_t ProcessID) -> unsigned long
                {
                    unsigned long Libcaddress = 0; char Buffer[1024];
                    std::FILE *Filehandle = std::fopen(va("/proc/%d/maps", ProcessID).c_str(), "r");
                    if(!Filehandle)
                    {
                        Infoprint("Could not read process memory mapping.");
                        Infoprint("Exiting.");
                        exit(1);
                    }
                    while(std::fgets(Buffer, 1024, Filehandle))
                    {
                        std::scanf("%lx-%*lx %*s %*s %*s %*d", &Libcaddress);
                        if(std::strstr(Buffer, "libc-")) break;
                    }
                    std::fclose(Filehandle);

                    return Libcaddress;
                };

                // Local libc information.
                size_t Locallibc = Getlibc(getpid());
                void *Dlopenaddress = dlsym(dlopen("libc.so.6", RTLD_LAZY), "__libc_dlopen_mode");
                void *Mallocaddress = dlsym(dlopen("libc.so.6", RTLD_LAZY), "malloc");
                void *Freeaddress = dlsym(dlopen("libc.so.6", RTLD_LAZY), "free");

                // Shared libc information.
                size_t Dlopenoffset = size_t(Dlopenaddress) - Locallibc;
                size_t Mallocoffset = size_t(Mallocaddress) - Locallibc;
                size_t Freeoffset = size_t(Freeaddress) - Locallibc;

                // Remote libc information.
                size_t Remotelibc = Getlibc(ProcessID);
                size_t Remotedlopen = Remotelibc + Dlopenoffset;
                size_t Remotemalloc = Remotelibc + Mallocoffset;
                size_t Remotefree = Remotelibc + Freeoffset;

                // Save the state of the process.
                user_regs_struct Oldstate{};
                if(ptrace(PTRACE_GETREGS, ProcessID, NULL, &Oldstate) == -1)
                {
                    Infoprint("Could not get the state of the game.");
                    Infoprint("Exiting.");
                    exit(1);
                }

                // Find a executable memory region in the game.
                auto Getcodepage = [](pid_t ProcessID) -> unsigned long
                {
                    unsigned long Address = 0; char Buffer[1024]; char Permissions[5]{}; char Device[6]{};
                    std::FILE *Filehandle = std::fopen(va("/proc/%d/maps", ProcessID).c_str(), "r");
                    if(!Filehandle)
                    {
                        Infoprint("Could not read process memory mapping.");
                        Infoprint("Exiting.");
                        exit(1);
                    }
                    while(std::fgets(Buffer, 1024, Filehandle))
                    {
                        std::scanf("%lx-%*lx %s %*s %s %*d", &Address, Permissions, Device);
                        if(Permissions[2] == 'x') break;
                    }
                    std::fclose(Filehandle);

                    return Address;
                };

                // Set the new register state per architecture, no ARM yet.
                auto Entrypoint = Getcodepage(ProcessID) + sizeof(long);
                user_regs_struct Newstate = Oldstate;
                #if defined (ENVIRONMENT64)
                Newstate.rip = Entrypoint + 2;
                Newstate.rdi = Remotemalloc;
                Newstate.rdx = Remotedlopen;
                Newstate.rsi = Remotefree;
                Newstate.rcx = Pathlength;
                #else
                Newstate.eip = Entrypoint;
                Newstate.ebx = Remotemalloc;
                Newstate.edi = Remotedlopen;
                Newstate.esi = Remotefree;
                Newstate.ecx = Pathlength;
                #endif
                if(ptrace(PTRACE_SETREGS, ProcessID, NULL, &Newstate) == -1)
                {
                    Infoprint("Could not set the state of the game.");
                    Infoprint("Exiting.");
                    exit(1);
                }

                // The loaders by architecture.
                #if defined (ENVIRONMENT64)
                auto Loadercode = []() [[noreturn]] -> void {};
                #else
                auto Loadercode = []() [[noreturn]] -> void
                {
                    // AT&T makes me a sad hedgehog.
                    asm
                    (
                        "dec %esi \n"           // Maybe optional.
                        "push %ecx \n"          // Push the size to allocate.
                        "call *%ebx \n"         // Call malloc.
                        "mov %eax, %ebx \n"     // Save the address to ebx.
                        "int $3 \n"             // Notify ptrace to write to the buffer.

                        "push $1 \n"            // Push RTLD_LAZY.
                        "push %ebx \n"          // Push the buffer address.
                        "call *%edi \n"         // Call dlopen.
                        "int $3 \n"             // Notify ptrace to check injection.

                        "push %ebx \n"          // Push the buffer to free.
                        "call *%esi \n"         // Call free.
                        "int $3"                // Notify ptrace that we are done.
                    );
                };
                #endif

                // Backup the codesegment and write our new one.
                auto Oldcode = std::make_unique<uint8_t []>(sizeof(Loadercode));



            }
        }

        // Resume the process.
        kill(ProcessID, SIGCONT);
    }
    #endif
}
