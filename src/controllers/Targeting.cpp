#include "Targeting.h"

#include "utils/I18n.h"

#include <XBase/Targeting.h>
#include <XBase/Runtime.h>

namespace Controllers::Targeting {
namespace {
using XBase::Json::Value;
using namespace XBase::Targeting;

Value MenuPayload(const Menu& menu) {
    Value result;
    for (const Binding& binding : menu) {
        Value item;
        item.Set("action", Value(static_cast<int>(binding.action)));
        item.Set("value", Value(static_cast<double>(binding.value)));
        item.Set("secondary", Value(binding.secondary));
        item.Set("tertiary", Value(binding.tertiary));
        item.Set("quaternary", Value(binding.quaternary));
        item.Set("enabled", Value(binding.enabled));
        result.Push(item);
    }
    return result;
}

void ReadBinding(const Value& params, Binding& binding) {
    if (!params.IsObject()) return;
    binding.action = static_cast<Action>(params["action"].AsInt(static_cast<int>(binding.action)));
    binding.value = static_cast<float>(params["value"].AsNumber(binding.value));
    binding.secondary = params["secondary"].AsInt(binding.secondary);
    binding.tertiary = params["tertiary"].AsInt(binding.tertiary);
    binding.quaternary = params["quaternary"].AsInt(binding.quaternary);
    binding.enabled = params["enabled"].AsBool(binding.enabled);
}

void ReadMenu(const Value& params, Menu& menu) {
    if (!params.IsArray() || params.Size() != SlotCount) return;
    for (std::size_t i = 0; i < SlotCount; ++i) {
        ReadBinding(params[i], menu[i]);
    }
}
}

Value ConfigPayload() {
    const Config config = GetConfig();
    Value result;
    result.Set("enabled", Value(config.enabled));
    result.Set("drawLinks", Value(config.drawLinks));
    result.Set("includePeds", Value(config.includePeds));
    result.Set("includeVehicles", Value(config.includeVehicles));
    result.Set("mouseSelect", Value(config.mouseSelect));
    result.Set("radius", Value(static_cast<double>(config.radius)));
    result.Set("hitRadius", Value(static_cast<double>(config.hitRadius)));
    result.Set("maxTargets", Value(config.maxTargets));
    result.Set("pedMenu", MenuPayload(config.pedMenu));
    result.Set("vehicleMenu", MenuPayload(config.vehicleMenu));
    return result;
}

Value ActionsPayload() {
    Value result;
    result.Set("colorChannels", Value(XBase::Runtime::GetGameTarget() == XBase::Runtime::GameTarget::SanAndreas ? 4 : 2));
    for (Kind kind : {Kind::Ped, Kind::Vehicle}) {
        Value items;
        for (const ActionInfo& action : GetActions(kind)) {
            Value item;
            item.Set("action", Value(static_cast<int>(action.action)));
            item.Set("labelKey", Value(action.labelKey));
            item.Set("parameter", Value(static_cast<int>(action.parameter)));
            item.Set("min", Value(static_cast<double>(action.minimum)));
            item.Set("max", Value(static_cast<double>(action.maximum)));
            item.Set("defaultValue", Value(static_cast<double>(action.defaultValue)));
            item.Set("supported", Value(action.supported));
            items.Push(item);
        }
        result.Set(kind == Kind::Ped ? "ped" : "vehicle", items);
    }
    return result;
}

void ApplyConfig(const Value& params) {
    Config config = GetConfig();
    if (params["reset"].AsBool()) {
        config.pedMenu = Config{}.pedMenu;
        config.vehicleMenu = Config{}.vehicleMenu;
    }
    if (params["enabled"].IsBool()) config.enabled = params["enabled"].AsBool();
    if (params["drawLinks"].IsBool()) config.drawLinks = params["drawLinks"].AsBool();
    if (params["includePeds"].IsBool()) config.includePeds = params["includePeds"].AsBool();
    if (params["includeVehicles"].IsBool()) config.includeVehicles = params["includeVehicles"].AsBool();
    if (params["mouseSelect"].IsBool()) config.mouseSelect = params["mouseSelect"].AsBool();
    if (params["radius"].IsNumber()) config.radius = static_cast<float>(params["radius"].AsNumber());
    if (params["hitRadius"].IsNumber()) config.hitRadius = static_cast<float>(params["hitRadius"].AsNumber());
    if (params["maxTargets"].IsNumber()) config.maxTargets = params["maxTargets"].AsInt();
    ReadMenu(params["pedMenu"], config.pedMenu);
    ReadMenu(params["vehicleMenu"], config.vehicleMenu);
    if (params["slot"].IsNumber() && params["binding"].IsObject()) {
        const int slot = params["slot"].AsInt(-1);
        const std::string kind = params["kind"].AsString();
        if (slot >= 0 && slot < static_cast<int>(SlotCount) && (kind == "ped" || kind == "vehicle")) {
            Menu& menu = kind == "ped" ? config.pedMenu : config.vehicleMenu;
            ReadBinding(params["binding"], menu[static_cast<std::size_t>(slot)]);
        }
    }
    SetConfig(config);
}

void UpdateLabels() {
    static std::string lastLanguage;
    static std::string lastFallback;
    static XBase::Runtime::GameTarget lastGame = XBase::Runtime::GameTarget::Unknown;
    const std::string language = I18n::GetCurrentLanguageCode();
    const std::string fallback = I18n::GetFallbackLanguageCode();
    const auto game = XBase::Runtime::GetGameTarget();
    if (language == lastLanguage && fallback == lastFallback && game == lastGame) return;
    lastLanguage = language;
    lastFallback = fallback;
    lastGame = game;
    Labels labels;
    labels.ped = I18n::T("targeting.ped");
    labels.vehicle = I18n::T("targeting.vehicle");
    labels.heal = I18n::T("targeting.heal");
    labels.restore = I18n::T("targeting.restore");
    for (Kind kind : {Kind::Ped, Kind::Vehicle}) {
        for (const ActionInfo& info : GetActions(kind)) {
            labels.actions[static_cast<std::size_t>(info.action)] = I18n::T(info.labelKey);
        }
    }
    SetLabels(labels);
}

}
