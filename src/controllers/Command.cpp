#include "Command.h"
#include "Player.h"
#include "Vehicle.h"
#include "Weapon.h"
#include "World.h"
#include "Teleport.h"
#include "ui/MenuState.h"
#include "utils/I18n.h"
#include <XBase/Teleport.h>
#include <XBase/UI.h>
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
            XBase::Teleport::To(x, y, z);
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

        XBase::UI::SetNextWindowSize({520.0f, 280.0f}, true);
        XBase::UI::Window("CommandWindow", I18n::T("command.title"), [&] {
            XBase::UI::Child("CommandHistory", [&] {
                for (const std::string& line : history) {
                    XBase::UI::Text(line.c_str());
                }
            }, {0.0f, -32.0f}, true);

            XBase::UI::PushItemWidth(-72.0f);
            const bool submit = XBase::UI::InputText("##CommandInput", input, sizeof(input), nullptr, false, true);
            XBase::UI::PopItemWidth();
            XBase::UI::SameLine();
            if (XBase::UI::Button(I18n::T("command.run")) || submit) {
                Execute(input);
                input[0] = '\0';
            }
        }, &MenuState::CommandWindowEnabled);
    }
}