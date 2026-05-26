#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <string>

namespace {
    enum class GameTarget {
        Unknown,
        SA,
        VC,
        III,
    };

    HMODULE payloadModule = nullptr;

    bool ReadUInt(std::uintptr_t address, unsigned int& value) {
        __try {
            value = *reinterpret_cast<const unsigned int*>(address);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            value = 0;
            return false;
        }
    }

    bool MatchUInt(std::uintptr_t address, unsigned int expected) {
        unsigned int value = 0;
        return ReadUInt(address, value) && value == expected;
    }

    GameTarget DetectGame() {
        if (MatchUInt(0x401000, 0x53EC8B55) || MatchUInt(0x401000, 0x16197BE9)
            || MatchUInt(0x8245BC, 0x94BF) || MatchUInt(0x8252FC, 0x94BF)
            || MatchUInt(0x82533C, 0x94BF) || MatchUInt(0x858D51, 0x3539F633)
            || MatchUInt(0x858C61, 0x3539F633)) {
            return GameTarget::SA;
        }

        if (MatchUInt(0x667BF0, 0x53E58955) || MatchUInt(0x667C40, 0x53E58955)
            || MatchUInt(0x666BA0, 0x53E58955)) {
            return GameTarget::VC;
        }

        if (MatchUInt(0x5C1E70, 0x53E58955) || MatchUInt(0x5C2130, 0x53E58955)
            || MatchUInt(0x5C6FD0, 0x53E58955)) {
            return GameTarget::III;
        }

        return GameTarget::Unknown;
    }

    const char* PayloadFileName(GameTarget target) {
        switch (target) {
        case GameTarget::SA:
            return "XMenuSA.dll";
        case GameTarget::VC:
            return "XMenuVC.dll";
        case GameTarget::III:
            return "XMenuIII.dll";
        case GameTarget::Unknown:
        default:
            return nullptr;
        }
    }

    std::string DirectoryFromModule(HMODULE module) {
        char path[MAX_PATH]{};
        const DWORD size = GetModuleFileNameA(module, path, MAX_PATH);
        if (size == 0) {
            return "";
        }

        std::string directory(path, size);
        const std::size_t slash = directory.find_last_of("\\/");
        if (slash == std::string::npos) {
            return "";
        }
        return directory.substr(0, slash + 1);
    }

    std::string RuntimePayloadPath(HINSTANCE instance, GameTarget target) {
        const char* fileName = PayloadFileName(target);
        if (!fileName) {
            return "";
        }

        const std::string asiDirectory = DirectoryFromModule(instance);
        if (asiDirectory.empty()) {
            return std::string("XMenu\\") + fileName;
        }
        return asiDirectory + "XMenu\\" + fileName;
    }

    void ShowError(const char* message) {
        MessageBoxA(HWND_DESKTOP, message, "XMenu", MB_ICONERROR);
    }

    bool LoadPayload(HINSTANCE instance) {
        const GameTarget target = DetectGame();
        if (target == GameTarget::Unknown) {
            ShowError("Failed to detect supported GTA runtime.\n\nSupported games: GTA SA, GTA Vice City, GTA III.");
            return false;
        }

        const std::string payloadPath = RuntimePayloadPath(instance, target);
        payloadModule = LoadLibraryA(payloadPath.c_str());
        if (!payloadModule) {
            char message[768]{};
            std::snprintf(
                message,
                sizeof(message),
                "Failed to load XMenu payload.\n\nExpected file:\n%s\n\nError code: %lu",
                payloadPath.c_str(),
                GetLastError()
            );
            ShowError(message);
            return false;
        }

        return true;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        LoadPayload(instance);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (payloadModule) {
            FreeLibrary(payloadModule);
            payloadModule = nullptr;
        }
    }

    return TRUE;
}