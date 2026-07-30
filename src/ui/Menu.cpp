#include "Menu.h"
#include <array>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include "game/Runtime.h"
#include "utils/Log.h"
#include "utils/D3DHook.h"
#include "utils/I18n.h"
#include "utils/UpdateChecker.h"
#include "utils/AppConfig.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "ui/MenuState.h"
#include "ui/GuiTheme.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "ui/pages/Player.h"
#include "ui/pages/Vehicle.h"
#include "ui/pages/Teleport.h"
#include "ui/pages/Weapon.h"
#include "ui/pages/World.h"
#include "ui/pages/Ped.h"
#include "ui/pages/Scene.h"
#include "ui/pages/Visual.h"
#include "controllers/Hotkeys.h"
#include "controllers/Overlay.h"
#include "controllers/Command.h"
#include "controllers/Teleport.h"
#include "controllers/World.h"
#include "controllers/BulletAssist.h"

extern const bool XMENU_DEBUG_MODE;
extern const char* XMENU_VERSION;
extern const char* XMENU_AUTHOR;
extern const char* XMENU_AUTHOR_TEST;
extern const char* XMENU_GITHUB;
extern const char* XMENU_URL;
extern const char* XMENU_QQ_GROUP;
extern const char* XMENU_TECH_STACK;
extern const char* XMENU_OPEN_SOURCE_LIBS;

namespace {
    const char* T(const char* key) {
        return I18n::T(key);
    }

    enum class Page {
        Player,
        Vehicle,
        Ped,
        Weapon,
        World,
        Scene,
        Visual,
        Teleport,
        Settings,
        About
    };

}

namespace Menu {
    struct NavItem {
        Page page;
        const char* textKey;
        const char* id;
    };
    
    bool menuVisible = false;
    Page activePage = Page::Player;
    std::vector<Page> pageStack;

    void PushPage(Page page) {
        pageStack.push_back(activePage);
        activePage = page;
        UI::ResetListIndex();
    }

    void PopPage() {
        if (!pageStack.empty()) {
            activePage = pageStack.back();
            pageStack.pop_back();
            UI::ResetListIndex();
        }
    }


    int configTransferScope = 1;
    char configImportText[65536] = "";
    char configExportText[65536] = "";
    char configStatus[256] = "";

    struct PersistentRestoreDialogState {
        std::vector<AppConfig::PersistentFeatureState> features;
        std::vector<bool> selected;
        bool open = false;
    };

    PersistentRestoreDialogState persistentRestoreDialog;
    bool openConfigImportPopup = false;
    bool openConfigExportPopup = false;
    const char* PersistentStatePopupId = "PersistentStatePopup";
    const char* ConfigImportPopupId = "ConfigImportTextPopup";
    const char* ConfigExportPopupId = "ConfigExportTextPopup";

    const NavItem navItems[] = {
        {Page::Player, "tab.player", "player"},
        {Page::Vehicle, "tab.vehicle", "vehicle"},
        {Page::Ped, "tab.ped", "ped"},
        {Page::Weapon, "tab.weapon", "weapon"},
        {Page::World, "tab.world", "world"},
        {Page::Scene, "tab.scene", "scene"},
        {Page::Visual, "tab.visual", "visual"},
        {Page::Teleport, "tab.teleport", "teleport"},
        {Page::Settings, "tab.settings", "settings"},
        {Page::About, "tab.about", "about"}
    };

    bool IsSaRuntime() {
        return GameRuntime::Current().target == GameRuntime::Target::SA;
    }

    bool IsPageAvailable(Page page) {
        if (page == Page::Scene) {
            return IsSaRuntime();
        }
        return true;
    }

    void EnsureActivePageAvailable() {
        if (!IsPageAvailable(activePage)) {
            activePage = Page::Player;
        }
    }

    void LabelWithStableId(char* output, std::size_t outputSize, const char* textKey, const char* id) {
        std::snprintf(output, outputSize, "%s###%s", T(textKey), id);
    }

    void DrawSettings();
    void DrawGuiSettings();
    void DrawRuntimeSettings();
    void DrawOverlaySettings();
    void DrawPersistentStateSettings();
    void DrawPersistentStatePopup();
    void DrawActionHotkeys();
    void DrawConfigSettings();
    void DrawConfigImportPopup();
    void DrawConfigExportPopup();
    void DrawUpdateSettings();
    void DrawLogViewer();
    void DrawDebugSettings();
    void DrawAbout();
    void DrawUpdateDialog();
    void DrawVersionBadge();

