#include "PayloadResources.h"
#include <windows.h>
#include <cstdio>
#include <string>

namespace {
    enum class GameTarget {
        Unknown,
        SA,
        VC,
        III,
    };

    HMODULE payloadModule = nullptr;

    bool ReadUInt(unsigned int address, unsigned int& value) {
        __try {
            value = *reinterpret_cast<const unsigned int*>(address);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            value = 0;
            return false;
        }
    }

    bool MatchUInt(unsigned int address, unsigned int expected) {
        unsigned int value = 0;
        return ReadUInt(address, value) && value == expected;
    }

    GameTarget DetectGameTarget() {
        // 与 plugin-sdk 的 GameVersion 检测逻辑保持同源：读游戏内固定签名，不依赖 exe 文件名。
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

    int ResourceIdForTarget(GameTarget target) {
        switch (target) {
        case GameTarget::SA:
            return IDR_XMENU_SA;
        case GameTarget::VC:
            return IDR_XMENU_VC;
        case GameTarget::III:
            return IDR_XMENU_III;
        default:
            return 0;
        }
    }

    const char* NameForTarget(GameTarget target) {
        switch (target) {
        case GameTarget::SA:
            return "sa";
        case GameTarget::VC:
            return "vc";
        case GameTarget::III:
            return "iii";
        default:
            return "unknown";
        }
    }

    bool EnsureDirectory(const std::string& path) {
        if (CreateDirectoryA(path.c_str(), nullptr)) {
            return true;
        }
        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    bool WritePayload(HINSTANCE instance, GameTarget target, std::string& outputPath) {
        const int resourceId = ResourceIdForTarget(target);
        if (resourceId == 0) {
            return false;
        }

        HRSRC resource = FindResourceA(instance, MAKEINTRESOURCEA(resourceId), RT_RCDATA);
        if (!resource) {
            return false;
        }

        HGLOBAL loaded = LoadResource(instance, resource);
        if (!loaded) {
            return false;
        }

        const DWORD size = SizeofResource(instance, resource);
        const void* data = LockResource(loaded);
        if (!data || size == 0) {
            return false;
        }

        char tempPath[MAX_PATH]{};
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            return false;
        }

        std::string runtimeDir = std::string(tempPath) + "XMenu";
        if (!EnsureDirectory(runtimeDir)) {
            return false;
        }

        outputPath = runtimeDir + "\\XMenu_" + NameForTarget(target) + ".dll";
        HANDLE file = CreateFileA(outputPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD written = 0;
        const BOOL ok = WriteFile(file, data, size, &written, nullptr);
        CloseHandle(file);
        return ok && written == size;
    }

    void ShowError(const char* message) {
        MessageBoxA(HWND_DESKTOP, message, "XMenu", MB_ICONERROR);
    }

    bool LoadPayload(HINSTANCE instance) {
        const GameTarget target = DetectGameTarget();
        if (target == GameTarget::Unknown) {
            ShowError("Unsupported game executable. XMenu supports GTA III, GTA VC and GTA SA only.");
            return false;
        }

        std::string payloadPath;
        if (!WritePayload(instance, target, payloadPath)) {
            ShowError("Failed to extract XMenu runtime payload.");
            return false;
        }

        payloadModule = LoadLibraryA(payloadPath.c_str());
        if (!payloadModule) {
            char message[512]{};
            std::snprintf(message, sizeof(message), "Failed to load XMenu runtime payload. Error code: %lu", GetLastError());
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