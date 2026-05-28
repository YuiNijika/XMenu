#include "Ped.h"
#include "features/GameLogic.h"
#include "ui/MenuState.h"
#include "utils/Log.h"

namespace {
    CPed* lastSpawnedPed = nullptr;

    GameLogic::PedSpawnOptions CurrentOptions() {
        GameLogic::PedSpawnOptions options;
        options.modelId = static_cast<unsigned int>(MenuState::PedSpawnModel < 0 ? 0 : MenuState::PedSpawnModel);
        options.pedType = MenuState::PedSpawnType;
        options.gangType = MenuState::PedGangType;
        options.asGang = MenuState::PedSpawnAsGang;
        options.health = MenuState::PedHealth;
        options.armour = MenuState::PedArmour;
        options.freeze = MenuState::PedFreeze;
        options.hostile = MenuState::PedHostile;
        options.weaponModel = static_cast<unsigned int>(MenuState::PedWeaponModel < 0 ? 0 : MenuState::PedWeaponModel);
        return options;
    }
}

namespace Controllers::Ped {
    CPed* GetLastSpawnedPed() {
        return lastSpawnedPed;
    }

    bool SpawnNearPlayer() {
        lastSpawnedPed = GameLogic::SpawnPedNearPlayer(CurrentOptions());
        if (!lastSpawnedPed) {
            MenuState::ShowNotice("Ped spawn failed", 2.0);
            Log::Warn("Ped 生成失败：玩家附近生成未返回有效对象");
            return false;
        }
        MenuState::ShowNotice("Ped spawned", 1.5);
        return true;
    }

    bool SpawnAtMarker() {
        lastSpawnedPed = GameLogic::SpawnPedAtMarker(CurrentOptions());
        if (!lastSpawnedPed) {
            MenuState::ShowNotice("Ped marker spawn unavailable", 2.0);
            Log::Warn("Ped 标记点生成失败或当前版本不支持");
            return false;
        }
        MenuState::ShowNotice("Ped spawned at marker", 1.5);
        return true;
    }

    void DeleteLastSpawnedPed() {
        GameLogic::DeletePed(lastSpawnedPed);
        lastSpawnedPed = nullptr;
    }

    void Process() {
        if (lastSpawnedPed) {
            GameLogic::ApplyPedOptions(lastSpawnedPed, CurrentOptions());
        }

        CPlayerPed* player = FindPlayerPed();
        if (player) {
            GameLogic::ProcessSmokingEffect(player, MenuState::SmokingEffect);
            GameLogic::ProcessFliesEffect(player, MenuState::FliesEffect);
        }
    }
}