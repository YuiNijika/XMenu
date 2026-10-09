#include "Targeting.h"

#include "ui/MenuState.h"
#include "ui/Widget.h"
#include "utils/AppConfig.h"
#include "utils/I18n.h"

#include <XBase/Runtime.h>
#include <XBase/Targeting.h>
#include <XBase/UI.h>

#include <algorithm>
#include <cstdio>

namespace Pages::Targeting {
namespace {
using namespace XBase::Targeting;

bool DrawMenu(Kind kind, Menu& menu) {
    const auto actions = GetActions(kind);
    const char* directions[] = {"top", "topRight", "bottomRight", "bottom", "bottomLeft", "topLeft"};
    bool changed = false;
    for (std::size_t slot = 0; slot < SlotCount; ++slot) {
        Binding& binding = menu[slot];
        const auto selected = std::find_if(actions.begin(), actions.end(), [&](const ActionInfo& info) {
            return info.action == binding.action;
        });
        if (selected == actions.end()) continue;
        char label[160]{};
        const std::string key = std::string("targeting.slot.") + directions[slot];
        std::snprintf(label, sizeof(label), "%s###%d_%zu", I18n::T(key.c_str()), static_cast<int>(kind), slot);
        if (MenuState::UseNativeMenu) {
            std::string title = std::string(I18n::T(key.c_str())) + " / " + I18n::T(selected->labelKey);
            if (UI::Button(title.c_str())) {
                auto next = selected;
                do {
                    if (++next == actions.end()) next = actions.begin();
                } while (!next->supported && next != selected);
                binding = {next->action, next->defaultValue};
                if (next->parameter == Parameter::Weapon) binding.secondary = 100;
                changed = true;
            }
        } else {
            XBase::UI::Combo(label, I18n::T(selected->labelKey), [&] {
                for (const ActionInfo& info : actions) {
                    if (!info.supported) continue;
                    if (XBase::UI::Selectable(I18n::T(info.labelKey), binding.action == info.action)) {
                        binding = {info.action, info.defaultValue};
                        if (info.parameter == Parameter::Weapon) binding.secondary = 100;
                        changed = true;
                    }
                }
            });
        }
        const auto info = std::find_if(actions.begin(), actions.end(), [&](const ActionInfo& item) {
            return item.action == binding.action;
        });
        if (info == actions.end()) continue;
        const auto fieldLabel = [&](const char* field) {
            return std::string(I18n::T(field)) + "###" + std::to_string(static_cast<int>(kind))
                + "_" + std::to_string(slot) + "_" + field;
        };
        if (info->parameter == Parameter::Toggle) {
            changed |= UI::Checkbox(fieldLabel("targeting.parameter.enabled").c_str(), &binding.enabled);
        } else if (info->parameter != Parameter::None) {
            const char* valueKey = info->parameter == Parameter::Colors ? "targeting.parameter.primary"
                : info->parameter == Parameter::Weapon ? "targeting.parameter.weapon"
                : info->parameter == Parameter::Door ? "targeting.parameter.door"
                : info->parameter == Parameter::Seat ? "targeting.parameter.seat"
                : "targeting.parameter.value";
            changed |= UI::SliderFloat(fieldLabel(valueKey).c_str(), &binding.value, info->minimum, info->maximum, "%.0f");
        }
        if (info->parameter == Parameter::Colors || info->parameter == Parameter::Weapon) {
            const bool weapon = info->parameter == Parameter::Weapon;
            changed |= UI::SliderInt(fieldLabel(weapon ? "targeting.parameter.ammo" : "targeting.parameter.secondary").c_str(),
                &binding.secondary, 0, weapon ? 99999 : 255);
            if (!weapon && XBase::Runtime::GetGameTarget() == XBase::Runtime::GameTarget::SanAndreas) {
                changed |= UI::SliderInt(fieldLabel("targeting.parameter.tertiary").c_str(), &binding.tertiary, 0, 255);
                changed |= UI::SliderInt(fieldLabel("targeting.parameter.quaternary").c_str(), &binding.quaternary, 0, 255);
            }
        }
    }
    return changed;
}
}

void Draw() {
    Config config = GetConfig();
    bool changed = false;
    XBase::UI::BeginGroupBox("TargetingGeneral");
    XBase::UI::TextDisabled(I18n::T("targeting.nearby"));
    XBase::UI::Columns(2, "TargetingGeneralGrid", false);
    changed |= UI::Checkbox(I18n::T("targeting.enabled"), &config.enabled);
    changed |= UI::Checkbox(I18n::T("targeting.drawLinks"), &config.drawLinks);
    XBase::UI::NextColumn();
    changed |= UI::Checkbox(I18n::T("targeting.mouseSelect"), &config.mouseSelect);
    changed |= UI::Checkbox(I18n::T("targeting.vehicles"), &config.includeVehicles);
    XBase::UI::NextColumn();
    changed |= UI::Checkbox(I18n::T("targeting.peds"), &config.includePeds);
    changed |= UI::SliderInt(I18n::T("targeting.maxTargets"), &config.maxTargets, 1, 64);
    XBase::UI::NextColumn();
    changed |= UI::SliderFloat(I18n::T("targeting.radius"), &config.radius, 5, 250, "%.0f");
    changed |= UI::SliderFloat(I18n::T("targeting.hitRadius"), &config.hitRadius, 40, 400, "%.0f");
    XBase::UI::Columns(1);
    XBase::UI::EndGroupBox();

    static bool pedOpen = true;
    static bool vehicleOpen = true;
    if (UI::CollapsingHeader(I18n::T("targeting.pedMenu"), pedOpen)) {
        XBase::UI::BeginGroupBox("TargetingPedMenu");
        changed |= DrawMenu(Kind::Ped, config.pedMenu);
        XBase::UI::EndGroupBox();
    }
    if (UI::CollapsingHeader(I18n::T("targeting.vehicleMenu"), vehicleOpen)) {
        XBase::UI::BeginGroupBox("TargetingVehicleMenu");
        changed |= DrawMenu(Kind::Vehicle, config.vehicleMenu);
        XBase::UI::EndGroupBox();
    }
    if (UI::Button(I18n::T("targeting.resetMenu"))) {
        config.pedMenu = Config{}.pedMenu;
        config.vehicleMenu = Config{}.vehicleMenu;
        changed = true;
    }
    if (changed) {
        SetConfig(config);
        AppConfig::Save();
    }
    XBase::UI::Separator();
    XBase::UI::TextDisabled(I18n::T("targeting.nearby"));
    for (const Target& target : GetTargets()) {
        char text[160]{};
        std::snprintf(text, sizeof(text), "%s #%u / %.1fm / %.0f HP",
            I18n::T(target.kind == Kind::Vehicle ? "targeting.vehicle" : "targeting.ped"),
            target.id.value, target.distance, target.health);
        XBase::UI::Text(text);
    }
}
}
