#include "BulletAssist.h"
#include "ui/MenuState.h"
#include "utils/Log.h"
#include "plugin.h"
#include "CWorld.h"
#include "CPools.h"
#include "CPed.h"
#include "CPlayerPed.h"
#include "CVehicle.h"
#include "CSprite.h"
#include "CColPoint.h"
#include "CVector.h"
#include "CEntity.h"
#include "CColModel.h"
#include "CCollisionData.h"
#include "CColBox.h"
#include "CColSphere.h"
#include "CWeapon.h"
#include "ePedBones.h"
#include "ePedState.h"
#include "kiero/minhook/MinHook.h"
#include "imgui/imgui.h"
#include <algorithm>
#include <cmath>
#include <vector>

// SA：与 VC demo 同策略
// - 只在 FireInstantHit* 调用栈内改写 CWorld::ProcessLineOfSight
// - 追踪：范围内最多 N 个目标；子弹按准星亲和度分配，连射可落到多个目标
// - 穿墙：仅开火时忽略建筑/物体

namespace {
    using ProcessLineOfSight_t = bool(__cdecl*)(
        CVector const& origin,
        CVector const& target,
        CColPoint& outColPoint,
        CEntity*& outEntity,
        bool buildings,
        bool vehicles,
        bool peds,
        bool objects,
        bool dummies,
        bool doSeeThroughCheck,
        bool doCameraIgnoreCheck,
        bool doShootThroughCheck);

    using FireInstantHit_t = bool(__thiscall*)(
        CWeapon* self,
        CEntity* firingEntity,
        CVector* origin,
        CVector* muzzlePosn,
        CEntity* targetEntity,
        CVector* target,
        CVector* originForDriveBy,
        bool arg6,
        bool muzzle);

    using FireInstantHitFromCar_t = bool(__thiscall*)(CWeapon* self, CVehicle* vehicle, bool leftSide, bool rightSide);

    ProcessLineOfSight_t oProcessLineOfSight = nullptr;
    FireInstantHit_t oFireInstantHit = nullptr;
    FireInstantHitFromCar_t oFireInstantHitFromCar = nullptr;
    bool s_hookAttempted = false;
    int s_bulletFireDepth = 0;

    constexpr float kPi = 3.14159265f;

    struct TrackCandidate {
        CVector pos{};
        float playerDistSq = 0.0f;
    };

    struct LockCache {
        std::vector<CVector> targets;
    };
    LockCache s_lock;
    unsigned s_trackRoundRobin = 0;

    // 一次 FireInstantHit* 只绑定一个最终目标，避免开火改 Dest 与多次 LOS 重复选目标互相打架
    struct ShotTrackState {
        bool hasTarget = false;
        CVector target{};
    };
    ShotTrackState s_shotTrack;

    bool InBulletFire() {
        return s_bulletFireDepth > 0;
    }

