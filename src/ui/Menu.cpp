#include "Menu.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <XBase/Runtime.h>
#include "utils/Log.h"
#include <XBase/Hooks.h>
#include <XBase/Input.h>
#include <XBase/Platform.h>
#include <XBase/UI.h>
#include "utils/I18n.h"
#include "utils/UpdateChecker.h"
#include "utils/AppConfig.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "ui/MenuState.h"
#include "ui/GuiTheme.h"
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
#include "controllers/Ped.h"
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
    XBase::UI::MenuSurfaceState listSurfaceState;
    bool listSurfaceResetPending = false;

    void PushPage(Page page) {
        pageStack.push_back(activePage);
        activePage = page;
        listSurfaceResetPending = true;
    }

    void PopPage() {
        if (!pageStack.empty()) {
            activePage = pageStack.back();
            pageStack.pop_back();
            listSurfaceResetPending = true;
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
        return XBase::Runtime::GetGameTarget() == XBase::Runtime::GameTarget::SanAndreas;
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

    void DrawPageHeader(const char* titleKey) {
        XBase::UI::Text(T(titleKey));
        XBase::UI::Separator();
        XBase::UI::Spacing();

        if (MenuState::HasNotice()) {
            XBase::UI::TextWrapped(MenuState::NoticeText);
            XBase::UI::Spacing();
            XBase::UI::Separator();
            XBase::UI::Spacing();
        }
    }

    void DrawVersionBadge() {
        const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
        const char* remoteVersion = info.latestVersion.empty() ? T("status.remoteUnknown") : info.latestVersion.c_str();
        const char* statusText = T("status.remoteUnknown");
        const char* sourceText = info.sourceName.empty()
            ? UpdateChecker::SourceDisplayName(info.source)
            : info.sourceName.c_str();

        if (UpdateChecker::IsChecking()) {
            statusText = T("status.checking");
        } else if (info.status == UpdateChecker::VersionStatus::Equal) {
            statusText = T("status.upToDate");
        } else if (info.status == UpdateChecker::VersionStatus::LocalNewer) {
            statusText = T("status.localNewer");
        } else if (info.status == UpdateChecker::VersionStatus::RemoteNewer) {
            statusText = T("status.remoteNewer");
        }

        std::string tooltip = statusText;
        tooltip += "\n";
        tooltip += T("status.localVersion");
        tooltip += ": ";
        tooltip += XMENU_VERSION;
        tooltip += "\n";
        tooltip += T("status.remoteVersion");
        tooltip += ": ";
        tooltip += remoteVersion;
        tooltip += "\n";
        tooltip += T("update.source");
        tooltip += ": ";
        tooltip += sourceText;

        XBase::UI::SameLine();
        XBase::UI::TextDisabled(XMENU_VERSION);
        XBase::UI::SameLine();
        XBase::UI::Button("?##VersionInfo", {20.0f, 0.0f});
        XBase::UI::Tooltip(tooltip.c_str());
    }

    void DrawNavigation() {
        XBase::UI::Text("XMenu");
        DrawVersionBadge();
        XBase::UI::Spacing();
        XBase::UI::Separator();
        XBase::UI::Spacing();

        EnsureActivePageAvailable();

        for (const NavItem& item : navItems) {
            if (!IsPageAvailable(item.page)) continue;
            char label[96] = {};
            LabelWithStableId(label, sizeof(label), item.textKey, item.id);
            if (XBase::UI::Selectable(label, activePage == item.page, {0.0f, 34.0f})) {
                activePage = item.page;
            }
        }

        XBase::UI::Separator();
        XBase::UI::TextDisabled(T("status.language"), I18n::GetLanguageName(I18n::GetLanguage()));
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
        namespace BaseUI = XBase::UI;
        BaseUI::Text(T("settings.interfaceLanguage"));
        BaseUI::Spacing();

        const std::vector<I18n::LanguageInfo>& languages = I18n::GetAvailableLanguages();
        const std::string currentLanguageCode = I18n::GetCurrentLanguageCode();
        BaseUI::Combo("##InterfaceLanguage", I18n::GetLanguageName(currentLanguageCode), [&] {
            for (const I18n::LanguageInfo& language : languages) {
                const bool selected = currentLanguageCode == language.code;
                if (BaseUI::Selectable(language.name.c_str(), selected)) {
                    I18n::SetLanguage(language.code);
                }
                if (selected) BaseUI::FocusLastItemByDefault();
            }
        });

        BaseUI::Spacing();
        BaseUI::Text(T("settings.fallbackLanguage"));
        const std::string fallbackLanguageCode = AppConfig::GetFallbackLanguageCode();
        BaseUI::Combo("##FallbackLanguage", I18n::GetLanguageName(fallbackLanguageCode), [&] {
            for (const I18n::LanguageInfo& language : languages) {
                const bool selected = fallbackLanguageCode == language.code;
                if (BaseUI::Selectable(language.name.c_str(), selected)) {
                    AppConfig::SetFallbackLanguageCode(language.code);
                }
                if (selected) BaseUI::FocusLastItemByDefault();
            }
        });
        BaseUI::TextDisabled(T("settings.fallbackLanguageHint"));

        BaseUI::Spacing();
        BaseUI::TextDisabled(T("settings.applyImmediately"));

        static char hotkeyInput[32] = "";
        if (hotkeyInput[0] == '\0') {
            std::snprintf(hotkeyInput, sizeof(hotkeyInput), "%s", AppConfig::GetMenuKeyName().c_str());
        }
        BaseUI::Spacing();
        BaseUI::InputText(T("settings.menuHotkey"), hotkeyInput, sizeof(hotkeyInput), "M");
        BaseUI::SameLine();
        if (BaseUI::Button(T("settings.applyHotkey"))) {
            AppConfig::SetMenuKeyName(hotkeyInput);
            std::snprintf(hotkeyInput, sizeof(hotkeyInput), "%s", AppConfig::GetMenuKeyName().c_str());
        }
        BaseUI::TextDisabled(T("settings.currentHotkey"), AppConfig::GetMenuKeyName().c_str());

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
        namespace BaseUI = XBase::UI;
        BaseUI::Text(T("settings.guiStyle"));
        BaseUI::Spacing();

        bool listChanged = UI::Checkbox(T("settings.useListMenu"), &MenuState::UseNativeMenu);
        if (MenuState::UseNativeMenu) {
            listChanged |= UI::Checkbox(T("settings.enableListMouse"), &MenuState::ListMenuMouseInput);
            BaseUI::TextDisabled(T("settings.listMouseHint"));
        }
        if (listChanged) {
            AppConfig::Save();
            GuiTheme::Sync();
            if (MenuState::UseNativeMenu) {
                listSurfaceState.selectedIndex = 0;
                listSurfaceState.itemCount = 0;
            }
        }

        BaseUI::Spacing();
        BaseUI::Separator();
        BaseUI::Spacing();
        BaseUI::Text(T("settings.theme"));
        const int currentTheme = GuiTheme::GetThemeIndex();
        BaseUI::Combo("##GuiTheme", T(GuiTheme::GetThemeNameKey(currentTheme)), [&] {
            for (int i = 0; i < GuiTheme::ThemeCount; ++i) {
                const bool selected = i == currentTheme;
                if (BaseUI::Selectable(T(GuiTheme::GetThemeNameKey(i)), selected)) {
                    AppConfig::SetGuiThemeIndex(i);
                    GuiTheme::Sync();
                }
                if (selected) BaseUI::FocusLastItemByDefault();
            }
        });

        if (!MenuState::UseNativeMenu) {
            BaseUI::Spacing();
            BaseUI::Text(T("settings.interactionMode"));
            const int currentInteraction = GuiTheme::GetInteractionIndex();
            BaseUI::Combo("##InteractionMode", T(GuiTheme::GetInteractionNameKey(currentInteraction)), [&] {
                for (int i = 0; i < GuiTheme::InteractionCount; ++i) {
                    const bool selected = i == currentInteraction;
                    if (BaseUI::Selectable(T(GuiTheme::GetInteractionNameKey(i)), selected)) {
                        AppConfig::SetInteractionMode(i);
                        GuiTheme::Sync();
                    }
                    if (selected) BaseUI::FocusLastItemByDefault();
                }
            });
            BaseUI::TextDisabled(T("settings.interactionHint"));
        } else {
            BaseUI::Spacing();
            BaseUI::TextDisabled(T("settings.listNavHint"));
        }
    }

    void DrawRuntimeSettings() {
        XBase::UI::Text(T("settings.runtime"));
        XBase::UI::Checkbox(T("command.enabled"), MenuState::CommandWindowEnabled);
    }

    void DrawOverlaySettings() {
        XBase::UI::Text(T("settings.overlay"));
        XBase::UI::TextDisabled(T("settings.overlayHint"));

        bool changed = false;
        changed |= XBase::UI::Checkbox(T("overlay.enabled"), MenuState::OverlayEnabled);
        XBase::UI::SameLine();
        changed |= XBase::UI::Checkbox(T("overlay.showDetails"), MenuState::OverlayShowDetails);
        XBase::UI::SameLine();
        changed |= XBase::UI::Checkbox(T("overlay.showFeatures"), MenuState::OverlayShowFeatures);

        XBase::UI::Columns(2, nullptr, false);
        changed |= XBase::UI::Checkbox(T("overlay.showPosition"), MenuState::OverlayShowPosition);
        XBase::UI::NextColumn();
        changed |= XBase::UI::Checkbox(T("overlay.showPlayer"), MenuState::OverlayShowPlayer);
        XBase::UI::NextColumn();
        changed |= XBase::UI::Checkbox(T("overlay.showVehicle"), MenuState::OverlayShowVehicle);
        XBase::UI::NextColumn();
        changed |= XBase::UI::Checkbox(T("overlay.showTime"), MenuState::OverlayShowTime);
        XBase::UI::NextColumn();
        changed |= XBase::UI::Checkbox(T("overlay.showWorld"), MenuState::OverlayShowWorld);
        XBase::UI::NextColumn();
        changed |= XBase::UI::Checkbox(T("overlay.showFps"), MenuState::OverlayShowFps);
        XBase::UI::Columns(1);

        if (changed) AppConfig::Save();
    }

    void DrawPersistentStateSettings() {
        namespace BaseUI = XBase::UI;
        BaseUI::Text(T("settings.persistentState"));
        BaseUI::TextDisabled(T("settings.persistentStateHint"));

        const std::size_t restoreCount = AppConfig::GetPersistentRestoreCount();
        BaseUI::TextDisabled(T("settings.persistentState.restoreCount"), static_cast<int>(restoreCount));

        if (BaseUI::Button(T("settings.persistentState.captureCurrent"), {190.0f, 0.0f})) {
            AppConfig::CaptureEnabledPersistentFeatures();
        }
        BaseUI::SameLine();
        if (BaseUI::Button(T("settings.persistentState.manage"), {140.0f, 0.0f})) {
            persistentRestoreDialog.features = AppConfig::GetPersistentFeatureStates();
            persistentRestoreDialog.selected.clear();
            persistentRestoreDialog.selected.reserve(persistentRestoreDialog.features.size());
            for (const AppConfig::PersistentFeatureState& feature : persistentRestoreDialog.features) {
                persistentRestoreDialog.selected.push_back(feature.restoreNextLaunch);
            }
            persistentRestoreDialog.open = true;
        }
        BaseUI::SameLine();
        if (BaseUI::Button(T("settings.persistentState.clear"), {110.0f, 0.0f})) {
            AppConfig::ClearPersistentRestoreSelection();
        }

        DrawPersistentStatePopup();
    }

    void DrawPersistentStatePopup() {
        namespace BaseUI = XBase::UI;
        if (persistentRestoreDialog.open) {
            BaseUI::OpenModal(PersistentStatePopupId);
            persistentRestoreDialog.open = false;
        }

        BaseUI::Modal(PersistentStatePopupId, [&] {
            BaseUI::Text(T("settings.persistentState.title"));
            BaseUI::TextWrapped(T("settings.persistentState.description"));
            BaseUI::Spacing();

            int selectedCount = 0;
            int enabledCount = 0;
            for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                if (persistentRestoreDialog.features[i].enabledNow) ++enabledCount;
                if (i < persistentRestoreDialog.selected.size() && persistentRestoreDialog.selected[i]) ++selectedCount;
            }
            BaseUI::TextDisabled(T("settings.persistentState.dialogCounts"), enabledCount, selectedCount,
                static_cast<int>(persistentRestoreDialog.features.size()));

            if (BaseUI::Button(T("settings.persistentState.selectEnabled"), {150.0f, 0.0f})) {
                for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                    persistentRestoreDialog.selected[i] = persistentRestoreDialog.features[i].enabledNow;
                }
            }
            BaseUI::SameLine();
            if (BaseUI::Button(T("settings.persistentState.selectNone"), {120.0f, 0.0f})) {
                for (std::size_t i = 0; i < persistentRestoreDialog.selected.size(); ++i) {
                    persistentRestoreDialog.selected[i] = false;
                }
            }
            BaseUI::SameLine();
            if (BaseUI::Button(T("settings.persistentState.selectOverlay"), {150.0f, 0.0f})) {
                for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                    persistentRestoreDialog.selected[i] = persistentRestoreDialog.features[i].group == "overlay";
                }
            }

            BaseUI::Spacing();
            const float tableHeight = BaseUI::GetContentAvailable().y - 70.0f;
            if (persistentRestoreDialog.features.empty()) {
                BaseUI::Child("PersistentRestoreEmpty", [&] {
                    BaseUI::TextDisabled(T("settings.persistentState.noneAvailable"));
                }, {0.0f, tableHeight}, true);
            } else {
                const BaseUI::TableColumn columns[] = {
                    {T("settings.feature"), 0.55f},
                    {T("settings.persistentState.currentState"), 0.22f},
                    {T("settings.persistentState.restoreNextLaunch"), 0.23f}
                };
                BaseUI::Table("PersistentRestoreTable", columns, 3, [&] {
                    std::string currentGroup;
                    for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                        const AppConfig::PersistentFeatureState& feature = persistentRestoreDialog.features[i];
                        if (feature.group != currentGroup) {
                            currentGroup = feature.group;
                            BaseUI::TableNextRow();
                            BaseUI::TableNextCell();
                            BaseUI::TextDisabled(T((std::string("settings.persistentState.group.") + currentGroup).c_str()));
                            BaseUI::TableNextCell();
                            BaseUI::TableNextCell();
                        }

                        BaseUI::TableNextRow();
                        BaseUI::TableNextCell();
                        BaseUI::Text(T(feature.name.c_str()));
                        BaseUI::TableNextCell();
                        BaseUI::Text(T(feature.enabledNow ? "settings.enabled" : "settings.disabled"));
                        BaseUI::TableNextCell();
                        char checkboxId[128] = {};
                        bool selected = persistentRestoreDialog.selected[i];
                        std::snprintf(checkboxId, sizeof(checkboxId), "##restore_%s", feature.id.c_str());
                        if (BaseUI::Checkbox(checkboxId, selected)) persistentRestoreDialog.selected[i] = selected;
                    }
                }, {0.0f, tableHeight});
            }

            UI::SpacingSeparator();
            if (BaseUI::Button(T("settings.persistentState.apply"), {110.0f, 0.0f})) {
                std::vector<std::string> selectedIds;
                for (std::size_t i = 0; i < persistentRestoreDialog.features.size(); ++i) {
                    if (persistentRestoreDialog.selected[i]) selectedIds.push_back(persistentRestoreDialog.features[i].id);
                }
                AppConfig::ApplyPersistentRestoreSelection(selectedIds);
                BaseUI::CloseModal();
            }
            BaseUI::SameLine();
            if (BaseUI::Button(T("settings.persistentState.cancel"), {110.0f, 0.0f})) {
                BaseUI::CloseModal();
            }
        }, {720.0f, 520.0f});
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
    std::uint64_t actionHotkeyCaptureReadyTick = 0;
    const char* ActionHotkeyCapturePopupId = "ActionHotkeyCapturePopup";

    bool CapturePressedHotkey(std::string& value) {
        if (XBase::Platform::MonotonicMilliseconds() < actionHotkeyCaptureReadyTick) return false;

        XBase::Input::Hotkey hotkey;
        if (!XBase::Input::CapturePressedHotkey(hotkey)) return false;
        value = XBase::Input::FormatHotkey(hotkey);
        return true;
    }

    void OpenActionHotkeyCapturePopup(const AppConfig::ActionHotkey& action) {
        pendingCaptureActionId = action.id;
        pendingCaptureActionName = action.name;
        pendingCaptureValue = AppConfig::FormatHotkey(action.hotkey);
        actionHotkeyCaptureReadyTick = XBase::Platform::MonotonicMilliseconds() + 250;
        openActionHotkeyCapturePopup = true;
    }

    void DrawActionHotkeyCapturePopup() {
        namespace BaseUI = XBase::UI;
        if (openActionHotkeyCapturePopup) {
            BaseUI::OpenModal(ActionHotkeyCapturePopupId);
            openActionHotkeyCapturePopup = false;
        }

        BaseUI::Modal(ActionHotkeyCapturePopupId, [&] {
            BaseUI::Text(T("settings.hotkeyCaptureTitle"));
            BaseUI::Separator();
            BaseUI::Text(T("settings.hotkeyCaptureAction"), T(pendingCaptureActionName.c_str()));
            BaseUI::Text(T("settings.hotkeyCaptureCurrent"), pendingCaptureValue.c_str());
            BaseUI::TextDisabled(T("settings.hotkeyCaptureHint"));

            std::string captured;
            if (CapturePressedHotkey(captured)) pendingCaptureValue = captured;

            BaseUI::Spacing();
            if (BaseUI::Button(T("settings.apply"), {120.0f, 0.0f})) {
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
                BaseUI::CloseModal();
            }
            BaseUI::SameLine();
            if (BaseUI::Button(T("settings.cancel"), {120.0f, 0.0f})) {
                pendingCaptureActionId.clear();
                pendingCaptureActionName.clear();
                pendingCaptureValue.clear();
                BaseUI::CloseModal();
            }
        }, {}, true);
    }

    void DrawActionHotkeys() {
        namespace BaseUI = XBase::UI;
        BaseUI::Text(T("settings.actionHotkeys"));
        BaseUI::TextDisabled(T("settings.hotkeyHint"));

        const std::vector<AppConfig::ActionHotkey>& actions = AppConfig::GetActionHotkeys();
        const BaseUI::TableColumn columns[] = {
            {T("settings.action"), 1.0f},
            {T("settings.hotkey"), 1.0f},
            {T("settings.capture"), 1.0f}
        };
        BaseUI::Table("ActionHotkeyTable", columns, 3, [&] {
            for (const AppConfig::ActionHotkey& action : actions) {
                BaseUI::TableNextRow();
                BaseUI::TableNextCell();
                BaseUI::Text(T(action.name.c_str()));

                BaseUI::TableNextCell();
                ActionHotkeyInputState& input = ActionHotkeyInput(action);
                BaseUI::Text(input.value.data());

                BaseUI::TableNextCell();
                char buttonId[96] = {};
                std::snprintf(buttonId, sizeof(buttonId), "%s##capture_%s", T("settings.setHotkey"), action.id.c_str());
                if (BaseUI::Button(buttonId)) OpenActionHotkeyCapturePopup(action);
            }
        });

        DrawActionHotkeyCapturePopup();
    }

    void DrawConfigSettings() {
        namespace BaseUI = XBase::UI;
        BaseUI::Text(T("settings.config"));
        BaseUI::TextDisabled(T("settings.defaultConfigPath"), AppConfig::GetConfigPath().c_str());
        BaseUI::Text(T("settings.transferScope"));
        BaseUI::Choice(T("settings.transferAll"), configTransferScope, 0);
        BaseUI::SameLine();
        BaseUI::Choice(T("settings.transferCustomLocations"), configTransferScope, 1);

        const AppConfig::TransferScope scope = configTransferScope == 1
            ? AppConfig::TransferScope::CustomLocations
            : AppConfig::TransferScope::All;

        if (BaseUI::Button(T("settings.importConfig"), {130.0f, 0.0f})) {
            configImportText[0] = '\0';
            openConfigImportPopup = true;
        }
        BaseUI::SameLine();
        if (BaseUI::Button(T("settings.exportConfig"), {130.0f, 0.0f})) {
            const std::string exported = AppConfig::ExportToText(scope);
            std::snprintf(configExportText, sizeof(configExportText), "%s", exported.c_str());
            if (exported.empty()) {
                std::snprintf(configStatus, sizeof(configStatus), "%s", T("settings.exportFailed"));
            } else {
                std::snprintf(configStatus, sizeof(configStatus), "%s",
                    configTransferScope == 1 ? T("settings.exportPartialSuccess") : T("settings.exportSuccess"));
                openConfigExportPopup = true;
            }
        }

        if (configStatus[0] != '\0') BaseUI::TextDisabled(configStatus);
    }

    void DrawConfigImportPopup() {
        namespace BaseUI = XBase::UI;
        char popupTitle[128] = {};
        std::snprintf(popupTitle, sizeof(popupTitle), "%s###%s", T("settings.importTextTitle"), ConfigImportPopupId);

        if (openConfigImportPopup) {
            BaseUI::OpenModal(popupTitle);
            openConfigImportPopup = false;
        }

        const AppConfig::TransferScope scope = configTransferScope == 1
            ? AppConfig::TransferScope::CustomLocations
            : AppConfig::TransferScope::All;

        BaseUI::Modal(popupTitle, [&] {
            BaseUI::Text(T("settings.importTextTitle"));
            BaseUI::Text(T("settings.importTextHint"));
            BaseUI::InputTextMultiline("##ConfigImportText", configImportText, sizeof(configImportText), {620.0f, 300.0f});
            if (BaseUI::Button(T("settings.importTextApply"), {130.0f, 0.0f})) {
                if (AppConfig::ImportFromText(configImportText, scope)) {
                    Resources::ReloadLocations();
                    std::snprintf(configStatus, sizeof(configStatus), "%s",
                        configTransferScope == 1 ? T("settings.importPartialSuccess") : T("settings.importSuccess"));
                    BaseUI::CloseModal();
                } else {
                    std::snprintf(configStatus, sizeof(configStatus), "%s", T("settings.importFailed"));
                }
            }
            BaseUI::SameLine();
            if (BaseUI::Button(T("settings.close"), {130.0f, 0.0f})) BaseUI::CloseModal();
        }, {}, true);
    }

    void DrawConfigExportPopup() {
        namespace BaseUI = XBase::UI;
        char popupTitle[128] = {};
        std::snprintf(popupTitle, sizeof(popupTitle), "%s###%s", T("settings.exportTextTitle"), ConfigExportPopupId);

        if (openConfigExportPopup) {
            BaseUI::OpenModal(popupTitle);
            openConfigExportPopup = false;
        }

        BaseUI::Modal(popupTitle, [&] {
            BaseUI::Text(T("settings.exportTextTitle"));
            BaseUI::Text(T("settings.exportTextHint"));
            BaseUI::InputTextMultiline("##ConfigExportText", configExportText, sizeof(configExportText), {620.0f, 300.0f}, true);
            if (BaseUI::Button(T("settings.copyText"), {130.0f, 0.0f})) {
                BaseUI::SetClipboardText(configExportText);
            }
            BaseUI::SameLine();
            if (BaseUI::Button(T("settings.close"), {130.0f, 0.0f})) BaseUI::CloseModal();
        }, {}, true);
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

    void DrawUpdateSettings() {
        const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
        const bool checking = UpdateChecker::IsChecking();
        const char* remoteVersion = info.latestVersion.empty() ? T("status.remoteUnknown") : info.latestVersion.c_str();
        const char* releaseUrl = info.releaseUrl.empty() ? XMENU_GITHUB : info.releaseUrl.c_str();
        const char* sourceText = info.sourceName.empty()
            ? UpdateChecker::SourceDisplayName(info.source)
            : info.sourceName.c_str();

        XBase::UI::Text(T("update.details"));
        XBase::UI::Text(checking ? T("status.checking") : VersionStatusText(info.status));
        XBase::UI::Text(T("status.localVersion"), XMENU_VERSION);
        XBase::UI::Text(T("status.remoteVersion"), remoteVersion);
        XBase::UI::Text(T("update.source"), checking ? T("status.checking") : sourceText);
        XBase::UI::TextDisabled(T("update.sourceHint"));

        if (XBase::UI::Button(T("update.refresh"), {130.0f, 0.0f})) UpdateChecker::Refresh();
        if (checking) {
            XBase::UI::SameLine();
            XBase::UI::TextDisabled(T("status.checking"));
        }
        XBase::UI::Spacing();
        if (XBase::UI::Button(T("update.openGTAMODX"), {130.0f, 0.0f})) XBase::Platform::OpenExternal(XMENU_URL);
        XBase::UI::SameLine();
        if (XBase::UI::Button(T("update.openGitHub"), {130.0f, 0.0f})) XBase::Platform::OpenExternal(releaseUrl);
    }

    void DrawDebugSettings() {
        XBase::UI::Text(T("settings.debug"));
        XBase::UI::Text(T("settings.renderBackendStatus"), XBase::Hooks::GetStatusText());
        XBase::UI::TextDisabled(XBase::Hooks::IsInitialized() ? T("settings.renderBackendInitialized") : T("settings.renderBackendFailed"));
        if (XBase::UI::Button(T("update.debugShowDialog"), {160.0f, 0.0f})) UpdateChecker::ForceDebugUpdate();
    }

    void DrawLogViewer() {
        XBase::UI::Text(T("log.title"));
        XBase::UI::SameLine();
        if (XBase::UI::Button(T("log.copy"))) XBase::UI::SetClipboardText(Log::GetText().c_str());
        const std::vector<Log::Entry> entries = Log::GetEntries();
        XBase::UI::SameLine();
        XBase::UI::TextDisabled(T("log.counts"), static_cast<int>(Log::GetTotalCount()), static_cast<int>(entries.size()));
        XBase::UI::Child("XMenuLogViewer", [&] {
            if (entries.empty()) {
                XBase::UI::TextDisabled(T("log.empty"));
            } else {
                for (const Log::Entry& entry : entries) XBase::UI::Text(entry.line.c_str());
            }
        }, {0.0f, 240.0f}, true);
    }

    void DrawAbout() {
        XBase::UI::CenterText("XMenu");
        XBase::UI::TextWrapped(T("about.version"), XMENU_VERSION);
        XBase::UI::TextWrapped(T("about.author"), XMENU_AUTHOR);
        XBase::UI::TextWrapped(T("about.testing"), XMENU_AUTHOR_TEST);
        XBase::UI::TextWrapped(T("about.techStack"), XMENU_TECH_STACK);
        XBase::UI::TextWrapped(T("about.openSourceLibs"), XMENU_OPEN_SOURCE_LIBS);
        UI::SpacingSeparator();
        XBase::UI::TextWrapped(T("about.notice1"));
        XBase::UI::TextWrapped(T("about.notice2"));
        XBase::UI::Spacing();
        if (XBase::UI::Button(T("about.joinGroup"), {130.0f, 0.0f})) XBase::Platform::OpenExternal(XMENU_QQ_GROUP);
        XBase::UI::SameLine();
        if (XBase::UI::Button(T("about.projectPage"), {130.0f, 0.0f})) XBase::Platform::OpenExternal(XMENU_GITHUB);
    }

    void DrawUpdateDialog() {
        if (UpdateChecker::HasUpdate()) {
            XBase::UI::OpenModal("XMenuUpdateDialog");
        }

        XBase::UI::Modal("XMenuUpdateDialog", [&] {
            const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
            const char* sourceText = info.sourceName.empty()
                ? UpdateChecker::SourceDisplayName(info.source)
                : info.sourceName.c_str();
            XBase::UI::Text(T("update.availableTitle"));
            XBase::UI::Separator();
            XBase::UI::Spacing();
            XBase::UI::TextWrapped(T("update.availableMessage"), info.currentVersion.c_str(), info.latestVersion.c_str());
            XBase::UI::Text(T("update.source"), sourceText);
            XBase::UI::Spacing();
            if (XBase::UI::Button(T("update.openGitHub"), {120.0f, 0.0f})) {
                XBase::Platform::OpenExternal(info.releaseUrl.empty() ? XMENU_GITHUB : info.releaseUrl.c_str());
                UpdateChecker::Dismiss();
                XBase::UI::CloseModal();
            }
            XBase::UI::SameLine();
            if (XBase::UI::Button(T("update.openGTAMODX"), {120.0f, 0.0f})) {
                XBase::Platform::OpenExternal(XMENU_URL);
                UpdateChecker::Dismiss();
                XBase::UI::CloseModal();
            }
            XBase::UI::SameLine();
            if (XBase::UI::Button(T("update.close"), {120.0f, 0.0f})) {
                UpdateChecker::Dismiss();
                XBase::UI::CloseModal();
            }
        }, {}, true);
    }
}
void Menu::Process() {
    Controllers::World::ProcessHost();
    Pages::Player::Process();
    Pages::Vehicle::Process();
    Pages::Weapon::Process();
    Pages::Visual::Process();
    Controllers::Ped::Process();
    Controllers::Teleport::ProcessHost();
    Controllers::Command::Process();
    Controllers::Hotkeys::Process();
    Controllers::BulletAssist::Process();
    XBase::Hooks::SetBackgroundRenderActive(
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
    menuVisible = XBase::Hooks::IsMenuVisible();
    char windowTitle[160] = {};

    char visibleTitle[128] = {};
    std::snprintf(visibleTitle, sizeof(visibleTitle), T("window.title"), XMENU_AUTHOR);
    std::snprintf(windowTitle, sizeof(windowTitle), "%s###XMenuMainWindow", visibleTitle);

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

            const XBase::UI::MenuSurfaceResult surfaceResult = XBase::UI::MenuSurface(
                "XMenuMainWindow",
                visibleTitle,
                subtitle,
                listSurfaceState,
                MenuState::ListMenuMouseInput,
                !pageStack.empty(),
                [&] {
                    if (pageStack.empty()) {
                        for (const NavItem& item : navItems) {
                            if (!IsPageAvailable(item.page)) continue;
                            if (UI::Button(T(item.textKey))) PushPage(item.page);
                        }
                    } else {
                        DrawActivePage();
                    }
                });
            if (surfaceResult.backRequested && !pageStack.empty()) PopPage();
            if (listSurfaceResetPending) {
                listSurfaceState.selectedIndex = 0;
                listSurfaceState.itemCount = 0;
                listSurfaceResetPending = false;
            }
        } else {
            XBase::UI::SetNextWindowSize({780.0f, 520.0f}, true);
            bool windowOpen = true;
            XBase::UI::Window("XMenuMainWindow", windowTitle, [&] {
                XBase::UI::Child("XMenuSidebar", [&] { DrawNavigation(); }, {170.0f, 0.0f}, true);
                XBase::UI::SameLine();
                XBase::UI::Child("XMenuContent", [&] { DrawActivePage(); }, {0.0f, 0.0f}, true);
                DrawUpdateDialog();
                DrawConfigImportPopup();
                DrawConfigExportPopup();
            }, &windowOpen, XBase::UI::Flag(XBase::UI::WindowFlag::NoCollapse));
            if (!windowOpen) XBase::Hooks::SetMenuVisible(false);
        }
    }

    Controllers::Overlay::Draw();
    Controllers::Command::Draw();
    Controllers::Teleport::DrawQuickMap();
    Controllers::BulletAssist::Draw();
}