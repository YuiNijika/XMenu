#include "Menu.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "utils/Log.h"
#include "utils/D3DHook.h"
#include "utils/I18n.h"
#include "utils/UpdateChecker.h"
#include "utils/AppConfig.h"
#include "resources/ResourceData.h"
#include "ui/Widget.h"
#include "imgui/imgui.h"
#include "ui/pages/Player.h"
#include "ui/pages/Vehicle.h"
#include "ui/pages/Teleport.h"
#include "ui/pages/Weapon.h"
#include "ui/pages/World.h"

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
        Weapon,
        World,
        Teleport,
        Settings,
        About
    };

    struct NavItem {
        Page page;
        const char* textKey;
        const char* id;
    };

    Page activePage = Page::Player;
    int configTransferScope = 1;
    char configImportText[65536] = "";
    char configExportText[65536] = "";
    char configStatus[256] = "";
    bool openConfigImportPopup = false;
    bool openConfigExportPopup = false;
    constexpr const char* ConfigImportPopupId = "ConfigImportTextPopup";
    constexpr const char* ConfigExportPopupId = "ConfigExportTextPopup";

    const NavItem navItems[] = {
        {Page::Player, "tab.player", "player"},
        {Page::Vehicle, "tab.vehicle", "vehicle"},
        {Page::Weapon, "tab.weapon", "weapon"},
        {Page::World, "tab.world", "world"},
        {Page::Teleport, "tab.teleport", "teleport"},
        {Page::Settings, "tab.settings", "settings"},
        {Page::About, "tab.about", "about"}
    };

    void LabelWithStableId(char* output, std::size_t outputSize, const char* textKey, const char* id) {
        std::snprintf(output, outputSize, "%s###%s", T(textKey), id);
    }

    void DrawSettings();
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
        ImGui::TextUnformatted(T(titleKey));
        ImGui::Separator();
        ImGui::Spacing();
    }

    void DrawVersionBadge() {
        const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
        const char* remoteVersion = info.latestVersion.empty() ? T("status.remoteUnknown") : info.latestVersion.c_str();
        const char* statusText = T("status.remoteUnknown");

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
            ImGui::EndTooltip();
        }
    }

    void DrawNavigation() {
        ImGui::TextUnformatted("XMenu");
        DrawVersionBadge();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (const NavItem& item : navItems) {
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
        case Page::Weapon: titleKey = "tab.weapon"; break;
        case Page::World: titleKey = "tab.world"; break;
        case Page::Teleport: titleKey = "tab.teleport"; break;
        case Page::Settings: titleKey = "tab.settings"; break;
        case Page::About: titleKey = "tab.about"; break;
        }

        DrawPageHeader(titleKey);

        switch (activePage) {
        case Page::Player: Pages::Player::Draw(); break;
        case Page::Vehicle: Pages::Vehicle::Draw(); break;
        case Page::Weapon: Pages::Weapon::Draw(); break;
        case Page::World: Pages::World::Draw(); break;
        case Page::Teleport: Pages::Teleport::Draw(); break;
        case Page::Settings: DrawSettings(); break;
        case Page::About: DrawAbout(); break;
        }
    }

    void DrawSettings() {
        ImGui::TextUnformatted(T("settings.interfaceLanguage"));
        ImGui::Spacing();

        const I18n::Language languages[] = {
            I18n::Language::Zh,
            I18n::Language::En,
            I18n::Language::Jp,
            I18n::Language::Ru
        };

        I18n::Language currentLanguage = I18n::GetLanguage();
        if (ImGui::BeginCombo("##InterfaceLanguage", I18n::GetLanguageName(currentLanguage))) {
            for (const I18n::Language language : languages) {
                const bool selected = currentLanguage == language;
                if (ImGui::Selectable(I18n::GetLanguageName(language), selected)) {
                    I18n::SetLanguage(language);
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

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

        ImGui::TextUnformatted(T("update.details"));
        ImGui::PushStyleColor(ImGuiCol_Text, VersionStatusColor(info.status));
        ImGui::TextUnformatted(checking ? T("status.checking") : VersionStatusText(info.status));
        ImGui::PopStyleColor();
        ImGui::Text(T("status.localVersion"), XMENU_VERSION);
        ImGui::Text(T("status.remoteVersion"), remoteVersion);

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

        if (ImGui::BeginPopupModal("XMenuUpdateDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            const UpdateChecker::UpdateInfo info = UpdateChecker::GetUpdateInfo();
            ImGui::TextUnformatted(T("update.availableTitle"));
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped(T("update.availableMessage"), info.currentVersion.c_str(), info.latestVersion.c_str());
            ImGui::Spacing();

            if (ImGui::Button(T("update.openGitHub"), ImVec2(130.0f, 0.0f))) {
                ShellExecuteA(nullptr, "open", info.releaseUrl.empty() ? XMENU_GITHUB : info.releaseUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                UpdateChecker::Dismiss();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(T("update.openGTAMODX"), ImVec2(130.0f, 0.0f))) {
                ShellExecuteA(nullptr, "open", XMENU_URL, nullptr, nullptr, SW_SHOWNORMAL);
                UpdateChecker::Dismiss();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}

void Menu::Process() {
    Pages::Player::Process();
    Pages::Vehicle::Process();
    Pages::Weapon::Process();
    Pages::World::Process();
}

void Menu::Draw() {
    bool menuVisible = D3DHook::IsMenuVisible();
    char windowTitle[160] = {};

    char visibleTitle[128] = {};
    std::snprintf(visibleTitle, sizeof(visibleTitle), T("window.title"), XMENU_AUTHOR);
    std::snprintf(windowTitle, sizeof(windowTitle), "%s###XMenuMainWindow", visibleTitle);

    ImGui::SetNextWindowSize(ImVec2(780.0f, 520.0f), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.060f, 0.075f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.075f, 0.082f, 0.100f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.180f, 0.280f, 0.420f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.220f, 0.340f, 0.520f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.260f, 0.420f, 0.650f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.170f, 0.270f, 0.400f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.220f, 0.350f, 0.540f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.260f, 0.420f, 0.650f, 1.00f));

    if (ImGui::Begin(windowTitle, &menuVisible, ImGuiWindowFlags_NoCollapse)) {
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

    ImGui::PopStyleColor(8);
    ImGui::PopStyleVar(3);

    if (!menuVisible) {
        D3DHook::SetMenuVisible(false);
    }
}