    float Dist2XY(const CVector& a, const CVector& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    float LockRange() {
        return MenuState::WeaponBulletLockRange > 10.0f
            ? MenuState::WeaponBulletLockRange
            : 10.0f;
    }

    int MaxTrackTargets() {
        int n = MenuState::WeaponBulletMaxTargets;
        if (n < 1) {
            n = 1;
        }
        if (n > 16) {
            n = 16;
        }
        return n;
    }

    bool IsUsablePed(CPed* ped, CPed* player) {
        if (!ped || ped == player) {
            return false;
        }
        if (ped->m_fHealth <= 0.0f) {
            return false;
        }
        if (ped->m_ePedState == PEDSTATE_DEAD || ped->m_ePedState == PEDSTATE_DIE || ped->m_ePedState == PEDSTATE_DIE_BY_STEALTH) {
            return false;
        }
        return true;
    }

    bool IsUsableVehicle(CVehicle* veh) {
        return veh && veh->m_fHealth > 0.0f;
    }

    CVector BonePos(CPed* ped, ePedBones bone) {
        RwV3d out{};
        ped->GetBonePosition(out, static_cast<unsigned int>(bone), false);
        return CVector(out.x, out.y, out.z);
    }

    bool BoneValid(const CVector& p) {
        return std::fabs(p.x) > 0.001f || std::fabs(p.y) > 0.001f || std::fabs(p.z) > 0.001f;
    }

    CVector PedAimPoint(CPed* ped) {
        // 头骨偶发无效会“看起来没追踪上”；优先头，失败用躯干，再失败用实体点
        const CVector head = BonePos(ped, BONE_HEAD);
        if (BoneValid(head)) {
            return head;
        }
        const CVector spine = BonePos(ped, BONE_SPINE1);
        if (BoneValid(spine)) {
            return spine;
        }
        CVector pos = ped->GetPosition();
        pos.z += 0.7f;
        return pos;
    }

    std::vector<TrackCandidate> CollectTrackCandidates() {
        std::vector<TrackCandidate> candidates;
        CPed* player = FindPlayerPed();
        if (!player || !CPools::ms_pPedPool) {
            return candidates;
        }

        const CVector playerPos = player->GetPosition();
        const float maxRangeSq = LockRange() * LockRange();
        const int maxTargets = MaxTrackTargets();
        candidates.reserve(static_cast<std::size_t>(maxTargets));

        for (CPed* ped : CPools::ms_pPedPool) {
            if (!IsUsablePed(ped, player)) {
                continue;
            }
            const CVector pos = PedAimPoint(ped);
            const float d2 = Dist2XY(playerPos, pos);
            if (d2 > maxRangeSq) {
                continue;
            }

            TrackCandidate item{ pos, d2 };
            if (static_cast<int>(candidates.size()) < maxTargets) {
                candidates.push_back(item);
                std::push_heap(candidates.begin(), candidates.end(), [](const TrackCandidate& a, const TrackCandidate& b) {
                    return a.playerDistSq < b.playerDistSq; // 最大堆：堆顶最远
                });
                continue;
            }
            if (d2 >= candidates.front().playerDistSq) {
                continue;
            }
            std::pop_heap(candidates.begin(), candidates.end(), [](const TrackCandidate& a, const TrackCandidate& b) {
                return a.playerDistSq < b.playerDistSq;
            });
            candidates.back() = item;
            std::push_heap(candidates.begin(), candidates.end(), [](const TrackCandidate& a, const TrackCandidate& b) {
                return a.playerDistSq < b.playerDistSq;
            });
        }

        std::sort(candidates.begin(), candidates.end(), [](const TrackCandidate& a, const TrackCandidate& b) {
            return a.playerDistSq < b.playerDistSq;
        });
        return candidates;
    }

    void RefreshLockVisuals(const std::vector<TrackCandidate>& candidates) {
        s_lock.targets.clear();
        s_lock.targets.reserve(candidates.size());
        for (const TrackCandidate& item : candidates) {
            s_lock.targets.push_back(item.pos);
        }
    }

    // 本发射击绑定最终落点：单目标=最近；多目标=轮询分别追踪（每发推进一次）
    void BeginShotTrack() {
        s_shotTrack.hasTarget = false;
        s_shotTrack.target = CVector{};
        if (!MenuState::WeaponBulletTrack) {
            return;
        }

        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        if (candidates.empty()) {
            return;
        }

        if (candidates.size() == 1 || MaxTrackTargets() == 1) {
            s_shotTrack.target = candidates[0].pos;
        } else {
            s_shotTrack.target = candidates[s_trackRoundRobin % candidates.size()].pos;
            ++s_trackRoundRobin;
        }
        s_shotTrack.hasTarget = true;
    }

    void EndShotTrack() {
        s_shotTrack.hasTarget = false;
    }

    bool ResolveShotTrackTarget(CVector& outTarget) {
        if (!MenuState::WeaponBulletTrack) {
            return false;
        }
        // 开火栈内：固定使用本发已绑定目标，保证每一发都稳定落到同一点
        if (InBulletFire()) {
            if (!s_shotTrack.hasTarget) {
                return false;
            }
            outTarget = s_shotTrack.target;
            return true;
        }
        // 兜底（不应常走到）：即时选一个
        const std::vector<TrackCandidate> candidates = CollectTrackCandidates();
        RefreshLockVisuals(candidates);
        if (candidates.empty()) {
            return false;
        }
        outTarget = candidates[0].pos;
        return true;
    }

    struct PlayerShotScope {
        bool active = false;
        explicit PlayerShotScope(bool enable) {
            if (!enable) {
                return;
            }
            active = true;
            ++s_bulletFireDepth;
            if (s_bulletFireDepth == 1) {
                BeginShotTrack();
            }
        }
        ~PlayerShotScope() {
            if (!active) {
                return;
            }
            if (s_bulletFireDepth == 1) {
                EndShotTrack();
            }
            --s_bulletFireDepth;
        }
    };

    bool __cdecl hkProcessLineOfSight(
        CVector const& origin,
        CVector const& target,
        CColPoint& outColPoint,
        CEntity*& outEntity,
        bool buildings,
        bool vehicles,
        bool peds,
        bool objects,
        bool dummies,
        bool doSeeThroughCheck,
        bool doCameraIgnoreCheck,
        bool doShootThroughCheck) {
        if (!oProcessLineOfSight) {
            return false;
        }

        // 玩家开火栈内：放宽 LOS 特征，避免部分武器 buildings/peds 组合漏改 Dest
        if (!InBulletFire() || doCameraIgnoreCheck) {
            return oProcessLineOfSight(
                origin, target, outColPoint, outEntity,
                buildings, vehicles, peds, objects, dummies,
                doSeeThroughCheck, doCameraIgnoreCheck, doShootThroughCheck);
        }

        CVector redirected = target;
        bool useBuildings = buildings;
        bool useObjects = objects;
        bool useDummies = dummies;

        if (MenuState::WeaponBulletTrack) {
            CVector trackPos{};
            if (ResolveShotTrackTarget(trackPos)) {
                redirected = trackPos;
            }
        }

        if (MenuState::WeaponBulletThroughWalls) {
            useBuildings = false;
            useObjects = false;
            useDummies = false;
        }

        return oProcessLineOfSight(
            origin, redirected, outColPoint, outEntity,
            useBuildings, vehicles, peds, useObjects, useDummies,
            doSeeThroughCheck, doCameraIgnoreCheck, doShootThroughCheck);
    }

    bool IsLocalPlayerEntity(CEntity* ent) {
        CPed* player = FindPlayerPed();
        return player && ent == static_cast<CEntity*>(player);
    }

    bool __fastcall hkFireInstantHit(
        CWeapon* self,
        void* /*edx*/,
        CEntity* firingEntity,
        CVector* origin,
        CVector* muzzlePosn,
        CEntity* targetEntity,
        CVector* target,
        CVector* originForDriveBy,
        bool arg6,
        bool muzzle) {
        PlayerShotScope shotScope(IsLocalPlayerEntity(firingEntity));
        if (!oFireInstantHit) {
            return false;
        }

        // 开枪前改 Dest；与 LOS 共用本发绑定目标，保证每一发都追踪
        CVector saved{};
        bool patched = false;
        if (shotScope.active && MenuState::WeaponBulletTrack && target) {
            CVector trackPos{};
            if (ResolveShotTrackTarget(trackPos)) {
                saved = *target;
                *target = trackPos;
                patched = true;
            }
        }

        const bool result = oFireInstantHit(
            self, firingEntity, origin, muzzlePosn, targetEntity, target, originForDriveBy, arg6, muzzle);

        if (patched) {
            *target = saved;
        }
        return result;
    }

    bool __fastcall hkFireInstantHitFromCar(CWeapon* self, void* /*edx*/, CVehicle* vehicle, bool leftSide, bool rightSide) {
        CPed* player = FindPlayerPed();
        const bool playerShot = player && player->m_pVehicle == vehicle;
        PlayerShotScope shotScope(playerShot);
        if (!oFireInstantHitFromCar) {
            return false;
        }
        return oFireInstantHitFromCar(self, vehicle, leftSide, rightSide);
    }

    void EnsureHook() {
        if (s_hookAttempted) {
            return;
        }
        s_hookAttempted = true;

        MH_STATUS initStatus = MH_Initialize();
        if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
            Log::Warn("BulletAssist SA: MinHook 初始化失败");
            return;
        }

        // FireInstantHit @ 0x73FB10
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x73FB10),
                reinterpret_cast<void*>(&hkFireInstantHit),
                reinterpret_cast<void**>(&oFireInstantHit));
            if (st == MH_OK || st == MH_ERROR_ALREADY_CREATED) {
                MH_EnableHook(reinterpret_cast<void*>(0x73FB10));
            } else {
                Log::Warn("BulletAssist SA: FireInstantHit hook 失败");
            }
        }
        // FireInstantHitFromCar @ 0x73EC40
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x73EC40),
                reinterpret_cast<void*>(&hkFireInstantHitFromCar),
                reinterpret_cast<void**>(&oFireInstantHitFromCar));
            if (st == MH_OK || st == MH_ERROR_ALREADY_CREATED) {
                MH_EnableHook(reinterpret_cast<void*>(0x73EC40));
            }
        }

        // CWorld::ProcessLineOfSight @ 0x56BA00 — 仅 InBulletFire
        {
            const MH_STATUS st = MH_CreateHook(
                reinterpret_cast<void*>(0x56BA00),
                reinterpret_cast<void*>(&hkProcessLineOfSight),
                reinterpret_cast<void**>(&oProcessLineOfSight));
            if (st != MH_OK && st != MH_ERROR_ALREADY_CREATED) {
                Log::Warn("BulletAssist SA: CWorld::PLOS hook 失败");
                return;
            }
            const MH_STATUS en = MH_EnableHook(reinterpret_cast<void*>(0x56BA00));
            if (en != MH_OK && en != MH_ERROR_ENABLED) {
                Log::Warn("BulletAssist SA: 启用 CWorld::PLOS hook 失败");
                return;
            }
        }

        Log::Info("BulletAssist SA: 开火上下文 + LOS hook 已就绪");
    }

    bool WorldToScreen(const CVector& world, ImVec2& out) {
        RwV3d in{world.x, world.y, world.z};
        RwV3d scr{};
        float w = 0.0f;
        float h = 0.0f;
        if (!CSprite::CalcScreenCoors(in, &scr, &w, &h, true, true) || w < 1.0f || h < 1.0f) {
            return false;
        }
        out = ImVec2(scr.x, scr.y);
        return true;
    }

    void DrawWorldLine(ImDrawList* dl, const CVector& a, const CVector& b, ImU32 color, float thickness = 1.25f) {
        ImVec2 sa{};
        ImVec2 sb{};
        if (!WorldToScreen(a, sa) || !WorldToScreen(b, sb)) {
            return;
        }
        dl->AddLine(sa, sb, color, thickness);
    }

    void DrawLocalBoxWire(ImDrawList* dl, CEntity* ent, const CVector& mn, const CVector& mx, ImU32 color) {
        if (!ent) {
            return;
        }
        CVector c[8];
        c[0] = ent->TransformFromObjectSpace(CVector(mn.x, mn.y, mn.z));
        c[1] = ent->TransformFromObjectSpace(CVector(mx.x, mn.y, mn.z));
        c[2] = ent->TransformFromObjectSpace(CVector(mx.x, mx.y, mn.z));
        c[3] = ent->TransformFromObjectSpace(CVector(mn.x, mx.y, mn.z));
        c[4] = ent->TransformFromObjectSpace(CVector(mn.x, mn.y, mx.z));
        c[5] = ent->TransformFromObjectSpace(CVector(mx.x, mn.y, mx.z));
        c[6] = ent->TransformFromObjectSpace(CVector(mx.x, mx.y, mx.z));
        c[7] = ent->TransformFromObjectSpace(CVector(mn.x, mx.y, mx.z));
        static const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        };
        for (const auto& e : edges) {
            DrawWorldLine(dl, c[e[0]], c[e[1]], color);
        }
    }

    void DrawLocalSphereWire(ImDrawList* dl, CEntity* ent, const CVector& center, float radius, ImU32 color) {
        if (!ent || radius < 0.01f) {
            return;
        }
        constexpr int kSeg = 12;
        CVector prevXY{};
        CVector prevXZ{};
        CVector prevYZ{};
        for (int i = 0; i <= kSeg; ++i) {
            const float ang = (kPi * 2.0f) * static_cast<float>(i) / static_cast<float>(kSeg);
            const float ca = std::cos(ang);
            const float sa = std::sin(ang);
            const CVector pXY = ent->TransformFromObjectSpace(CVector(center.x + radius * ca, center.y + radius * sa, center.z));
            const CVector pXZ = ent->TransformFromObjectSpace(CVector(center.x + radius * ca, center.y, center.z + radius * sa));
            const CVector pYZ = ent->TransformFromObjectSpace(CVector(center.x, center.y + radius * ca, center.z + radius * sa));
            if (i > 0) {
                DrawWorldLine(dl, prevXY, pXY, color);
                DrawWorldLine(dl, prevXZ, pXZ, color);
                DrawWorldLine(dl, prevYZ, pYZ, color);
            }
            prevXY = pXY;
            prevXZ = pXZ;
            prevYZ = pYZ;
        }
    }

    void DrawEntityDebugBound(ImDrawList* dl, CEntity* ent, ImU32 color) {
        CColModel* col = ent->GetColModel();
        if (!col) {
            return;
        }
        DrawLocalBoxWire(dl, ent, col->m_boundBox.m_vecMin, col->m_boundBox.m_vecMax, color);
    }

    void DrawEntityColPartial(ImDrawList* dl, CEntity* ent, ImU32 boxColor, ImU32 sphereColor) {
        CColModel* col = ent->GetColModel();
        if (!col || !col->m_pColData) {
            return;
        }
        CCollisionData* data = col->m_pColData;
        if (data->m_pBoxes && data->m_nNumBoxes > 0) {
            for (unsigned short i = 0; i < data->m_nNumBoxes; ++i) {
                DrawLocalBoxWire(dl, ent, data->m_pBoxes[i].m_vecMin, data->m_pBoxes[i].m_vecMax, boxColor);
            }
        }
        if (data->m_pSpheres && data->m_nNumSpheres > 0) {
            for (unsigned short i = 0; i < data->m_nNumSpheres; ++i) {
                DrawLocalSphereWire(dl, ent, data->m_pSpheres[i].m_vecCenter, data->m_pSpheres[i].m_fRadius, sphereColor);
            }
        }
    }

    void DrawBoneLine(ImDrawList* dl, CPed* ped, ePedBones a, ePedBones b, ImU32 color) {
        const CVector pa = BonePos(ped, a);
        const CVector pb = BonePos(ped, b);
        if (!BoneValid(pa) || !BoneValid(pb)) {
            return;
        }
        DrawWorldLine(dl, pa, pb, color);
    }

    void DrawPedSkeleton(ImDrawList* dl, CPed* ped, ImU32 color) {
        DrawBoneLine(dl, ped, BONE_PELVIS, BONE_SPINE1, color);
        DrawBoneLine(dl, ped, BONE_SPINE1, BONE_NECK, color);
        DrawBoneLine(dl, ped, BONE_NECK, BONE_HEAD, color);
        DrawBoneLine(dl, ped, BONE_NECK, BONE_LEFTSHOULDER, color);
        DrawBoneLine(dl, ped, BONE_LEFTSHOULDER, BONE_LEFTELBOW, color);
        DrawBoneLine(dl, ped, BONE_LEFTELBOW, BONE_LEFTHAND, color);
        DrawBoneLine(dl, ped, BONE_NECK, BONE_RIGHTSHOULDER, color);
        DrawBoneLine(dl, ped, BONE_RIGHTSHOULDER, BONE_RIGHTELBOW, color);
        DrawBoneLine(dl, ped, BONE_RIGHTELBOW, BONE_RIGHTHAND, color);
        DrawBoneLine(dl, ped, BONE_PELVIS, BONE_LEFTHIP, color);
        DrawBoneLine(dl, ped, BONE_LEFTHIP, BONE_LEFTKNEE, color);
        DrawBoneLine(dl, ped, BONE_LEFTKNEE, BONE_LEFTFOOT, color);
        DrawBoneLine(dl, ped, BONE_PELVIS, BONE_RIGHTHIP, color);
        DrawBoneLine(dl, ped, BONE_RIGHTHIP, BONE_RIGHTKNEE, color);
        DrawBoneLine(dl, ped, BONE_RIGHTKNEE, BONE_RIGHTFOOT, color);
    }

    void DrawTrackRange(ImDrawList* dl, CPed* player) {
        const float range = LockRange();
        const CVector center = player->GetPosition();
        const ImU32 rangeColor = IM_COL32(255, 80, 80, 180);
        const ImU32 lockColor = IM_COL32(255, 40, 40, 255);

        constexpr int kSeg = 48;
        ImVec2 prev{};
        bool hasPrev = false;
        for (int i = 0; i <= kSeg; ++i) {
            const float ang = (kPi * 2.0f) * static_cast<float>(i) / static_cast<float>(kSeg);
            const CVector p(center.x + std::cos(ang) * range, center.y + std::sin(ang) * range, center.z);
            ImVec2 s{};
            if (WorldToScreen(p, s)) {
                if (hasPrev) {
                    dl->AddLine(prev, s, rangeColor, 1.5f);
                }
                prev = s;
                hasPrev = true;
            } else {
                hasPrev = false;
            }
        }

        for (const CVector& lockPos : s_lock.targets) {
            ImVec2 s{};
            if (WorldToScreen(lockPos, s)) {
                dl->AddCircle(s, 12.0f, lockColor, 16, 2.0f);
                dl->AddCircleFilled(s, 3.0f, lockColor, 8);
            }
            DrawWorldLine(dl, center, lockPos, lockColor, 1.4f);
        }
    }
}

