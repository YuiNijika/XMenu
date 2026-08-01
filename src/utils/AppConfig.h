#pragma once

#include "DataManager.h"
#include <XBase/Input.h>

#include <cstddef>
#include <string>
#include <vector>

namespace AppConfig {
    using Hotkey = XBase::Input::Hotkey;

    struct ActionHotkey {
        std::string id;
        std::string name;
        Hotkey hotkey;
    };

    struct UpdateCache {
        long long timestamp = 0;
        std::string tagName;
        std::string htmlUrl;
        std::string source; // GTAMODX | GitHub
    };

    struct PersistentFeatureState {
        std::string id;
        std::string name;
        std::string group;
        bool enabledNow = false;
        bool restoreNextLaunch = false;
        bool restoreValue = false;
    };

    enum class TransferScope {
        All,
        CustomLocations
    };

    void Init();
    void Save();
    std::string GetConfigPath();
    bool ImportFrom(const std::string& path, TransferScope scope = TransferScope::All);
    bool ImportFromText(const std::string& text, TransferScope scope = TransferScope::All);
    bool ExportTo(const std::string& path, TransferScope scope = TransferScope::All);
    std::string ExportToText(TransferScope scope = TransferScope::All);

    bool LoadUpdateCache(UpdateCache& cache);
    void SaveUpdateCache(const UpdateCache& cache);

    std::vector<PersistentFeatureState> GetPersistentFeatureStates();
    std::size_t GetPersistentRestoreCount();
    void CaptureEnabledPersistentFeatures();
    void ApplyPersistentRestoreSelection(const std::vector<std::string>& selectedIds);
    void ClearPersistentRestoreSelection();
    void SyncPersistentState();

    const Hotkey& GetMenuHotkey();
    bool IsMenuHotkeyPressed();
    std::string GetMenuKeyName();
    void SetMenuKeyName(const std::string& keyName);
    std::string GetFallbackLanguageCode();
    void SetFallbackLanguageCode(const std::string& code);

    const std::vector<ActionHotkey>& GetActionHotkeys();
    bool IsActionHotkeySupportedForRuntime(const std::string& actionId);
    const Hotkey* GetActionHotkey(const std::string& actionId);
    std::string GetActionHotkeyName(const std::string& actionId);
    void SetActionHotkeyName(const std::string& actionId, const std::string& keyName);
    bool IsHotkeyPressed(const Hotkey& hotkey);
    std::string FormatHotkey(const Hotkey& hotkey);

    const std::vector<DataManager::LocationData>& GetCustomLocations();
    bool AddCustomLocation(const std::string& name, float x, float y, float z, int interior);

    int GetGuiThemeIndex();
    void SetGuiThemeIndex(int index);
    int GetInteractionMode();
    void SetInteractionMode(int mode);
}