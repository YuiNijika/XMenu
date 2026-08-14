#pragma once

#include <XBase/BulletAssist.h>
#include <XBase/Capabilities.h>
#include "ui/MenuState.h"

namespace Controllers::BulletAssist {
    inline XBase::BulletAssist::Config CurrentConfig() {
        const auto available = [](XBase::FeatureCapability capability) {
            return XBase::HasCapability(capability);
        };
        XBase::BulletAssist::Config config;
        config.tracking = available(XBase::FeatureCapability::BulletAssistTracking)
            && MenuState::WeaponBulletTrack;
        config.throughWalls = available(XBase::FeatureCapability::BulletAssistThroughWalls)
            && MenuState::WeaponBulletThroughWalls;
        config.hardLock = available(XBase::FeatureCapability::BulletAssistHardLock)
            && MenuState::WeaponBulletHardLock;
        config.trackCivilian = MenuState::WeaponTrackCivilian;
        config.trackFriend = MenuState::WeaponTrackFriend;
        config.trackHostile = MenuState::WeaponTrackHostile;
        config.trackNeutral = MenuState::WeaponTrackNeutral;
        config.aimPart = static_cast<XBase::BulletAssist::AimPart>(
            MenuState::WeaponBulletAimPart < 0 || MenuState::WeaponBulletAimPart > 3
                ? 1
                : MenuState::WeaponBulletAimPart);
        config.lockRange = MenuState::WeaponBulletLockRange;
        config.maxTargets = MenuState::WeaponBulletMaxTargets;
        config.drawPedBounds = available(XBase::FeatureCapability::BulletAssistPedBounds)
            && MenuState::WeaponPedEsp;
        config.drawPedCollision = available(XBase::FeatureCapability::BulletAssistPedCollision)
            && MenuState::WeaponPedColEsp;
        config.drawPedSkeleton = available(XBase::FeatureCapability::BulletAssistPedSkeleton)
            && MenuState::WeaponPedSkeleton;
        config.drawVehicleBounds = available(XBase::FeatureCapability::BulletAssistVehicleBounds)
            && MenuState::WeaponVehicleEsp;
        config.drawVehicleCollision = available(XBase::FeatureCapability::BulletAssistVehicleCollision)
            && MenuState::WeaponVehicleColEsp;
        return config;
    }

    inline void SyncConfig() {
        XBase::BulletAssist::SetConfig(CurrentConfig());
    }

    inline void Init() {}

    inline void Process() {
        SyncConfig();
    }

    inline void Draw() {
        XBase::BulletAssist::Draw();
    }

    inline void Shutdown() {}
}