namespace Controllers::BulletAssist {
    void Init() {
        EnsureHook();
    }

    void Process() {
        s_lock.targets.clear();
        if (!MenuState::WeaponBulletTrack) {
            return;
        }
        EnsureHook();
        RefreshLockVisuals(CollectTrackCandidates());
    }

    void Draw() {
        const bool pedDebug = MenuState::WeaponPedEsp;
        const bool pedCol = MenuState::WeaponPedColEsp;
        const bool pedSkel = MenuState::WeaponPedSkeleton;
        const bool vehDebug = MenuState::WeaponVehicleEsp;
        const bool vehCol = MenuState::WeaponVehicleColEsp;
        const bool track = MenuState::WeaponBulletTrack;
        if (!pedDebug && !pedCol && !pedSkel && !vehDebug && !vehCol && !track) {
            return;
        }

        CPed* player = FindPlayerPed();
        if (!player) {
            return;
        }

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        if (!dl) {
            return;
        }

        if (track) {
            DrawTrackRange(dl, player);
        }

        const ImU32 pedBoundColor = IM_COL32(80, 220, 120, 230);
        const ImU32 pedBoxColor = IM_COL32(60, 180, 255, 220);
        const ImU32 pedSphColor = IM_COL32(120, 200, 255, 200);
        const ImU32 skelColor = IM_COL32(255, 200, 60, 230);
        const ImU32 vehBoundColor = IM_COL32(255, 140, 60, 230);
        const ImU32 vehBoxColor = IM_COL32(255, 90, 90, 220);
        const ImU32 vehSphColor = IM_COL32(255, 160, 120, 200);

        if ((pedDebug || pedCol || pedSkel) && CPools::ms_pPedPool) {
            for (CPed* ped : CPools::ms_pPedPool) {
                if (!IsUsablePed(ped, player)) {
                    continue;
                }
                if (pedDebug) {
                    DrawEntityDebugBound(dl, ped, pedBoundColor);
                }
                if (pedCol) {
                    DrawEntityColPartial(dl, ped, pedBoxColor, pedSphColor);
                }
                if (pedSkel) {
                    DrawPedSkeleton(dl, ped, skelColor);
                }
            }
        }

        if ((vehDebug || vehCol) && CPools::ms_pVehiclePool) {
            for (CVehicle* veh : CPools::ms_pVehiclePool) {
                if (!IsUsableVehicle(veh)) {
                    continue;
                }
                if (vehDebug) {
                    DrawEntityDebugBound(dl, veh, vehBoundColor);
                }
                if (vehCol) {
                    DrawEntityColPartial(dl, veh, vehBoxColor, vehSphColor);
                }
            }
        }
    }
}