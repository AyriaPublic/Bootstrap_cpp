/*
    Initial author: Convery (tcn@ayria.se)
    Started: 30-07-2017
    License: MIT
    Notes:
        Analyze a binary to get process information.
*/

#include "../Stdinclude.h"

namespace Bootstrap
{
    namespace Internal
    {
        // Header offsets for the fileformats.
        constexpr uint8_t PEHeadersize32 = 0xEC;
        constexpr uint8_t PEHeadersize64 = 0xFC;
    }

    // Basic filetype checking.
    bool isPE(const char *Targetpath)
    {
        std::FILE *Filehandle = std::fopen(Targetpath, "rb");
        if(Filehandle)
        {
            uint8_t MZBuffer[2]{};

            std::fread(MZBuffer, 2, 1, Filehandle);
            std::fclose(Filehandle);

            return 0 == std::memcmp(MZBuffer, "\x4D\x5A", 2);
        }

        return false;
    }
    bool isELF(const char *Targetpath)
    {
        std::FILE *Filehandle = std::fopen(Targetpath, "rb");
        if(Filehandle)
        {
            uint8_t ELFBuffer[4]{};

            std::fread(ELFBuffer, 4, 1, Filehandle);
            std::fclose(Filehandle);

            return 0 == std::memcmp(ELFBuffer, "\x7F\x45\x4C\x46", 4);
        }

        return false;
    }

    // Architecture checking.
    bool isPE64(const char *Targetpath)
    {
        std::FILE *Filehandle = std::fopen(Targetpath, "rb");
        if(Filehandle)
        {
            uint32_t NTHeaderoffset{};
            std::fseek(Filehandle, 0x3C, SEEK_SET);
            std::fread(&NTHeaderoffset, sizeof(uint32_t), 1, Filehandle);

            uint8_t PEBuffer[2]{};
            std::fseek(Filehandle, NTHeaderoffset + 0x18, SEEK_SET);
            std::fread(PEBuffer, 2, 1, Filehandle);
            std::fclose(Filehandle);

            return 0 == std::memcmp(PEBuffer, "\x0B\x02", 2);
        }

        return false;
    }
    bool isELF64(const char *Targetpath)
    {
        std::FILE *Filehandle = std::fopen(Targetpath, "rb");
        if(Filehandle)
        {
            uint8_t Format;
            std::fseek(Filehandle, 4, SEEK_SET);
            std::fread(&Format, sizeof(uint8_t), 1, Filehandle);
            std::fclose(Filehandle);

            return 2 == Format;
        }

        return false;
    }
    bool isManagedPE(const char *Targetpath)
    {
        std::FILE *Filehandle = std::fopen(Targetpath, "rb");
        if(Filehandle)
        {
            uint32_t NTHeaderoffset{};
            std::fseek(Filehandle, 0x3C, SEEK_SET);
            std::fread(&NTHeaderoffset, sizeof(uint32_t), 1, Filehandle);

            uint32_t COMHeaderoffset{};
            std::fseek(Filehandle, NTHeaderoffset + (isPE64(Targetpath) ? Internal::PEHeadersize64 : Internal::PEHeadersize32), SEEK_SET);
            std::fread(&COMHeaderoffset, sizeof(uint32_t), 1, Filehandle);
            std::fclose(Filehandle);

            return COMHeaderoffset != 0;
        }

        return false;
    }

    // Analyze a binary to get process information.
    Processtype Analyzetarget(const char *Targetpath)
    {
        if (isPE(Targetpath) && isManagedPE(Targetpath))
            return Processtype::PE_MANAGED;

        if (isPE(Targetpath) && isPE64(Targetpath))
            return Processtype::PE64_NATIVE;

        if (isPE(Targetpath) && !isPE64(Targetpath))
            return Processtype::PE32_NATIVE;

        if (isELF(Targetpath) && isELF64(Targetpath))
            return Processtype::ELF64_NATIVE;

        if (isELF(Targetpath) && !isELF64(Targetpath))
            return Processtype::ELF32_NATIVE;

        return Processtype::INVALID;
    }
}