    void HandleMainWindowDrag() {
        ImGuiIO& io = ImGui::GetIO();
        const ImVec2 windowPos = ImGui::GetWindowPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const float titleHeight = ImGui::GetFrameHeight();
        const ImVec2 mouse = io.MousePos;

        const bool inTitleBar =
            mouse.x >= windowPos.x && mouse.x <= windowPos.x + windowSize.x &&
            mouse.y >= windowPos.y && mouse.y <= windowPos.y + titleHeight;

        if (inTitleBar && ImGui::IsMouseDown(0) && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)) {
            ImGui::SetWindowPos(ImVec2(windowPos.x + io.MouseDelta.x, windowPos.y + io.MouseDelta.y));
        }
    }

    void DrawPageHeader(const char* titleKey) {
        ImGui::TextUnformatted(T(titleKey));
        ImGui::Separator();
        ImGui::Spacing();

        if (MenuState::HasNotice()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.78f, 0.25f, 1.0f));
            ImGui::TextWrapped("%s", MenuState::NoticeText);
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }
    }

    void DrawVersionBadge() {
        const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
        const char* remoteVersion = info.latestVersion.empty() ? T("status.remoteUnknown") : info.latestVersion.c_str();
        const char* statusText = T("status.remoteUnknown");
        const char* sourceText = info.sourceName.empty()
            ? UpdateChecker::SourceDisplayName(info.source)
            : info.sourceName.c_str();

        ImVec4 versionColor = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        if (UpdateChecker::IsChecking()) {
            statusText = T("status.checking");
        } else if (info.status == UpdateChecker::VersionStatus::Equal) {
            versionColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            statusText = T("status.upToDate");
        } else if (info.status == UpdateChecker::VersionStatus::LocalNewer) {
            versionColor = ImVec4(1.0f, 0.30f, 0.25f, 1.0f);
            statusText = T("status.localNewer");
        } else if (info.status == UpdateChecker::VersionStatus::RemoteNewer) {
            versionColor = ImVec4(1.0f, 0.82f, 0.20f, 1.0f);
            statusText = T("status.remoteNewer");
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%s", XMENU_VERSION);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, versionColor);
        ImGui::SmallButton("?##VersionInfo");
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(statusText);
            ImGui::Separator();
            ImGui::Text("%s: %s", T("status.language"), I18n::GetLanguageName(I18n::GetLanguage()));
            ImGui::Text(T("status.localVersion"), XMENU_VERSION);
            ImGui::Text(T("status.remoteVersion"), remoteVersion);
            ImGui::Text(T("update.source"), sourceText);
            ImGui::EndTooltip();
        }
    }

    void DrawNavigation() {
        ImGui::TextUnformatted("XMenu");
        DrawVersionBadge();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        EnsureActivePageAvailable();

        for (const NavItem& item : navItems) {
            if (!IsPageAvailable(item.page)) {
                continue;
            }

            char label[96] = {};
            LabelWithStableId(label, sizeof(label), item.textKey, item.id);
            const bool selected = activePage == item.page;
            if (ImGui::Selectable(label, selected, 0, ImVec2(0.0f, 34.0f))) {
                activePage = item.page;
            }
        }

        const float footerHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
        const float footerY = ImGui::GetWindowContentRegionMax().y - footerHeight;
        if (ImGui::GetCursorPosY() < footerY) {
            ImGui::SetCursorPosY(footerY);
        }
        ImGui::Separator();
        ImGui::TextDisabled("%s: %s", T("status.language"), I18n::GetLanguageName(I18n::GetLanguage()));
    }

    void DrawActivePage() {
        const char* titleKey = "tab.player";
        switch (activePage) {
            case Page::Player: titleKey = "tab.player"; break;
            case Page::Vehicle: titleKey = "tab.vehicle"; break;
            case Page::Ped: titleKey = "tab.ped"; break;
            case Page::Weapon: titleKey = "tab.weapon"; break;
            case Page::World: titleKey = "tab.world"; break;
            case Page::Scene: titleKey = "tab.scene"; break;
            case Page::Visual: titleKey = "tab.visual"; break;
            case Page::Teleport: titleKey = "tab.teleport"; break;
            case Page::Settings: titleKey = "tab.settings"; break;
            case Page::About: titleKey = "tab.about"; break;
        }

        if (!MenuState::UseNativeMenu) {
            DrawPageHeader(titleKey);
        }

        switch (activePage) {
            case Page::Player: Pages::Player::Draw(); break;
            case Page::Vehicle: Pages::Vehicle::Draw(); break;
            case Page::Ped: Pages::Ped::Draw(); break;
            case Page::Weapon: Pages::Weapon::Draw(); break;
            case Page::World: Pages::World::Draw(); break;
            case Page::Scene: Pages::Scene::Draw(); break;
            case Page::Visual: Pages::Visual::Draw(); break;
            case Page::Teleport: Pages::Teleport::Draw(); break;
            case Page::Settings: DrawSettings(); break;
            case Page::About: DrawAbout(); break;
        }
    }

    void DrawSettings() {
        ImGui::TextUnformatted(T("settings.interfaceLanguage"));
        ImGui::Spacing();

        const std::vector<I18n::LanguageInfo>& languages = I18n::GetAvailableLanguages();
        const std::string currentLanguageCode = I18n::GetCurrentLanguageCode();
        if (ImGui::BeginCombo("##InterfaceLanguage", I18n::GetLanguageName(currentLanguageCode))) {
            for (const I18n::LanguageInfo& language : languages) {
                const bool selected = currentLanguageCode == language.code;
                if (ImGui::Selectable(language.name.c_str(), selected)) {
                    I18n::SetLanguage(language.code);
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::TextUnformatted(T("settings.fallbackLanguage"));
        const std::string fallbackLanguageCode = AppConfig::GetFallbackLanguageCode();
        if (ImGui::BeginCombo("##FallbackLanguage", I18n::GetLanguageName(fallbackLanguageCode))) {
            for (const I18n::LanguageInfo& language : languages) {
                const bool selected = fallbackLanguageCode == language.code;
                if (ImGui::Selectable(language.name.c_str(), selected)) {
                    AppConfig::SetFallbackLanguageCode(language.code);
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::TextDisabled("%s", T("settings.fallbackLanguageHint"));

        ImGui::Spacing();
        ImGui::TextDisabled("%s", T("settings.applyImmediately"));

        static char hotkeyInput[32] = "";
        if (hotkeyInput[0] == '\0') {
            std::snprintf(hotkeyInput, sizeof(hotkeyInput), "%s", AppConfig::GetMenuKeyName().c_str());
        }
        ImGui::Spacing();
        ImGui::InputTextWithHint(T("settings.menuHotkey"), "M", hotkeyInput, sizeof(hotkeyInput));
        ImGui::SameLine();
        if (ImGui::Button(T("settings.applyHotkey"))) {
            AppConfig::SetMenuKeyName(hotkeyInput);
            std::snprintf(hotkeyInput, sizeof(hotkeyInput), "%s", AppConfig::GetMenuKeyName().c_str());
        }
        ImGui::TextDisabled(T("settings.currentHotkey"), AppConfig::GetMenuKeyName().c_str());

        UI::SpacingSeparator();
        DrawGuiSettings();

        UI::SpacingSeparator();
        DrawRuntimeSettings();

        UI::SpacingSeparator();
        DrawOverlaySettings();

        UI::SpacingSeparator();
        DrawPersistentStateSettings();

        UI::SpacingSeparator();
        DrawActionHotkeys();

        UI::SpacingSeparator();
        DrawConfigSettings();

        UI::SpacingSeparator();
        DrawUpdateSettings();
        if (XMENU_DEBUG_MODE) {
            UI::SpacingSeparator();
            DrawDebugSettings();
        }
        UI::SpacingSeparator();
        DrawLogViewer();
    }

    void DrawGuiSettings() {
        ImGui::TextUnformatted(T("settings.guiStyle"));
        ImGui::Spacing();

        bool listChanged = UI::Checkbox(T("settings.useListMenu"), &MenuState::UseNativeMenu);
        if (MenuState::UseNativeMenu) {
            listChanged |= UI::Checkbox(T("settings.enableListMouse"), &MenuState::ListMenuMouseInput);
            ImGui::TextDisabled("%s", T("settings.listMouseHint"));
        }
        if (listChanged) {
            AppConfig::Save();
            GuiTheme::Sync();
            if (MenuState::UseNativeMenu) {
                UI::ResetListIndex();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted(T("settings.theme"));
        int currentTheme = GuiTheme::GetThemeIndex();
        if (ImGui::BeginCombo("##GuiTheme", T(GuiTheme::GetThemeNameKey(currentTheme)))) {
            for (int i = 0; i < GuiTheme::ThemeCount; ++i) {
                const bool selected = i == currentTheme;
                if (ImGui::Selectable(T(GuiTheme::GetThemeNameKey(i)), selected)) {
                    AppConfig::SetGuiThemeIndex(i);
                    GuiTheme::Sync();
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (!MenuState::UseNativeMenu) {
            ImGui::Spacing();
            ImGui::TextUnformatted(T("settings.interactionMode"));
            int currentInteraction = GuiTheme::GetInteractionIndex();
            if (ImGui::BeginCombo("##InteractionMode", T(GuiTheme::GetInteractionNameKey(currentInteraction)))) {
                for (int i = 0; i < GuiTheme::InteractionCount; ++i) {
                    const bool selected = i == currentInteraction;
                    if (ImGui::Selectable(T(GuiTheme::GetInteractionNameKey(i)), selected)) {
                        AppConfig::SetInteractionMode(i);
                        GuiTheme::Sync();
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::TextDisabled("%s", T("settings.interactionHint"));
        } else {
            ImGui::Spacing();
            ImGui::TextDisabled("%s", T("settings.listNavHint"));
        }
    }

    void DrawRuntimeSettings() {
        ImGui::TextUnformatted(T("settings.runtime"));
        ImGui::Checkbox(T("command.enabled"), &MenuState::CommandWindowEnabled);
    }

    void DrawOverlaySettings() {
        ImGui::TextUnformatted(T("settings.overlay"));
        ImGui::TextDisabled("%s", T("settings.overlayHint"));

        bool changed = false;
        changed |= ImGui::Checkbox(T("overlay.enabled"), &MenuState::OverlayEnabled);
        ImGui::SameLine();
        changed |= ImGui::Checkbox(T("overlay.showDetails"), &MenuState::OverlayShowDetails);
        ImGui::SameLine();
        changed |= ImGui::Checkbox(T("overlay.showFeatures"), &MenuState::OverlayShowFeatures);

        ImGui::Columns(2, nullptr, false);
        changed |= ImGui::Checkbox(T("overlay.showPosition"), &MenuState::OverlayShowPosition);
        ImGui::NextColumn();
        changed |= ImGui::Checkbox(T("overlay.showPlayer"), &MenuState::OverlayShowPlayer);
        ImGui::NextColumn();
        changed |= ImGui::Checkbox(T("overlay.showVehicle"), &MenuState::OverlayShowVehicle);
        ImGui::NextColumn();
        changed |= ImGui::Checkbox(T("overlay.showTime"), &MenuState::OverlayShowTime);
        ImGui::NextColumn();
        changed |= ImGui::Checkbox(T("overlay.showWorld"), &MenuState::OverlayShowWorld);
        ImGui::NextColumn();
        changed |= ImGui::Checkbox(T("overlay.showFps"), &MenuState::OverlayShowFps);
        ImGui::Columns(1);

        if (changed) {
            AppConfig::Save();
        }
    }

    void DrawPersistentStateSettings() {
        ImGui::TextUnformatted(T("settings.persistentState"));
        ImGui::TextDisabled("%s", T("settings.persistentStateHint"));

        const std::size_t restoreCount = AppConfig::GetPersistentRestoreCount();
        ImGui::TextDisabled(T("settings.persistentState.restoreCount"), static_cast<int>(restoreCount));

        if (ImGui::Button(T("settings.persistentState.captureCurrent"), ImVec2(190.0f, 0.0f))) {
            AppConfig::CaptureEnabledPersistentFeatures();
        }
        ImGui::SameLine();
        if (ImGui::Button(T("settings.persistentState.manage"), ImVec2(140.0f, 0.0f))) {
            persistentRestoreDialog.features = AppConfig::GetPersistentFeatureStates();
            persistentRestoreDialog.selected.clear();
            persistentRestoreDialog.selected.reserve(persistentRestoreDialog.features.size());
            for (const AppConfig::PersistentFeatureState& feature : persistentRestoreDialog.features) {
                persistentRestoreDialog.selected.push_back(feature.restoreNextLaunch);
            }
            persistentRestoreDialog.open = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(T("settings.persistentState.clear"), ImVec2(110.0f, 0.0f))) {
            AppConfig::ClearPersistentRestoreSelection();
        }

        DrawPersistentStatePopup();
    }

    void DrawPersistentStatePopup() {
        if (persistentRestoreDialog.open) {
            ImGui::OpenPopup(PersistentStatePopupId);
            persistentRestoreDialog.open = false;
        }

        ImGui::SetNextWindowSize(ImVec2(720.0f, 520.0f), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal(PersistentStatePopupId, nullptr, ImGuiWindowFlags_NoResize)) {
            ImGui::TextUnformatted(T("settings.persistentState.title"));
            ImGui::TextWrapped("%s", T("settings.persistentState.description"));
            ImGui::Spacing();

            int selectedCount = 0;
            int enabledCount = 0;
            for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                if (persistentRestoreDialog.features[i].enabledNow) {
                    ++enabledCount;
                }
                if (i < persistentRestoreDialog.selected.size() && persistentRestoreDialog.selected[i]) {
                    ++selectedCount;
                }
            }
            ImGui::TextDisabled(T("settings.persistentState.dialogCounts"), enabledCount, selectedCount, static_cast<int>(persistentRestoreDialog.features.size()));

            if (ImGui::Button(T("settings.persistentState.selectEnabled"), ImVec2(150.0f, 0.0f))) {
                for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                    persistentRestoreDialog.selected[i] = persistentRestoreDialog.features[i].enabledNow;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(T("settings.persistentState.selectNone"), ImVec2(120.0f, 0.0f))) {
                for (std::size_t i = 0; i < persistentRestoreDialog.selected.size(); ++i) {
                    persistentRestoreDialog.selected[i] = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(T("settings.persistentState.selectOverlay"), ImVec2(150.0f, 0.0f))) {
                for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                    persistentRestoreDialog.selected[i] = persistentRestoreDialog.features[i].group == "overlay";
                }
            }

            ImGui::Spacing();
            const float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 3.0f;
            const float tableHeight = ImGui::GetContentRegionAvail().y - footerHeight;
            if (persistentRestoreDialog.features.empty()) {
                ImGui::BeginChild("PersistentRestoreEmpty", ImVec2(0.0f, tableHeight), true);
                ImGui::TextDisabled("%s", T("settings.persistentState.noneAvailable"));
                ImGui::EndChild();
            } else if (ImGui::BeginTable("PersistentRestoreTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, ImVec2(0.0f, tableHeight))) {
                ImGui::TableSetupColumn(T("settings.feature"), ImGuiTableColumnFlags_WidthStretch, 0.55f);
                ImGui::TableSetupColumn(T("settings.persistentState.currentState"), ImGuiTableColumnFlags_WidthStretch, 0.22f);
                ImGui::TableSetupColumn(T("settings.persistentState.restoreNextLaunch"), ImGuiTableColumnFlags_WidthStretch, 0.23f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                std::string currentGroup;
                for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                    const AppConfig::PersistentFeatureState& feature = persistentRestoreDialog.features[i];
                    if (feature.group != currentGroup) {
                        currentGroup = feature.group;
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("%s", T((std::string("settings.persistentState.group.") + currentGroup).c_str()));
                        ImGui::TableNextColumn();
                        ImGui::TableNextColumn();
                    }

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(T(feature.name.c_str()));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(T(feature.enabledNow ? "settings.enabled" : "settings.disabled"));

                    ImGui::TableNextColumn();
                    char checkboxId[128] = {};
                    bool selected = persistentRestoreDialog.selected[i];
                    std::snprintf(checkboxId, sizeof(checkboxId), "##restore_%s", feature.id.c_str());
                    if (ImGui::Checkbox(checkboxId, &selected)) {
                        persistentRestoreDialog.selected[i] = selected;
                    }
                }

                ImGui::EndTable();
            }

            UI::SpacingSeparator();
            if (ImGui::Button(T("settings.persistentState.apply"), ImVec2(110.0f, 0.0f))) {
                std::vector<std::string> selectedIds;
                for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                    if (persistentRestoreDialog.selected[i]) {
                        selectedIds.push_back(persistentRestoreDialog.features[i].id);
                    }
                }
                AppConfig::ApplyPersistentRestoreSelection(selectedIds);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(T("settings.persistentState.cancel"), ImVec2(110.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    struct ActionHotkeyInputState {
        std::array<char, 32> value{};
        std::string lastSavedValue;
    };

    std::unordered_map<std::string, ActionHotkeyInputState> actionHotkeyInputs;

    ActionHotkeyInputState& ActionHotkeyInput(const AppConfig::ActionHotkey& action) {
        ActionHotkeyInputState& state = actionHotkeyInputs[action.id];
        const std::string current = AppConfig::FormatHotkey(action.hotkey);
        if (state.value[0] == '\0' || state.lastSavedValue != current) {
            std::snprintf(state.value.data(), state.value.size(), "%s", current.c_str());
            state.lastSavedValue = current;
        }
        return state;
    }

    std::string pendingCaptureActionId;
    std::string pendingCaptureActionName;
    std::string pendingCaptureValue;
    bool openActionHotkeyCapturePopup = false;
    ULONGLONG actionHotkeyCaptureReadyTick = 0;
    const char* ActionHotkeyCapturePopupId = "ActionHotkeyCapturePopup";

    const char* VirtualKeyName(int key) {
        if (key >= 'A' && key <= 'Z') {
            static char value[2] = {};
            value[0] = static_cast<char>(key);
            value[1] = '\0';
            return value;
        }
        if (key >= '0' && key <= '9') {
            static char value[2] = {};
            value[0] = static_cast<char>(key);
            value[1] = '\0';
            return value;
        }

        switch (key) {
        case VK_BACK: return "BACKSPACE";
        case VK_TAB: return "TAB";
        case VK_RETURN: return "ENTER";
        case VK_ESCAPE: return "ESC";
        case VK_SPACE: return "SPACE";
        case VK_PRIOR: return "PAGEUP";
        case VK_NEXT: return "PAGEDOWN";
        case VK_END: return "END";
        case VK_HOME: return "HOME";
        case VK_LEFT: return "LEFT";
        case VK_UP: return "UP";
        case VK_RIGHT: return "RIGHT";
        case VK_DOWN: return "DOWN";
        case VK_INSERT: return "INSERT";
        case VK_DELETE: return "DELETE";
        case VK_NUMPAD0: return "NUM0";
        case VK_NUMPAD1: return "NUM1";
        case VK_NUMPAD2: return "NUM2";
        case VK_NUMPAD3: return "NUM3";
        case VK_NUMPAD4: return "NUM4";
        case VK_NUMPAD5: return "NUM5";
        case VK_NUMPAD6: return "NUM6";
        case VK_NUMPAD7: return "NUM7";
        case VK_NUMPAD8: return "NUM8";
        case VK_NUMPAD9: return "NUM9";
        case VK_MULTIPLY: return "MULTIPLY";
        case VK_ADD: return "ADD";
        case VK_SUBTRACT: return "SUBTRACT";
        case VK_DECIMAL: return "DECIMAL";
        case VK_DIVIDE: return "DIVIDE";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        default: break;
        }

        static char fallback[16] = {};
        std::snprintf(fallback, sizeof(fallback), "VK_%d", key);
        return fallback;
    }

    bool IsModifierKey(int key) {
        return key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL
            || key == VK_MENU || key == VK_LMENU || key == VK_RMENU
            || key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT;
    }

    bool IsIgnoredCaptureKey(int key) {
        return key == VK_LBUTTON || key == VK_RBUTTON || key == VK_CANCEL || key == VK_MBUTTON
            || key == VK_XBUTTON1 || key == VK_XBUTTON2;
    }

    bool CapturePressedHotkey(std::string& value) {
        if (GetTickCount64() < actionHotkeyCaptureReadyTick) {
            return false;
        }

        if ((GetAsyncKeyState(VK_BACK) & 0x0001) != 0 || (GetAsyncKeyState(VK_DELETE) & 0x0001) != 0) {
            value = "None";
            return true;
        }

        for (int key = 1; key < 255; ++key) {
            if (IsModifierKey(key) || IsIgnoredCaptureKey(key)) {
                continue;
            }
            if ((GetAsyncKeyState(key) & 0x0001) == 0) {
                continue;
            }

            std::string result;
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 || (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0 || (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0) {
                result += "Ctrl+";
            }
            if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0 || (GetAsyncKeyState(VK_LMENU) & 0x8000) != 0 || (GetAsyncKeyState(VK_RMENU) & 0x8000) != 0) {
                result += "Alt+";
            }
            if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0 || (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0 || (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0) {
                result += "Shift+";
            }
            result += VirtualKeyName(key);
            value = result;
            return true;
        }

        return false;
    }

    void OpenActionHotkeyCapturePopup(const AppConfig::ActionHotkey& action) {
        pendingCaptureActionId = action.id;
        pendingCaptureActionName = action.name;
        pendingCaptureValue = AppConfig::FormatHotkey(action.hotkey);
        actionHotkeyCaptureReadyTick = GetTickCount64() + 250;
        openActionHotkeyCapturePopup = true;
    }

    void DrawActionHotkeyCapturePopup() {
        if (openActionHotkeyCapturePopup) {
            ImGui::OpenPopup(ActionHotkeyCapturePopupId);
            openActionHotkeyCapturePopup = false;
        }

        if (ImGui::BeginPopupModal(ActionHotkeyCapturePopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(T("settings.hotkeyCaptureTitle"));
            ImGui::Separator();
            ImGui::Text(T("settings.hotkeyCaptureAction"), T(pendingCaptureActionName.c_str()));
            ImGui::Text(T("settings.hotkeyCaptureCurrent"), pendingCaptureValue.c_str());
            ImGui::TextDisabled("%s", T("settings.hotkeyCaptureHint"));

            std::string captured;
            if (CapturePressedHotkey(captured)) {
                pendingCaptureValue = captured;
            }

            ImGui::Spacing();
            if (ImGui::Button(T("settings.apply"), ImVec2(120.0f, 0.0f))) {
                if (!pendingCaptureActionId.empty() && !pendingCaptureValue.empty()) {
                    AppConfig::SetActionHotkeyName(pendingCaptureActionId, pendingCaptureValue);
                    ActionHotkeyInputState& input = actionHotkeyInputs[pendingCaptureActionId];
                    const std::string current = AppConfig::GetActionHotkeyName(pendingCaptureActionId);
                    std::snprintf(input.value.data(), input.value.size(), "%s", current.c_str());
                    input.lastSavedValue = current;
                }
                pendingCaptureActionId.clear();
                pendingCaptureActionName.clear();
                pendingCaptureValue.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(T("settings.cancel"), ImVec2(120.0f, 0.0f))) {
                pendingCaptureActionId.clear();
                pendingCaptureActionName.clear();
                pendingCaptureValue.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void DrawActionHotkeys() {
        ImGui::TextUnformatted(T("settings.actionHotkeys"));
        ImGui::TextDisabled("%s", T("settings.hotkeyHint"));

        const std::vector<AppConfig::ActionHotkey>& actions = AppConfig::GetActionHotkeys();
        if (ImGui::BeginTable("ActionHotkeyTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(T("settings.action"));
            ImGui::TableSetupColumn(T("settings.hotkey"));
            ImGui::TableSetupColumn(T("settings.capture"));
            ImGui::TableHeadersRow();

            for (const AppConfig::ActionHotkey& action : actions) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(T(action.name.c_str()));

                ImGui::TableNextColumn();
                ActionHotkeyInputState& input = ActionHotkeyInput(action);
                ImGui::TextUnformatted(input.value.data());

                ImGui::TableNextColumn();
                char buttonId[96] = {};
                std::snprintf(buttonId, sizeof(buttonId), "%s##capture_%s", T("settings.setHotkey"), action.id.c_str());
                if (ImGui::SmallButton(buttonId)) {
                    OpenActionHotkeyCapturePopup(action);
                }
            }

            ImGui::EndTable();
        }

        DrawActionHotkeyCapturePopup();
    }

    void DrawConfigSettings() {
        ImGui::TextUnformatted(T("settings.config"));
        ImGui::TextDisabled(T("settings.defaultConfigPath"), AppConfig::GetConfigPath().c_str());
        ImGui::TextUnformatted(T("settings.transferScope"));
        ImGui::RadioButton(T("settings.transferAll"), &configTransferScope, 0);
        ImGui::SameLine();
        ImGui::RadioButton(T("settings.transferCustomLocations"), &configTransferScope, 1);

        const AppConfig::TransferScope scope = configTransferScope == 1
            ? AppConfig::TransferScope::CustomLocations
            : AppConfig::TransferScope::All;

        if (ImGui::Button(T("settings.importConfig"), ImVec2(130.0f, 0.0f))) {
            configImportText[0] = '\0';
            openConfigImportPopup = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(T("settings.exportConfig"), ImVec2(130.0f, 0.0f))) {
            const std::string exported = AppConfig::ExportToText(scope);
            std::snprintf(configExportText, sizeof(configExportText), "%s", exported.c_str());
            if (exported.empty()) {
                std::snprintf(configStatus, sizeof(configStatus), "%s", T("settings.exportFailed"));
            } else {
                std::snprintf(configStatus, sizeof(configStatus), "%s", configTransferScope == 1 ? T("settings.exportPartialSuccess") : T("settings.exportSuccess"));
                openConfigExportPopup = true;
            }
        }

        if (configStatus[0] != '\0') {
            ImGui::TextDisabled("%s", configStatus);
        }
    }

    void DrawConfigImportPopup() {
        char popupTitle[128] = {};
        std::snprintf(popupTitle, sizeof(popupTitle), "%s###%s", T("settings.importTextTitle"), ConfigImportPopupId);

        if (openConfigImportPopup) {
            ImGui::OpenPopup(popupTitle);
            openConfigImportPopup = false;
        }

        const AppConfig::TransferScope scope = configTransferScope == 1
            ? AppConfig::TransferScope::CustomLocations
            : AppConfig::TransferScope::All;

        if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(T("settings.importTextTitle"));
            ImGui::TextUnformatted(T("settings.importTextHint"));
            ImGui::InputTextMultiline("##ConfigImportText", configImportText, sizeof(configImportText), ImVec2(620.0f, 300.0f));
            if (ImGui::Button(T("settings.importTextApply"), ImVec2(130.0f, 0.0f))) {
                if (AppConfig::ImportFromText(configImportText, scope)) {
                    Resources::ReloadLocations();
                    std::snprintf(configStatus, sizeof(configStatus), "%s", configTransferScope == 1 ? T("settings.importPartialSuccess") : T("settings.importSuccess"));
                    ImGui::CloseCurrentPopup();
                } else {
                    std::snprintf(configStatus, sizeof(configStatus), "%s", T("settings.importFailed"));
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(T("settings.close"), ImVec2(130.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void DrawConfigExportPopup() {
        char popupTitle[128] = {};
        std::snprintf(popupTitle, sizeof(popupTitle), "%s###%s", T("settings.exportTextTitle"), ConfigExportPopupId);

        if (openConfigExportPopup) {
            ImGui::OpenPopup(popupTitle);
            openConfigExportPopup = false;
        }

        if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(T("settings.exportTextTitle"));
            ImGui::TextUnformatted(T("settings.exportTextHint"));
            ImGui::InputTextMultiline("##ConfigExportText", configExportText, sizeof(configExportText), ImVec2(620.0f, 300.0f), ImGuiInputTextFlags_ReadOnly);
            if (ImGui::Button(T("settings.copyText"), ImVec2(130.0f, 0.0f))) {
                ImGui::SetClipboardText(configExportText);
            }
            ImGui::SameLine();
            if (ImGui::Button(T("settings.close"), ImVec2(130.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    const char* VersionStatusText(UpdateChecker::VersionStatus status) {
        switch (status) {
        case UpdateChecker::VersionStatus::Equal: return T("status.upToDate");
        case UpdateChecker::VersionStatus::LocalNewer: return T("status.localNewer");
        case UpdateChecker::VersionStatus::RemoteNewer: return T("status.remoteNewer");
        case UpdateChecker::VersionStatus::Unknown:
        default: return T("status.remoteUnknown");
        }
    }

    ImVec4 VersionStatusColor(UpdateChecker::VersionStatus status) {
        if (status == UpdateChecker::VersionStatus::Equal) {
            return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        }
        if (status == UpdateChecker::VersionStatus::LocalNewer) {
            return ImVec4(1.0f, 0.30f, 0.25f, 1.0f);
        }
        if (status == UpdateChecker::VersionStatus::RemoteNewer) {
            return ImVec4(1.0f, 0.82f, 0.20f, 1.0f);
        }
        return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    }

    void DrawUpdateSettings() {
        const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
        const bool checking = UpdateChecker::IsChecking();
        const char* remoteVersion = info.latestVersion.empty() ? T("status.remoteUnknown") : info.latestVersion.c_str();
        const char* releaseUrl = info.releaseUrl.empty() ? XMENU_GITHUB : info.releaseUrl.c_str();
        const char* sourceText = info.sourceName.empty()
            ? UpdateChecker::SourceDisplayName(info.source)
            : info.sourceName.c_str();

        ImGui::TextUnformatted(T("update.details"));
        ImGui::PushStyleColor(ImGuiCol_Text, VersionStatusColor(info.status));
        ImGui::TextUnformatted(checking ? T("status.checking") : VersionStatusText(info.status));
        ImGui::PopStyleColor();
        ImGui::Text(T("status.localVersion"), XMENU_VERSION);
        ImGui::Text(T("status.remoteVersion"), remoteVersion);
        ImGui::Text(T("update.source"), checking ? T("status.checking") : sourceText);
        ImGui::TextDisabled("%s", T("update.sourceHint"));

        if (ImGui::Button(T("update.refresh"), ImVec2(130.0f, 0.0f))) {
            UpdateChecker::Refresh();
        }
        if (checking) {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", T("status.checking"));
        }

        ImGui::Spacing();
        if (UI::Button(T("update.openGTAMODX"), 2)) {
            ShellExecuteA(nullptr, "open", XMENU_URL, nullptr, nullptr, SW_SHOWNORMAL);
        }
        ImGui::SameLine();
        if (UI::Button(T("update.openGitHub"), 2)) {
            ShellExecuteA(nullptr, "open", releaseUrl, nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    void DrawDebugSettings() {
        ImGui::TextUnformatted(T("settings.debug"));
        ImGui::Text(T("settings.d3dHookStatus"), D3DHook::GetInitStatus());
        ImGui::TextDisabled("%s", D3DHook::IsInitialized() ? T("settings.d3dHookInitialized") : T("settings.d3dHookFailed"));

        if (ImGui::Button(T("update.debugShowDialog"), ImVec2(160.0f, 0.0f))) {
            UpdateChecker::ForceDebugUpdate();
        }
    }

    ImVec4 LogColor(const std::string& level) {
        if (level == "ERROR" || level == "CRASH") {
            return ImVec4(1.0f, 0.35f, 0.30f, 1.0f);
        }
        if (level == "WARN") {
            return ImVec4(1.0f, 0.78f, 0.25f, 1.0f);
        }
        return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    }

    void DrawLogViewer() {
        ImGui::TextUnformatted(T("log.title"));
        ImGui::SameLine();
        if (ImGui::SmallButton(T("log.copy"))) {
            ImGui::SetClipboardText(Log::GetText().c_str());
        }
        const std::vector<Log::Entry> entries = Log::GetEntries();
        const std::size_t totalCount = Log::GetTotalCount();
        ImGui::SameLine();
        ImGui::TextDisabled(T("log.counts"), totalCount, entries.size());

        const float height = ImGui::GetTextLineHeightWithSpacing() * 12.0f;
        ImGui::BeginChild("XMenuLogViewer", ImVec2(0.0f, height), true, ImGuiWindowFlags_HorizontalScrollbar);
        if (entries.empty()) {
            ImGui::TextDisabled("%s", T("log.empty"));
        } else {
            for (const Log::Entry& entry : entries) {
                ImGui::PushStyleColor(ImGuiCol_Text, LogColor(entry.level));
                ImGui::TextUnformatted(entry.line.c_str());
                ImGui::PopStyleColor();
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
    }

    void DrawAbout() {
        UI::TextCentered("XMenu");
        ImGui::TextWrapped(T("about.version"), XMENU_VERSION);
        ImGui::TextWrapped(T("about.author"), XMENU_AUTHOR);
        ImGui::TextWrapped(T("about.testing"), XMENU_AUTHOR_TEST);
        ImGui::TextWrapped(T("about.techStack"), XMENU_TECH_STACK);
        ImGui::TextWrapped(T("about.openSourceLibs"), XMENU_OPEN_SOURCE_LIBS);
        UI::SpacingSeparator();
        ImGui::TextWrapped("%s", T("about.notice1"));
        ImGui::TextWrapped("%s", T("about.notice2"));
        ImGui::Spacing();
        if (UI::Button(T("about.joinGroup"), 2)) {
            ShellExecuteA(nullptr, "open", XMENU_QQ_GROUP, nullptr, nullptr, SW_SHOWNORMAL);
        }
        ImGui::SameLine();
        if (UI::Button(T("about.projectPage"), 2)) {
            ShellExecuteA(nullptr, "open", XMENU_GITHUB, nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    void DrawUpdateDialog() {
        if (UpdateChecker::HasUpdate()) {
            ImGui::OpenPopup("XMenuUpdateDialog");
        }

        // p_open：右上角 X；点关闭/打开链接后 Dismiss，避免每帧再弹
        bool updateOpen = true;
        if (ImGui::BeginPopupModal("XMenuUpdateDialog", &updateOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
            const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
            const char* sourceText = info.sourceName.empty()
                ? UpdateChecker::SourceDisplayName(info.source)
                : info.sourceName.c_str();

            ImGui::TextUnformatted(T("update.availableTitle"));
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped(T("update.availableMessage"), info.currentVersion.c_str(), info.latestVersion.c_str());
            ImGui::Text(T("update.source"), sourceText);
            ImGui::Spacing();

            if (ImGui::Button(T("update.openGitHub"), ImVec2(120.0f, 0.0f))) {
                ShellExecuteA(nullptr, "open", info.releaseUrl.empty() ? XMENU_GITHUB : info.releaseUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                UpdateChecker::Dismiss();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(T("update.openGTAMODX"), ImVec2(120.0f, 0.0f))) {
                ShellExecuteA(nullptr, "open", XMENU_URL, nullptr, nullptr, SW_SHOWNORMAL);
                UpdateChecker::Dismiss();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(T("update.close"), ImVec2(120.0f, 0.0f))) {
                UpdateChecker::Dismiss();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        if (!updateOpen) {
            // 标题栏 X：Begin 可能仍为 true，统一在这里 Dismiss
            UpdateChecker::Dismiss();
        }
    }
}
void Menu::Process() {
    Pages::Player::Process();
    Pages::Vehicle::Process();
    Pages::Ped::Process();
    Pages::Weapon::Process();
    Pages::World::Process();
    if (IsSaRuntime()) {
        Pages::Scene::Process();
    }
    Pages::Visual::Process();
    Pages::Teleport::Process();
    Controllers::Overlay::Process();
    Controllers::Command::Process();
    Controllers::Hotkeys::Process();
    D3DHook::SetBackgroundRenderActive(
        MenuState::OverlayEnabled
        || MenuState::CommandWindowEnabled
        || MenuState::QuickTeleportMapActive
        || MenuState::WeaponPedEsp
        || MenuState::WeaponPedColEsp
        || MenuState::WeaponPedSkeleton
        || MenuState::WeaponVehicleEsp
        || MenuState::WeaponVehicleColEsp
        || MenuState::WeaponBulletTrack);
}

void Menu::Draw() {
    menuVisible = D3DHook::IsMenuVisible();
    char windowTitle[160] = {};

    char visibleTitle[128] = {};
    std::snprintf(visibleTitle, sizeof(visibleTitle), T("window.title"), XMENU_AUTHOR);
    std::snprintf(windowTitle, sizeof(windowTitle), "%s###XMenuMainWindow", visibleTitle);

    if (!menuVisible) {
        // 当菜单关闭时，如果有激活的页面（例如 Player 子菜单），我们重置为 Player 主菜单（或者根菜单）
        // 这样下次打开菜单时不会停留在深层子页面。也可以保留当前页面，取决于需求。
        // 对于 overlay 相关的渲染，不能被 return 截断
    }

    // Process controllers related to UI even when menu is hidden, e.g., overlay.
    // However, overlay rendering should happen inside D3DHook Present hook independently.
    // If the overlay is drawn from here, we need to draw it regardless of menuVisible.
    // if (!menuVisible) return;

    if (menuVisible) {
        GuiTheme::Sync();

        if (MenuState::UseNativeMenu) {
            const char* subtitle = "MAIN MENU";
            if (!pageStack.empty()) {
                switch (activePage) {
                    case Page::Player: subtitle = T("tab.player"); break;
                    case Page::Vehicle: subtitle = T("tab.vehicle"); break;
                    case Page::Ped: subtitle = T("tab.ped"); break;
                    case Page::Weapon: subtitle = T("tab.weapon"); break;
                    case Page::World: subtitle = T("tab.world"); break;
                    case Page::Scene: subtitle = T("tab.scene"); break;
                    case Page::Visual: subtitle = T("tab.visual"); break;
                    case Page::Teleport: subtitle = T("tab.teleport"); break;
                    case Page::Settings: subtitle = T("tab.settings"); break;
                    case Page::About: subtitle = T("tab.about"); break;
                    default: subtitle = "OPTIONS"; break;
                }
            }

            UI::BeginListShell(visibleTitle, subtitle, !pageStack.empty());

            if (pageStack.empty()) {
                for (const NavItem& item : navItems) {
                    if (!IsPageAvailable(item.page)) continue;
                    if (UI::Button(T(item.textKey))) {
                        PushPage(item.page);
                    }
                }
            } else {
                DrawActivePage();
            }

            UI::EndListShell();
        } else {
            ImGui::SetNextWindowSize(ImVec2(780.0f, 520.0f), ImGuiCond_FirstUseEver);

            bool windowOpen = true;
            if (ImGui::Begin(windowTitle, &windowOpen, ImGuiWindowFlags_NoCollapse)) {
                HandleMainWindowDrag();
                ImGui::BeginChild("XMenuSidebar", ImVec2(170.0f, 0.0f), true);
                DrawNavigation();
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("XMenuContent", ImVec2(0.0f, 0.0f), true);
                DrawActivePage();
                ImGui::EndChild();

                DrawUpdateDialog();
                DrawConfigImportPopup();
                DrawConfigExportPopup();
            }
            ImGui::End();

            if (!windowOpen) {
                D3DHook::SetMenuVisible(false);
            }
        }
    }

    Controllers::Overlay::Draw();
    Controllers::Command::Draw();
    Controllers::Teleport::DrawQuickMap();
    Controllers::BulletAssist::Draw();
}