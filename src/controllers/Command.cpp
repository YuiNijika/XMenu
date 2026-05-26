#include "Command.h"
#include "Player.h"
#include "Vehicle.h"
#include "Weapon.h"
#include "World.h"
#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include "imgui/imgui.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
    char input[256] = "";
    std::vector<std::string> history;

    void AddLine(const std::string& line) {
        history.push_back(line);
        if (history.size() > 64) {
            history.erase(history.begin());
        }
    }

    void Execute(const char* raw) {
        if (!raw || raw[0] == '\0') return;
        AddLine(std::string("> ") + raw);

        float x = 0.0f, y = 0.0f, z = 10.0f;
        int id = 0;
        if (std::sscanf(raw, "tp %f %f %f", &x, &y, &z) == 3) {
            Controllers::Teleport::To(x, y, z);
            AddLine(I18n::T("command.teleported"));
            return;
        }
        if (std::sscanf(raw, "veh %d", &id) == 1) {
            AddLine(Controllers::Vehicle::Spawn(static_cast<unsigned int>(id)) ? I18n::T("command.vehicleSpawned") : I18n::T("command.vehicleSpawnFailed"));
            return;
        }
        if (std::strcmp(raw, "heal") == 0) {
            Controllers::Player::Heal();
            AddLine(I18n::T("command.healed"));
            return;
        }
        if (std::strcmp(raw, "armor") == 0 || std::strcmp(raw, "armour") == 0) {
            Controllers::Player::GiveArmour();
            AddLine(I18n::T("command.armourRefilled"));
            return;
        }
        if (std::sscanf(raw, "weather %d", &id) == 1) {
            AddLine(I18n::T("command.weatherUnavailable"));
            return;
        }
        if (std::strcmp(raw, "weapons") == 0) {
            Controllers::Weapon::GiveAll();
            AddLine(I18n::T("command.weaponsGranted"));
            return;
        }
        AddLine(I18n::T("command.unknown"));
    }
}

namespace Controllers::Command {
    void Process() {
    }

    void Draw() {
        if (!MenuState::CommandWindowEnabled) {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(520.0f, 280.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(I18n::T("command.title"), &MenuState::CommandWindowEnabled)) {
            ImGui::BeginChild("CommandHistory", ImVec2(0.0f, -32.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (const std::string& line : history) {
                ImGui::TextUnformatted(line.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();

            ImGui::PushItemWidth(-72.0f);
            const bool submit = ImGui::InputText("##CommandInput", input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(I18n::T("command.run")) || submit) {
                Execute(input);
                input[0] = '\0';
            }
        }
        ImGui::End();
    }
}