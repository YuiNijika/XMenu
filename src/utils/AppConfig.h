#pragma once
#include "DataManager.h"
#include <windows.h>
#include <string>
#include <vector>

namespace AppConfig {
    struct Hotkey {
        int key = 'M';
        bool ctrl = false;
        bool alt = false;
        bool shift = false;
    };

    enum class TransferScope {
        All,
        CustomLocations
    };

    void Init();
    void Save();
    std::string GetConfigPath();
    bool ImportFrom(const std::string& path, TransferScope scope = TransferScope::All);
    bool ExportTo(const std::string& path, TransferScope scope = TransferScope::All);

    const Hotkey& GetMenuHotkey();
    bool IsMenuHotkeyPressed();
    int GetMenuKeyVirtualKey();
    std::string GetMenuKeyName();
    void SetMenuKeyName(const std::string& keyName);

    const std::vector<DataManager::LocationData>& GetCustomLocations();
    bool AddCustomLocation(const std::string& name, float x, float y, float z, int interior);
}